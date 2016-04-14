"""<module>
This module generates an html page based on some class
"""
import pdb
import kid
from xml.sax.saxutils import escape

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
from jscommon import *
from htmlcommon import *
from constants import *


class Tainer:
  pass

def render2(obj):
  z = render(obj)
  if "You can override" in z:
    pdb.set_trace()
    t = render(obj)
    return t
  return z

def generateSectionFile(name,fname, objlist,cfg):
   page = []
   sec = Tainer()
   sec.name = name
   sec.variables = []
   sec.methods = []
   sec.classes = []
   sec.desc = []
   for obj in objlist:
     sec.name = name
     sec.variables += obj.childrenWithTag(TagField) + obj.childrenWithTag(TagVariable)
     sec.methods   += obj.childrenWithTag(TagFunction) + obj.childrenWithTag(TagMethod)
     sec.classes   += obj.childrenWithTag(TagClass)
     sec.desc      += obj.childrenWithTag(TagDesc)

   sec.desc.sort(key=lambda x: x.order if hasattr(x,"order") else x.name if hasattr(x,"name") else "zzzzzzzz")
   sec.variables.sort(key=lambda x: x.order if hasattr(x,"order") else x.name if hasattr(x,"name") else "zzzzzzzz")
   sec.methods.sort(key=lambda x: x.name)
   sec.classes.sort(key=lambda x: x.name)


   out = {}
   out["g"] = globals()
   out["replace"] = PageLocCenter
   out["R"] = render
   out["XESC"] = escape
   out["this"] = sec 
   template = kid.Template(file=cfg["html"]["skin"] + os.sep + "asection.xml",**out)
     

   # page = [ "section %s test" % fname ]
   # WriteFile(FilePrefix+fname,page,HtmlFragment())
   f = open(FilePrefix+fname, "wb")
   f.write(str(template))
   f.close()
   


def genBody(fllist,cfg):
  header = ["Sections"]
  body = []
  secDict = {}
  for obj in fllist:
    if not secDict.has_key(obj.name):
      secDict[obj.name] = []
    sec = secDict[obj.name]
    sec.append(obj)

  lst = secDict.items()
  lst.sort(key=lambda x: x[0])

  for name,objlist in lst:
    secFil = name + ".html"
    body.append( [obj2tlink(objlist[0],PageLocCenter,name,secFil)])
    generateSectionFile(name, secFil, objlist,cfg)

  grid = GridFromList(header, body )
  grid.RowAttrs({"class":"header sectionIndexHeaderRow"},[{"class":"rowA fileIndexRowA"},{"class":"rowB fileIndexRowB"}])
  #grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid


def generate(objs,cfg,args,tagDict):
  mv = genBody(objs,cfg)

  hdr = VSplit([resize(2,"")])
  ctr = HSplit([BR,mv])
  page = HeaderFooter(hdr, None,ctr)

  fname = "Section.html"
  WriteFile(FilePrefix+fname,page,HtmlFragment())
 
  return (fname,page)

#</module>
