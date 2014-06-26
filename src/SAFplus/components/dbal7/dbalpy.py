"""
@namespace DBAL 

This module provides interfaces to access the DBAL provided by SAFplus library.
"""
import pdb
import os, sys
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
        self._load(self.suppliedData)

        #Convert list container or list-leaf to array list
        for (k, v) in self.xpathParentDB.items():
            if v > 1:
                try:
                    self.xpathDB[k + "[1]"] = self.xpathDB.pop(k)
                    del self.xpathParentDB[k]
                except:
                    # Lookup on xpathDB and replace with extra index
                    for key in filter(lambda key: key.rfind(k + "/") != -1, self.xpathDB.keys()):
                        key_prefix = k + "[1]"
                        key_new = key.replace(k, key_prefix)
                        self.xpathDB[key_new] = self.xpathDB.pop(key)

        for (key, value) in sorted(self.xpathDB.items()):
            try:
                self.Write(key, value)
            except:
                #Duplicate key
                pass

    """ Load cfg xml and save to binary database """
    def _load(self, element, xpath = ''):
        if isInstanceOf(element, microdom.MicroDom):
            xpath = "%s/%s" %(xpath, microdom.keyify(element.tag_))
            if len(element.attributes_) > 1:
                # Format attribute string
                attrs = ["@%s='%s'" % (microdom.keyify(k),v) for (k,v) in filter(lambda i: i[0] != 'tag_', element.attributes_.items())]

                attrs = PREDICATE_KEY_START + ",".join(attrs) + PREDICATE_KEY_END
                xpath += attrs
            if self.xpathParentDB.has_key(xpath):
                self.xpathParentDB[xpath] +=1
                xpath = xpath + "[%d]" %self.xpathParentDB[xpath]
            else:
                self.xpathParentDB[xpath] = 1

            if len(element.children()) > 0:
                for elchild in element.children():
                    self._load(elchild, xpath)
            else:
                self.xpathDB[xpath] = element.data_

        elif len(str(element).strip()) > 0:
            self.xpathDB[xpath] = str(element).strip()

    """ Build element attribute """
    def _transformAttr(self, next, token):
        predicate = []
        attributes = {}
        while 1:
            token = next()
            if token[0] == "]":
                break
            if token[0] and token[0][:1] in "'\"":
                token = "'", token[0][1:-1]
            predicate.append(token[1])
        while len(predicate)>3:
            key = predicate[1]
            value = predicate[3]
            # shift left
            predicate = predicate[5:]
            attributes[key_to_xml(key)] = value
        return attributes

    """ Export to cfg xml from binary database """
    def ExportXML(self, filename = None):
        """ Build element tree from xpaths """

        # Tree root node
        root = None
        mapparentelem = {}
        try:
            for xpath in self.Iterators():
                next = iter(ElementPath.xpath_tokenizer(xpath)).next

                # For each leaf node on xpath
                current_elemment = root

                #parent xpath
                pelem_xpath = None

                #current xpath token
                celem_xpath = None

                while 1:
                    try:
                        token = next()
                        if token[0] == "/":
                            token = next()
                            index = xpath.find(token[1])
                            if index>1:
                                pelem_xpath = "%s" %(xpath[:index-1])

                                index_cur = xpath.find("/", len(pelem_xpath) + 1)
                                if index_cur>0:
                                    celem_xpath = xpath[0:index_cur]
                                else:
                                    celem_xpath = xpath

                            if root is None:
                                root = ET.Element(key_to_xml(token[1]))
                                current_elemment = root
                                mapparentelem["/%s" %token[1]] = current_elemment
                            else:
                                try:
                                    #Get current element with xpath:celem_xpath if exists
                                    current_elemment = mapparentelem[celem_xpath]
                                except Exception, e:
                                    try:
                                        #Build current element with xpath: celem_xpath
                                        current_elemment = ET.Element(key_to_xml(token[1]))
                                        mapparentelem[celem_xpath] = current_elemment

                                        #Try to find parent element with xpath:pelem_xpath
                                        pelem = mapparentelem[pelem_xpath]
                                        pelem.append(current_elemment)
                                    except Exception, e:
                                        """ Backward up level of parent xpath """

                                        #xpath of parent's parent
                                        parent_pelem_xpath = pelem_xpath[:pelem_xpath.rfind("/")]

                                        #element tag of parent
                                        parent_token = pelem_xpath[pelem_xpath.rfind("/") + 1 :]
                                        try:
                                            #Build parent element
                                            pelem = ET.Element(key_to_xml(parent_token))
                                            pelem.append(current_elemment)
                                            mapparentelem[pelem_xpath] = pelem

                                            #Try to find parent element with xpath:parent_pelem_xpath (up to only one level)
                                            parent_elem = mapparentelem[parent_pelem_xpath]
                                            parent_elem.append(pelem)
                                        except Exception, e:
                                            pass
                        else:
                            attributes = self._transformAttr(next, token)
                            current_elemment.attrib = attributes

                    except Exception, e:
                        break
            
                current_elemment.text = str(self.Read(xpath))
    
            #Write to xml or stdout
            if filename is None:
                filename = "%s.xml" %self.dbName
    
            elmtree = ET.ElementTree(root)
            elmtree.write(filename)
        except:
            pass

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
    print etErrorString(e)

  try:
    #Store into binary database
    if argv[1] == '-x':
      data.StoreDB();

    #Export to XML from binary database
    if argv[1] == '-d':
      data.ExportXML();

  except Exception, e:
    print getErrorString(e)

  data.Finalize()

def Test():
    test = PyDBAL("SAFplusLog.xml") # Root of Log service start from /log ->  docRoot= "version.log_BootConfig.log"
    data.StoreDB();

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
