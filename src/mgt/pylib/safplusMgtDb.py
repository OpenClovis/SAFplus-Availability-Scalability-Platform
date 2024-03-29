#!/usr/bin/env python3
"""
@namespace DBAL 

This module provides interfaces to access the DBAL provided by SAFplus library.
"""
import pdb, traceback
import os, sys, os.path
import re
import copy

try:
  import pyDbal
  import microdom
  from common import *
except ImportError as e:
  print ("Unable to import SAFplus libraries.  Is your PATH and PYTHONPATH set up properly?")
  print ("Error was:", str(e))
  exit(1)

from xml.etree import ElementPath
from xml.etree import ElementTree as ET
from xml.dom import minidom

cfgpath = os.environ.get("SAFPLUS_CONFIG",os.environ.get("SAFPLUS_BINDIR","."))

InvisibleTags = ["top"]

SAFPLUS_VERSION = "7.0.0"
ROOT_WRAPPER = "SAFplus"

SAFplusError = {
    0x00:"CL_OK",
    0x02:"CL_ERR_INVALID_PARAMETER",
    0x03:"CL_ERR_NULL_POINTER",
    0x04:"CL_ERR_DOESNT_EXIST",
    0x05:"CL_ERR_INVALID_HANDLE",
    0x08:"CL_ERR_DUPLICATE",
    0x0e:"CL_ERR_NOT_INITIALIZED",
    0x10:"CL_ERR_ALREADY_EXIST",
    0x11:"CL_ERR_UNSPECIFIED",
    0x12:"CL_ERR_INVALID_STATE",
    0x13:"CL_ERR_DOESNT_EXIST",
    0x100:"CL_DBAL_ERR_DB_ERROR",
    0x101:"CL_DBAL_ERR_COMMIT_FAILED",
    0x102:"CL_DBAL_ERR_ABORT_FAILED"
}

PREDICATE_KEY_START = '['
PREDICATE_KEY_END = ']'

def key_to_xml(s):
    s = s.replace('-', '_')
    s = s.replace('/', '_')
    s = s.replace(':', '_')
    #tag without array index
    #/c[1] => /c
    s = s.split("[")[0]
    return s

def getErrorString(errCode):
    if type(errCode) == str:
        return errCode
    try:
        retStr = SAFplusError[errCode[0]]
    except KeyError:
        traceback.print_exc()
        retStr = ''
    return retStr

def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ET.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")

