"""
@namespace DBAL 

This module provides interfaces to access the DBAL provided by SAFplus library.
"""
import pdb
import os, sys
import re
import pyDbal
import microdom
from common import *

from xml.etree import ElementPath
from xml.etree import ElementTree as ET

cfgpath = os.environ.get("SAFPLUS_CONFIG",os.environ.get("SAFPLUS_BINDIR","."))

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
  s = s.replace("_",":")
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
        retStr = "UNKNOWN_ERROR_CODE"
   
    return retStr

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
        self.dbName = fileName.split('.')[0]
        self.docRoot = docRoot
        pyDbal.initializeDbal(self.dbName, maxKeySize, maxRecordSize)

    """ Load cfg xml and save to binary database """
    def StoreDB(self):
        try:
            self.suppliedData = microdom.LoadFile(self.cfgfile)
            if self.docRoot:
                self.suppliedData = self.suppliedData.get(self.docRoot)

        except IOError, e: # If the supplied data file does not exist just print a warning
            raise Exception("Supplied data file %s does not exist.  You may need to export SAFPLUS_CONFIG enviroment variable" % self.cfgfile)

        except Exception, e: # If the supplied data file does not exist just print a warning
            raise Exception("Invalid format XML file!")

        self.xpathDB = {}
        self.xpathParentDB = {}
        
        #Mapping to store container has attribute(s)
        self.xpath2phrase = {}

        #Process first phrase to retrieve xpath
        self._load(self.suppliedData)

        #Convert list container or list-leaf to array list
        map_change_xpath = {}

        for k in self.xpathParentDB.keys():
            if self.xpathParentDB[k] == 1:
                #Search through attributes and build new index
                try:
                    tmp_key = map_change_xpath[k]
                except:
                    tmp_key = k
    
                if tmp_key in self.xpath2phrase.keys():
                    #Add index by append attributes [name="value" ...]
                    key_prefix = tmp_key + "[%s]" %self.xpath2phrase.get(tmp_key)
    
                    #Modify key from first phrase
                    for key in self.xpathDB.keys():
                        if key.rfind(tmp_key) != -1:
                            key_new = key.replace(tmp_key, key_prefix)
                            self.xpathDB[key_new] = self.xpathDB.pop(key)
                            map_change_xpath[key] = key_new
    
                    #Also modify key if exiting on phrase 2 considering
                    for key in filter(lambda key: key.rfind(tmp_key) != -1, self.xpath2phrase.keys()):
                        key_new = key.replace(tmp_key, key_prefix)
                        self.xpath2phrase[key_new] = self.xpath2phrase.pop(key)

        #Done customize xpath
        #Unit test http://xmltoolbox.appspot.com/xpath_generator.html
        for (key, value) in sorted(self.xpathDB.items()):
            try:
                self.Write(key, value)
            except Exception, e:
                #Duplicate key
                pass

    """ Load cfg xml and save to binary database """
    def _load(self, element, xpath = ''):
        if isInstanceOf(element, microdom.MicroDom):
            xpath = "%s/%s" %(xpath, microdom.keyify(element.tag_))

            #Add index key into list container, also modify old one with index [1]
            if self.xpathParentDB.has_key(xpath):
                if self.xpathParentDB[xpath] == 1:
                    # Lookup on xpathDB and replace with extra index [1]
                    for key in filter(lambda key: key.rfind(xpath) != -1, self.xpathDB.keys()):
                        key_prefix = xpath + "[1]"
                        key_new = key.replace(xpath, key_prefix)
                        self.xpathDB[key_new] = self.xpathDB.pop(key)

                #Now, all having [index], just increase
                self.xpathParentDB[xpath] +=1
                xpath = xpath + "[%d]" %self.xpathParentDB[xpath]
            else:
                self.xpathParentDB[xpath] = 1

            if len(element.attributes_) > 1:
                # Format attribute string
                attrs = [(microdom.keyify(k),v) for (k,v) in filter(lambda i: i[0] != 'tag_', element.attributes_.items())]
                for el in attrs:
                    self.xpathDB["%s@%s" %(xpath, el[0])] = el[1]
                    self.xpathParentDB["%s@%s" %(xpath, el[0])] = 1

                #Put this xpath into second phrase considering
                self.xpath2phrase[xpath] = " ".join(["@%s=\"%s\"" %(k,v) for (k,v) in attrs])

            if len(element.children()) > 0:
                for elchild in element.children():
                    self._load(elchild, xpath)
            else:
                self.xpathDB[xpath] = str(element.data_)

        elif len(str(element).strip()) > 0:
            self.xpathDB[xpath] = str(element).strip()

    """ Build element attribute """
    def _transformAttr(self, next, token):
        predicate = []
        attributes = {}

        #This is attribute value of a node element
        #Ex: /a/b/c[1]@name => attribute 'name'
        if token[0] == "@":
            token = next()
            return token[1]

        #This is [list] attribute value of a node element
        #Ex: /a/b/c[@name="foo" @id="bar"] => attributes {name=foo, id=bar}
        if token[0] == "[":
            while 1:
                token = next()
                if token[0] == "]":
                    break
                if token[0] and token[0][:1] in "'\"":
                    token = "'", token[0][1:-1]
                predicate.append(token[1])

            #This is array index [1] ... [2] ...(already combine with @attributes)
            if not re.match("\d+$", predicate[0]):
                while len(predicate)>3:
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
        for xpath in self.Iterators():
            #Is attribute xpath? /a/b/c@name => attr name
            attr_xpath = False 

            value = str(self.Read(xpath))
            next = iter(ElementPath.xpath_tokenizer(xpath)).next

            #For each leaf node on xpath
            current_elemment = root

            #parent xpath
            parent_xpath = None

            #current xpath token
            current_xpath = None

            while 1:
                try:
                    token = next()
                    if token[0] == "/":
                        token = next()
                        index = xpath.rfind(token[1])
                        if index > 0:
                            parent_xpath = "%s" %(xpath[:index-1])

                            #Get xpath assume suffix is leaf
                            index_cur = xpath.rfind("/", len(parent_xpath) + 1)
                            if index_cur>0:
                                current_xpath = xpath[0:index_cur]
                            else:
                                #Get xpath until match '@', assume suffix is attr name  
                                index_cur = xpath.rfind("@", len(parent_xpath) + 1)
                                if index_cur>0:
                                    current_xpath = xpath[0:index_cur]
                                else:
                                    current_xpath = xpath

                            if root is None:
                                root = ET.Element(key_to_xml(token[1]))
                                current_elemment = root
                                self.mapparentelem["/%s" %token[1]] = current_elemment
                            else:
                                current_elemment = self._buildNode(current_xpath)
                    else:
                        attributes = self._transformAttr(next, token)
                        #attributes type: /a/b/c[1]@name => /a/b/c[name="value"]
                        if type(attributes) == str:
                            attr_xpath = True
                            current_elemment.attrib[attributes] = value
                        #attributes type: /a/b/c[name="foo" id="bar"]/name => /a/b/c[name="foo" id="bar"]
                        elif len(attributes) > 0:
                            current_elemment.attrib = attributes

                except StopIteration:
                    break

            if attr_xpath == False:
                current_elemment.text = value

        #Write to xml or stdout
        if filename is None:
            filename = "%s.xml" %self.dbName

        elmtree = ET.ElementTree(root)
        elmtree.write(filename)


    def Finalize(self):
        pyDbal.finalizeDbal()

    def Iterators(self, xpath=None):
        if xpath:
            try:
                keys = [key for key in pyDbal.iterators() if key.startswith(xpath)]
                return keys
            except:
                return []
        else:
            return pyDbal.iterators()

    def Write(self, key, data):
        return pyDbal.write(key, data)
    
    def Read(self, key):
        return pyDbal.read(key)

    def Replace(self, key, data):
        if key in self.Iterators():
            try:
                return pyDbal.replace(key, data)
            except Exception, e:
                return e
        raise ValueError(0x13)


