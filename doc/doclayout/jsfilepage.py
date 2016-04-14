"""<module>
This module generates an html page based on some file
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
from htmlcommon import *
from jscommon import *
from microdom import *
from constants import *

def genVars(obj):
  lst =  obj.getElementsByTagName(TagVariable)
  header = ["Global Variables","Documentation"]
  body = []
  for v in lst:
    if not v.findParent(TagClass): # Skip all variables that are actually defined in a class not at the top level
      body.append([v.name,str(v.data_)])
  grid = GridFromList(header, body )
  grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid

def genFns(obj):
  lst =  obj.getElementsByTagName(TagFunction)
  header = ["Global Functions","Args","Documentation"]
  body = []
  for m in lst:
    if m.findParent(TagClass): continue  # Skip all functions that are actually defined in a class not at the top level  
    args = m.getElementsByTagName(TagParam)
    args = [a.name for a in args]
    docstr = m.get(TagBrief,"")
    if not docstr:
      try:
          docstr = m.data_
      except IndexError:
          pass
    try:
      s = docstr.write()
    except:
      s = str(docstr)    
    body.append([m.name,",".join(args),s])
  grid = GridFromList(header, body )
  grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid

def genClasses(obj):
  lst =  obj.getElementsByTagName(TagClass)
  header = ["Classes","Documentation"]
  body = []
  for m in lst:
    args = m.getElementsByTagName(TagParam)
    args = [a.name for a in args]
    body.append([obj2link(m),str(m.data_)])
  grid = GridFromList(header, body )
  grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid



def generateold(obj,cfg):
  try:
    mv = genVars(obj)
    m =  genFns(obj)
    c =  genClasses(obj)

    # Figure out the upwards links  
    sectionobj  = obj.findParent("section")
    if sectionobj: s = ["Section: ",obj2tlink(sectionobj[0],PageLocCenter)]
    else: s= ""

    hdr = VSplit([resize(2,obj.name),s])
    ctr = HSplit([BR,mv,BR,m,BR,c])

    page = [hdr,ctr]
    fname = obj2file(obj)
    WriteFile("html"+os.sep+fname,page,HtmlFragment())
  except Exception, e:
    print e
    #pdb.set_trace()
    raise
 
  return (fname,page)


class Tainer:
  pass

def generate(obj,cfg,args,tagDict):
  
  sec = Tainer()
  sec.name = "all"
  sec.variables =  obj.getElementsByTagName(TagField) + obj.getElementsByTagName(TagVariable)
  sec.methods = obj.getElementsByTagName(TagFunction) + obj.getElementsByTagName(TagMethod)
  # sec.macros = obj.getElementsByTagName(TagMacro)
    
  out = {}
  out["g"] = globals()
  out["replace"] = PageLocCenter
  out["subSection"] = [sec]
  out["XESC"] = escape
  out["className"] = obj.name

  # Figure out the upwards links
  fileobj = obj.findParent(TagFile)
  if fileobj: out["ClassFile"] = obj2tlink(fileobj[0],PageLocCenter)
  else: out["ClassFile"] = ""
  
  sectionobj  = obj.findParent("section")
  if sectionobj: out["section"] = obj2tlink(sectionobj[0],PageLocCenter)
  else: out["section"]= ""

  out["this"] = obj
  out["R"] = render
  #out["flatSections"] = obj.filterByAttr(AttrTag,"section")
  template = kid.Template(file=cfg["html"]["skin"] + os.sep + "filedetails.xml",**out)
  # print str(template)

  fname = obj2file(obj)
  f = open("html"+os.sep+fname, "wb")
  f.write(str(template))
  f.close()
 
  return (fname,template)


#</module>