class PyDBAL():
    """
    @param fileName: 
        This parameter specifies the file name (without suffix) along with its path,
        where the database is to be created.
    @param maxKeySize:
        Maximum size of the key (in bytes) to be stored in the database.
    @param maxRecordSize:
        Maximum size of the record (in bytes) to be stored in the database.
    @param docRoot:
        Decode cfg file and set doc root tree
    """
    def __init__(self, fileName, maxKeySize = 4, maxRecordSize = 8, docRoot=None):
        self.cfgfile = "%s" % fileName
        self.suppliedData = None
        self.dbName = os.path.splitext(fileName)[0]
        self.docRoot = docRoot
        pyDbal.initialize(self.dbName, "", maxKeySize, maxRecordSize)

    """ Load cfg xml and save to binary database """
    def LoadXML(self, xmlFile = None):
        if xmlFile is None: xmlFile = self.cfgfile
        try:
            self.suppliedData = microdom.LoadFile(xmlFile)

            # This is IDE's model?
            if self.suppliedData.tag_ == 'SAFplusModel':
                self.docRoot = 'instances'

            if self.docRoot is not None:
                self.suppliedData = self.suppliedData.get(self.docRoot)

        except IOError as e: # If the supplied data file does not exist just print a warning
            raise Exception("Supplied data file %s does not exist.  You may need to export SAFPLUS_CONFIG enviroment variable" % xmlFile)

        except Exception as e: # If the supplied data file does not exist just print a warning
            raise Exception("Invalid format XML file!")

        self.xpathDB = {}
        self.xpathMetadataDB = {}
        self.xpathParentDB = {}

        #Process first phrase to retrieve xpath
        self._load(self.suppliedData)

        #Convert list container or list-leaf to array list
        map_change_xpath = {}

        #Write metadata into db
        for (k,v)  in self.xpathMetadataDB.items():
            self.Write(k, '', v)

        #Done customize xpath
        #Unit test http://xmltoolbox.appspot.com/xpath_generator.html
        for (key, value) in sorted(self.xpathDB.items()):
          self.Write(key, value)

    """ Load cfg xml and save to binary database """
    def _load(self, element, xpath = ''):
        child = None
        parentXpath = xpath
        if isInstanceOf(element, microdom.MicroDom):
            tag = microdom.keyify(element.tag_)
            if not tag in InvisibleTags:
              xpath = "%s/%s" %(xpath, tag)

            # Work-around for loading from IDE's model
            if element.attributes_.get('xpath') is not None:
                tmp = element.attributes_.get('xpath')
                xpath, attributes = self._buildMetadata(tmp)
                element.attributes_.pop('xpath')
                element.attributes_.update(attributes)

            if element.attributes_.get('module') is not None:
                element.attributes_.pop('module')

            if len(element.attributes_) > 1:
                # Format attribute string with 'and' expression for each attribute
                attrs = [(microdom.keyify(k),v) for (k,v) in filter(lambda i: i[0] != 'tag_', element.attributes_.items())]
                attrKey = " and ".join(["@%s=\"%s\"" %(k,v) for (k,v) in attrs])

                try:
                    self.xpathMetadataDB[xpath].append("[%s]" %attrKey)
                except:
                    self.xpathMetadataDB[xpath] = ["[%s]" %attrKey]

                xpath = xpath + "[%s]" %attrKey
                child = "%s[%s]" %(microdom.keyify(element.tag_), " and ".join(["@%s=\"%s\"" %(k,v) for (k,v) in attrs]))
            else:
                child = microdom.keyify(element.tag_)

            #Add index key into list container, also modify old one with index [1]
            if xpath in self.xpathParentDB:
                #Update old xpath to new with append suffix [1]
                if self.xpathParentDB[xpath] == 1:
                    # Lookup on xpathDB and replace with extra index [1]
                    for key in list(filter(lambda key: key.rfind(xpath) != -1, self.xpathDB.keys())):
                        key_prefix = xpath + "[1]"
                        key_new = key.replace(xpath, key_prefix)
                        self.xpathDB[key_new] = self.xpathDB.pop(key)

                        #Create new reference to xpath: a/b/c => [1]
                        self.xpathMetadataDB[xpath] = ["[1]"]

                #Now, all having [index], just increase
                self.xpathParentDB[xpath] +=1
                idx = "[%d]" %self.xpathParentDB[xpath]

                #Append new reference to xpath: a/b/c => [2],[3],...
                self.xpathMetadataDB[xpath].append(idx)

                child = None # Don't add duplicate child into parent xpath
                xpath = xpath + idx
            else:
                self.xpathParentDB[xpath] = 1

            # Store metadata for parent xpath
            if parentXpath != '' and child != None:
                try:
                    self.xpathMetadataDB[parentXpath].append(child)
                except:
                    self.xpathMetadataDB[parentXpath] = [child]

            if len(element.children()) > 0:
                for elchild in element.children():
                    self._load(elchild, xpath)
            elif len(str(element.data_).strip()) > 0:
                self.xpathDB[xpath] = str(element.data_).strip()

        elif len(str(element).strip()) > 0:
            self.xpathDB[xpath] = str(element).strip()

    def _buildMetadata(self, xpath, metadata = {}):
        parentXpath = xpath[:xpath.rfind("/")]
        childXpath = xpath[xpath.rfind("/") + 1:]

        if len(parentXpath) > 0:
            try:
                metadata[parentXpath].append(childXpath)
            except:
                metadata[parentXpath] = [childXpath]

        attributes = None
        iterObj = iter(ElementPath.xpath_tokenizer(childXpath))

        while 1:
                try:
                    token = next(iterObj)
                    if token[0] == "[":
                        attributes = self._transformAttr(iterObj, token)
                except StopIteration:
                    break

        if attributes is not None:
            childXpath = xpath[:xpath.rfind("[")]

        return childXpath, attributes

    """ Build element attribute """
    def _transformAttr(self, iterObj, token):
        predicate = []
        attributes = {}

        #This is [list] attribute value of a node element
        #Ex: /a/b/c[@name="foo" and @id="bar"] => attributes {name=foo, id=bar}
        if token[0] == "[":
            while 1:
                token = next(iterObj)
                if token[0] == "]":
                    break
                if token[0] and token[0][:1] in "'\"":
                    token = "'", token[0][1:-1]
                predicate.append(token[1])

            #This is array index [1] ... [2] ...(already combine with @attributes)
            if not re.match("\d+$", predicate[0]):
                while len(predicate)>3:
                    #Ignore 'and' expression token
                    if predicate[0] == 'and':
                        predicate = predicate[2:]
                    key = predicate[1]
                    value = predicate[3]
                    # shift left
                    predicate = predicate[5:]
                    attributes[key_to_xml(key)] = value

        return attributes

    def _buildNode(self, current_xpath):
        current_elemment = None
        p_parent_xpath = None
        try:
            current_elemment = self.mapparentelem[current_xpath]
        except:
            try:
                p_parent_xpath = current_xpath[:current_xpath.rfind("/")]
                token = current_xpath[current_xpath.rfind("/") + 1:]

                # TODO: This token is attributes value
                if token[0] == '@':
                    token = token[1:]

                #Get current element with xpath:current_xpath if exists
                parent_elemment = self.mapparentelem[p_parent_xpath]

                current_elemment = ET.Element(key_to_xml(token))
                self.mapparentelem[current_xpath] = current_elemment
                parent_elemment.append(current_elemment)

            except:
                """ Backward up level of parent xpath """
                current_elemment = self._buildNode(p_parent_xpath)
        return current_elemment

    """ Export to cfg xml from binary database """
    def ExportXML(self, filename = None):
        """ Build element tree from xpaths """

        # Tree root node
        root = None

        #Map to store xpath and ElementXpath
        self.mapparentelem = {}

        ''' Select all rows of db to export '''
        for xpath in self.Iterators('/'):
            #Is attribute xpath? /a/b/c@name => attr name
            attr_xpath = False 

            try:
              value = str(self.Read(xpath)[0])
            except SystemError as e:
              print ("Cannot read [%s], error %s" % (xpath,str(e)))
              continue
            iterObj = iter(ElementPath.xpath_tokenizer(xpath))

            #For each leaf node on xpath
            current_elemment = root

            #parent xpath
            parent_xpath = None

            #current xpath token
            current_xpath = None

            while 1:
                try:
                    token = next(iterObj)
                    if token[0] == "/":
                        token = next(iterObj)

                        #Ignore attribue element (xpath=//sg/@name)
                        if token[0] == "@":
                            attr_xpath = True
                            continue

                        index = xpath.rfind("/%s" %token[1])
                        if index >= 0:
                            parent_xpath = "%s" %(xpath[:index])

                            #Get xpath assume suffix is leaf
                            next_index = xpath.find("/", len(parent_xpath) + 1)
                            if next_index>0:
                                current_xpath = xpath[0:next_index]
                            else:
                                #Last leaf
                                current_xpath = xpath

                            if root is None:
                                root = ET.Element(key_to_xml(token[1]))
                                current_elemment = root
                                self.mapparentelem["/%s" %token[1]] = current_elemment
                            else:
                                current_elemment = self._buildNode(current_xpath)
                    elif token[0] == "[":
                        attributes = self._transformAttr(iterObj, token)
                        if len(attributes) > 0:
                            current_elemment.attrib = attributes

                except StopIteration:
                    break

            if attr_xpath == False:
                current_elemment.text = value

        #Write to xml or stdout
        if filename is None:
            filename = "%s.xml" %self.dbName

        file = open(filename, "w")

        ''' Wrapper into <SAFplus version="..."> ... </SAFplus>'''
        wrapper_safplus = ET.Element(ROOT_WRAPPER)
        wrapper_safplus.attrib['version'] = SAFPLUS_VERSION
        if root is not None:
          wrapper_safplus.append(root)
        else:  
          print ("Warning: empty database")
        file.write(prettify(wrapper_safplus))
        file.close()

    def Finalize(self):
        pyDbal.finalize()

    def Iterators(self, xpath=None):
        if xpath is not None:
            try:
                keys = pyDbal.iterators(xpath)
                return keys
            except:
                return []
        else:
            return pyDbal.iterators('/') #Default select all rows of db

    def mergeNonSafComp2Comp(self, key, nonSafCompValue):
        print ('[A0] reading child of key [%s]'%key)
        ret = self.Read(key)
        if ret:
            value = ret[0]
            child = ret[1]
            if value and len(value)>0 and len(child)==0:
                print ('[A0] value of key [%s] is [%s]:'%(key,value))
                compIdxList = ['[1]','[2]']
                #Writing /safplusAmf/ServiceUnit[@name="ServiceUnit13"]/nonSafComponents ->  childs: [[1],[2]...]
                pyDbal.write(key,'',compIdxList)
                #Write [/safplusAmf/ServiceUnit[@name="ServiceUnit13"]/components[n]] -> [Componentxy] children [0]
                k = key+compIdxList[0]
                print ("[A0] Writing %s -> %s childs: [0]" % (str(k),value))
                pyDbal.write(k,value,[])
                #Write [/safplusAmf/ServiceUnit[@name="ServiceUnit13"]/components[n]] -> [Componentxy] children [0]
                k = key+compIdxList[1]
                print ("[A0] Writing %s -> %s childs: [0]" % (str(k),nonSafCompValue))                
                pyDbal.write(k,nonSafCompValue,[])
            else:
                print ('[A0] child of key [%s] is of size [%d]:'%(key,len(child)))          
                for c in child:
                    print ('[A0]: %s'%c)  # c is [n]
                c = child[len(child)-1]
                i1 = c.find('[')
                i2 = c.find(']')
                lastIdx=int(c[i1+1:i2])
                lastIdx+=1
                child.append('[%d]'%lastIdx)
                print ('[A0] new child of key [%s] is of size [%d]:'%(key,len(child)))
                for c in child:
                    print ('[A0]: %s'%c)  # c is [n]
                print ("[A0] Writing %s -> childs: [%d]" % (str(key),len(child)))
                #Writing /safplusAmf/ServiceUnit[@name="ServiceUnit13"]/nonSafComponents ->  childs: [[1],[2]...]
                pyDbal.write(key,'', child)
                #Write [/safplusAmf/ServiceUnit[@name="ServiceUnit13"]/components[n]] -> [Componentxy] children [0]
                k = '%s[%d]'%(key,lastIdx)
                print ("[A0] Writing %s -> %s childs: [0]" % (str(k),nonSafCompValue))
                pyDbal.write(k,nonSafCompValue,[])
        else:
            print ("[A0] Writing %s -> %s childs: [0]" % (str(key),nonSafCompValue))
            pyDbal.write(key,nonSafCompValue,[])

    def Write(self, key, data, childs = []):
        print ("Writing %s -> %s childs: [%s]" % (str(key), str(data), ",".join(childs)))
        recordWritten = False
        i = key.rfind('nonSafComponents[')
        if i>1:
            #oriKey = key
            print ('[A] case nonSafComponents[')
            key = key[:i]+'components'
            self.mergeNonSafComp2Comp(key,data)
            recordWritten = True
        else:           
           i = key.rfind('nonSafComponents')
           if i>1 and len(childs)==0:
               print ('[A] case nonSafComponents without childs')
               key = key[:i]+'components'
               self.mergeNonSafComp2Comp(key,data)
               recordWritten = True
        children = copy.copy(childs)
        if 'nonSafComponents' in childs:
            children = filter(lambda child: child != 'nonSafComponents', childs)
            if 'components' not in children:
                children.append('components')
        if not recordWritten:
            print ("[A] Writing %s -> %s childs: [%s]" % (str(key), str(data), ",".join(children)))
            return pyDbal.write(key, data, children)
        return None

    def Read(self, key):
        ret = None
        try:
            ret = pyDbal.read(key)
        except Exception as e:
            print ('Read exception, message [%s]'%str(e))
        return ret   
    
