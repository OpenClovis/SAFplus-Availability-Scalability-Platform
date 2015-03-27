"""<module>
This module generates an html page that lists all classes
"""
import pdb

import sys

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

from microdom import *

def genSymbols(cllist):
  header = ["symbol","class","section","file"]
  body = []
  for (obj,cls,sec,fil) in cllist:
    body.append([obj2tlink(obj,PageLocCenter),obj2tlink(cls,PageLocCenter),obj2tlink(sec,PageLocCenter),obj2tlink(fil,PageLocCenter)])
  # parenttLink(obj,TagSection,PageLocCenter),parenttLink(obj,TagFile,PageLocCenter
  grid = GridFromList(header, body )
  #grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  grid.RowAttrs({"class":"header indexHeaderRow"},[{"class":"rowA indexRowA"},{"class":"rowB indexRowB"}])
  return grid


def generate(objs,cfg,args,tagDict=None):

  objlst = []
  for obj in objs.walk():
    if isInstanceOf(obj,MicroDom):
      if obj.tag_ in ConstructTags or obj.tag_ is TagField:  # Only pick out certain items for the index
        if obj.attributes_.has_key(AttrName):
          c = obj.findParent(TagClass)
          if c: c = c[0]
          else: c=None
          s = obj.findParent(TagSection)
          if s: s = s[0]
          else: s=None
          f = obj.findParent(TagFile)
          if f: f = f[0]
          else: f=None

          objlst.append((obj,c,s,f))

  objlst.sort(lambda x,y: cmp(x[0].name.lower(),y[0].name.lower()))

  mv = genSymbols(objlst)

  hdr = VSplit([resize(2,"Index")])
  ctr = HSplit([BR,mv])

  fname = "idx.html"
  page = [hdr,ctr]
  WriteFile(FilePrefix + fname,page,HtmlFragment())
 
  return (fname,page)

#</module>
