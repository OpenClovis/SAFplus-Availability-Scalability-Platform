"""? <module>
This module generates an html page based on some class
"""
import pdb
import kid

from PyHtmlGen.gen import *
from PyHtmlGen.document import *
from PyHtmlGen.htmldoc import *
from PyHtmlGen.bar import *
# from layoutstyle import *
from PyHtmlGen.layouttable import *
from PyHtmlGen.table import *
from PyHtmlGen.imagehtml import *
from PyHtmlGen.menu import *
from PyHtmlGen.layouthtml import *
from PyHtmlGen.form import *
from PyHtmlGen.attribute import *
from PyHtmlGen.json import *
from PyHtmlGen.cssclass import *

from common import *
from htmlcommon import *
from jscommon import *
from constants import *

class Tainer:
  pass


def generate(obj,cfg):
  
  sec = Tainer()
  sec.name = "all"
  sec.variables =  obj.getElementsByTagName(TagField) + obj.getElementsByTagName(TagVariable)
  sec.methods   = obj.getElementsByTagName(TagFunction) + obj.getElementsByTagName(TagMethod)
  sec.constants = obj.getElementsByTagName(TagConst)
  sec.constructors = obj.getElementsByTagName(TagCtor)
  sec.classes = obj.getElementsByTagName(TagClass)[1:]  # [1:] is needed to skip obj since it is a class
    
  out = {}
  out["subSection"] = [sec]
  out["R"] = render
  out["className"] = obj.name
  out["desc"] = obj.desc
  out["classlink"] = lambda x: obj2tlink(x,PageLocCenter)

  # Figure out the upwards links
  fileobj = obj.findParent(TagFile)
  if fileobj: out["ClassFile"] = obj2tlink(fileobj[0],PageLocCenter)
  else: out["ClassFile"] = ""
  
  sectionobj  = obj.findParent("section")
  if sectionobj: out["section"] = obj2tlink(sectionobj[0],PageLocCenter)
  else: out["section"]= ""
  out["obj2anchor"] = obj2anchor

  template = kid.Template(file=cfg["html"]["skin"] + os.sep + "classdetails.xml",**out)
  # print str(template)

  fname = obj2file(obj)
  f = open("html"+os.sep+fname, "wb")
  f.write(str(template))
  f.close()
 
  return (fname,template)

#?</module>
