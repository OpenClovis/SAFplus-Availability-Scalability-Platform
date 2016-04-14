"""<module>
This module generates an html page that lists all classes
"""
import pdb

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

def genClasses(cllist):
  header = ["Class","Section","File"]
  body = []
  for obj in cllist:
    body.append([obj2tlink(obj,PageLocCenter),parenttLink(obj,TagSection,PageLocCenter),parenttLink(obj,TagFile,PageLocCenter)])

  grid = GridFromList(header, body )
  #grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  grid.RowAttrs({"class":"classIndexHeaderRow"},[{"class":"classIndexRowA"},{"class":"classIndexRowB"}])
  return grid


def generate(objs,cfg,args,tagDict):

  objs.sort(key=lambda x: x.name)
  mv = genClasses(objs)

  hdr = VSplit([resize(2,"Class Directory")])
  ctr = HSplit([BR,mv])
  fname = "Class.html"
  page = [hdr,ctr]
  WriteFile(FilePrefix + fname,page,HtmlFragment())
 
  return (fname,page)

#</module>
