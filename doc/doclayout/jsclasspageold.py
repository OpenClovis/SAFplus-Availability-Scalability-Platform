"""? <module>
This module generates an html page based on some class
"""
import pdb

import sys
sys.path.append("/me/code")


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


def genMemVars(obj):
  lst =  obj.getElementsByTagName(TagField) + obj.getElementsByTagName(TagVariable)
  header = ["Member Variable","Type", "Documentation"]
  body = []
  for mv in lst:
    body.append([mv.name,mv.type,str(mv.children_[0])])
  grid = GridFromList(header, body )
  grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid

def genMethods(obj):
  lst =  obj.getElementsByTagName(TagFunction) + obj.getElementsByTagName(TagMethod)
  header = ["Methods","Returns", "Args","Documentation"]
  body = []
  for m in lst:
    args = m.getElementsByTagName(TagParam)
    args = [a.name for a in args]
    body.append([m.name,m.type,",".join(args),str(m.children_[0])])
  grid = GridFromList(header, body )
  grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid



def generate(obj,cfg):

  mv = genMemVars(obj)
  m = genMethods(obj)

  # Figure out the upwards links
  fileobj = obj.findParent(TagFile)
  if fileobj: f = ["File: ", obj2tlink(fileobj[0],PageLocCenter)]
  else: f = ""
  
  sectionobj  = obj.findParent("section")
  if sectionobj: s = ["Section: ",obj2tlink(sectionobj[0],PageLocCenter)]
  else: s= ""

  hdr = VSplit([resize(2,obj.name),f,s])
  ctr = HSplit([BR,mv,BR,m])

  page = [hdr,ctr]
  fname = obj2file(obj)
  WriteFile("html"+os.sep+fname,page,HtmlFragment())
 
  return (fname,page)

#?</module>
