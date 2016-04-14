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

def linker(x):
  if hasattr(x,"generatedTo") and x.generatedTo:
    return obj2tlink(x,PageLocCenter,x.name,x.generatedTo)
  else:
    return x.name

def generateContent(cfg, obj, contentSkin):
  ns = {}
  ns["me"] = obj
  ns['R'] = render
  template = kid.Template(file=cfg["html"]["skin"] + os.sep + contentSkin,**ns)
  f = open("html"+os.sep+obj.generatedTo, "wb")
  f.write(str(template))
  f.close()
 
def generate(mdtree,cfg,args,tagDict):
  me=Dotter()
  title,sectionFilter,contentSkin = args
  out = {}
  out['me'] = me
  out['R'] = render
  out['title']=title
  out['link']=lambda x: linker(x)
  me.sections = mdtree.filterByAttr(sectionFilter)
  for m in me.sections:
    brief = m.find("brief")
    if brief: m.brief = brief[0][1]
    else: m.brief = ""
    if contentSkin:
      m.generatedTo = title + "_" + os.path.basename(filenameify(m.name)) + ".html"
      generateContent(cfg, m, contentSkin)

  template = kid.Template(file=cfg["html"]["skin"] + os.sep + "toc.xml",**out)
  #print str(template)
  fname = args[0] + ".html"
  f = open("html"+os.sep+fname, "wb")
  f.write(str(template))
  f.close()

  for m in me.sections:
    pass
  return (fname,template)

#</module>
