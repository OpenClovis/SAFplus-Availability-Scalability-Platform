import xml.dom.minidom as minidom
import xml.parsers.expat
import pdb
import re
from types import *
from dot import *
from decimal import *

class VersionMismatch(Exception):
  def __init__(self,expected, actual, string):
    self.expected = expected
    self.actual   = actual
    self.string   = string

class BadConfigFile(Exception): pass
  

vparse = re.compile("(\d+)\.(\d+)\.(\d+)\.(\d+)")

def decode_file(file, type=None, ver=None):
  """The version (ver) should be a quadruple (Major, minor, patch, bugfix)
     the file must match the major and minor version exactly
     and be greater than the patch version (i.e. patches must be backwards compat)
     bugfix can be anything (i.e. must be completely compatible)

     If you do not pass a type or a version, then version checking will not be
     done.
  """
  f = open(file, "r")
  data = f.read()
  f.close()
  try:
    dom = decode_string(data)
  except xml.parsers.expat.ExpatError, e:
    raise BadConfigFile("Config file [%s] is bad, %s" % (file,str(e)))
    
  if type:    
    if ver:
      fver = vparse.match(dom[type].ver)
      fver = [int(x) for x in fver.groups()]
      if ver[0] != fver[0] or ver[1] != fver[1]:
        raise VersionMismatch(ver,fver,"Major or minor version do not match")
      if fver[2] > ver[2]:
        raise VersionMismatch(ver,fver,"Patch version is larger then my version")

    return dom[type]
  
  else:
    return dom


def decode_string(str):
  dom = minidom.parseString(str)
  return decode_dom(dom)

def insertTag(d,key,val):
      # Turn strings-of-types into the actual type
      try:
        val = int(val)
      except (TypeError, ValueError, AttributeError):
        try:
          val = Decimal(val)          
        except (InvalidOperation, TypeError, ValueError, AttributeError):
          pass

      if d.has_key(key):
        if type(d[key]) is ListType:
          d[key].append(val)
        else:
          d[key]=[d[key],val]
      else:
        d[key] = val


def decode_dom(dom):
  d = {}
  str = ""
  nodelst = []

  try:
    if dom._attrs.has_key('value'):
      str += dom._attrs['value'].nodeValue
    elif len(dom._attrs.items())>0:
      for (k, v) in dom._attrs.items():
        k = k.encode('utf8')
        val = v.nodeValue.encode('utf8')
        nodelst.append((k,val))
  except AttributeError,e:  # No attributes possible on this dom element
    pass 

  if len(dom.childNodes)>0:
   for n in dom.childNodes:
    if n.nodeType == minidom.Comment.nodeType: # Skip all comments
      pass
    elif n.__dict__.has_key('nodeValue'):
      str += n.nodeValue.strip()
    else:
      key = n.tagName.encode('utf8')      
      nodelst.append((key,decode_dom(n)))
      
  else:
    if dom.__dict__.has_key('nodeValue'):
      str+= dom.nodeValue.strip()


  if len(nodelst)==0: return str
  else:
    if len(str):
      nodelst.append(('value',str))
    for (k,v) in nodelst:
      insertTag(d,k,v)
        
  return Dot(d)



def Test():

  t9 = '<t9 c="3"><a b="1"/><a b="2"/></t9>'
  tmp = decode_string(t9)
  print t9,"\n",tmp

  t9 = '<t9><a b="1"/><a b="2"/></t9>'
  print t9, decode_string(t9)


  t6 = "<a><b>1</b></a>"
  print t6, decode_string(t6)

  t1 = "<a>1</a>"
  print t1, decode_string(t1)

  t2 = "<a value='1' />"
  print t2, decode_string(t2)

  t3 = "<a><b>1</b><c value='2' /></a>"
  print t3, decode_string(t3)

  t4 = "<a b='1' c='2' />"
  print t4, decode_string(t4)

  t5 = "<a>\n\n<b>\n  1\n</b>\n\n<c value='2' /></a>"
  print t5, decode_string(t5)

  t6="""<a value='1' />
  <!-- This is a comment! -->
  """
  print t6, decode_string(t6)

  # Same-named child test
  t7 = '<foo a="1" ><a value="2"/><a>3</a></foo>'
  print t7, decode_string(t7)

  t8 = '<foo a="1" b="5" ><a value="2"/><b>4</b><a>3</a></foo>'
  print t8, decode_string(t8)


def TestVer():
  print decode_file('testcfg.xml','model_config',(1,2,3,4))

  try:
    print decode_file('testcfg.xml','model_config',(2,3,4,4))
    print "failed 2"
  except VersionMismatch:
    pass

  try:
    print decode_file('testcfg.xml','model_config',(1,2,5,6))
  except VersionMismatch:
    print "failed 3"
    pass

  try:
    print decode_file('testcfg.xml','model_config',(1,2,5,1))
  except VersionMismatch:
    print "failed 3"
    pass
   

if __name__ == '__main__':
    Test()
