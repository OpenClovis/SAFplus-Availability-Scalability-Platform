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

# NOTE the ROW must be shown/hidden and have an ID
def genSymbols(cllist):
  header = ["symbol","class","section","file"]
  body = []
  for (idx,obj,cls,sec,fil) in cllist:
    body.append([obj2tlink(obj,PageLocCenter),obj2tlink(cls,PageLocCenter),obj2tlink(sec,PageLocCenter),obj2tlink(fil,PageLocCenter)])
  # parenttLink(obj,TagSection,PageLocCenter),parenttLink(obj,TagFile,PageLocCenter
  grid = GridFromList(header, body )
  for (idx,obj,cls,sec,fil) in cllist:
    row = grid.row(idx+1)  # +1 skips the table header
    row.id = "e%d" % idx
    row.forceId = True

  #grid.RowBackground(Color(250,250,100),[Color(200,200,200),Color(240,240,240)])
  grid.RowAttrs({"class":"header indexHeaderRow"},[{"class":"rowA indexRowA"},{"class":"rowB indexRowB"}])
  return grid

def updateWordDict(dct,desc,ref):
  """?? Given a dictionary of words, update it with all the words in 'desc' with the reference 'ref'
  <arg name='dct'>The word dictionary</arg>
  <arg name='desc'>A string of words to add to the dictionary</arg>
  <arg name='ref'>The reference to connect these words to</arg>
  """
  pct = '!"#$%&\'()*+,-./:;<=>?@[\\]^`{|}~'

  d = str(desc).translate(string.maketrans(pct," " * len(pct)))
  d = d.lower()
  words = d.split()
  #print words
  for word in words:
    if not dct.has_key(word):
      dct[word] = {}
    if not dct[word].has_key(ref):
      dct[word][ref] = 1
    else:
      dct[word][ref] = dct[word][ref] + 1


def generate(objs,cfg,args,tagDict):

  objlst = []
  wordDict = {}
  idx = 0
  for obj in objs.walk():
    if isInstanceOf(obj,MicroDom):
      if obj.tag_ in ConstructTags or obj.tag_ is TagField:  # Only pick out certain items for the index
        if obj.attributes_.has_key(AttrName):
          # construct the line that will appear as a search result
          c = obj.findParent(TagClass)
          if c: c = c[0]
          else: c=None
          s = obj.findParent(TagSection)
          if s: s = s[0]
          else: s=None
          f = obj.findParent(TagFile)
          if f: f = f[0]
          else: f=None

          #objlst[idx] = (obj,c,s,f)
          objlst.append((idx,obj,c,s,f))

          # construct the search dictionary
          for child in obj.walk():
            try:
              updateWordDict(wordDict,child.data_,idx)
            except AttributeError:
              pass
            try:
              updateWordDict(wordDict,child.brief,idx)
            except AttributeError:
              pass
            try:
              updateWordDict(wordDict,child.desc,idx)
            except AttributeError:
              pass
            try:
              updateWordDict(wordDict,child.name,idx)
            except AttributeError:
              pass


          idx += 1


  #objlst.sort(lambda x,y: cmp(x[0].name,y[0].name))
  #pdb.set_trace()
  mv = genSymbols(objlst)
  

  hdr = VSplit([resize(2,"Search")])
  ctr = HSplit([BR,mv])

  me = Dotter()
  me.searchDict = str(wordDict)
  me.indexEntities = str(mv)

  template = kid.Template(file="docskin/search.xml",me=me)
  #print str(template)
  fname = "search.html"
  f = open("html"+os.sep+fname, "wb")
  f.write(str(template))
  f.close()

  jstpl = kid.Template(file="doclayout/search.js.kid",me=me)
  fname = "search.js"
  f = open("html"+os.sep+fname, "wb")
  ser = kid.PlainSerializer()
  f.write(jstpl.serialize(output=ser))
  f.close()
   

  #page = [hdr,ctr]
  #WriteFile(FilePrefix + fname,page,HtmlFragment())
 
  return (fname,template)

#</module>
