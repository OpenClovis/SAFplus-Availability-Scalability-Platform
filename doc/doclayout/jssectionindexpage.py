"""<module>
This module generates an html page based on some class
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
from jscommon import *
from htmlcommon import *
from constants import *

def genBody(fllist):
  header = ["Sections"]
  body = []
  for obj in fllist:
    body.append([obj2tlink(obj,PageLocCenter)])

  grid = GridFromList(header, body )
  grid.RowAttrs({"class":"header sectionIndexHeaderRow"},[{"class":"rowA fileIndexRowA"},{"class":"rowB fileIndexRowB"}])
  #grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  return grid


def generate(objs,cfg):
  mv = genBody(objs)

  hdr = VSplit([resize(2,"")])
  ctr = HSplit([BR,mv])
  page = HeaderFooter(hdr, None,ctr)

  fname = "section.html"
  WriteFile(FilePrefix+fname,page,HtmlFragment())
 
  return (fname,page)

#</module>
