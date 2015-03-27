"""<module>
This module generates a client-side search
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

from microdom import *

class Dotter:
  pass

def generate(mdtree,cfg,args,tagDict=None):
  me=Dotter()

  out = {}
  out['me'] = me
  out['R'] = render

  me.homeSections = mdtree.filterByAttr({"tag_":"section","name":"home"})
  templateFileName = cfg["html"]["skin"] + os.sep + "home.xml"
  template = kid.Template(file=templateFileName,**out)
  #print str(template)
  fname = "home.html"
  f = open("html"+os.sep+fname, "wb")
  f.write(str(template))
  f.close()
 
  return (fname,template)

#</module>