def main(argv):
  if len(argv) != 3:
    print "Usage:\n '%s -x <xml file>': Encode xml file to database file\n" % argv[0]
    print " '%s -d <database file>': Read database file into xml file\n" % argv[0]
    return -1

  try:
    data = PyDBAL(argv[2])
  except Exception, e:
    print getErrorString(e)
    return

  #Store into binary database
  if argv[1] == '-x':
    data.StoreDB();

  #Export to XML from binary database
  if argv[1] == '-d':
    data.ExportXML();


  data.Finalize()

def Test():
    test = PyDBAL("SAFplusLog.xml") # Root of Log service start from /log ->  docRoot= "version.log_BootConfig.log"
    test.StoreDB();

    # Iterators
    for key in test.Iterators():
       try:
           data = test.Read(key)
           print "[%s] -> [%s]" %(key, data)
       except Exception, e:
           print getErrorString(e)

    #Replace data with a key
    try:
        key="/SAFplusLog/ServerConfig/maximumSharedMemoryPages"
        test.Replace(key, "100")
    except Exception, e:
        print getErrorString(e)

    try:
        key="/SAFplusLog/ServerConfig/maximumRecordsInPacket"
        test.Replace(key,"150")
    except Exception, e:
        print getErrorString(e)

    #Export to XML
    test.ExportXML("SAFplusLog_New.xml")

    test.Finalize()

if __name__ == '__main__':
  main(sys.argv)
