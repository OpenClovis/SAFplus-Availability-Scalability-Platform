import os.path
import os
from types import *
import inspect
import shutil

def instanceType(instance):
  return hasattr(instance, '__class__') and \
         ('__dict__' in dir(instance) or hasattr(instance, '__slots__'))
  
def isInstanceOf(obj,objType):
  """Is the object an instance (or derived from) of the class objType)?"""
  if instanceType(obj) and objType in inspect.getmro(obj.__class__): return True
  return False

def isTrue(v):
  if v.upper() == "TRUE": return True
  return False

def cSymbolify(s):
  s = s.replace(" ","_")
  s = s.replace("-","_")
  return s

def copyFile(src,dest):
  destdir = os.path.dirname(dest)
  if not os.path.exists (destdir): os.makedirs (destdir)
  shutil.copyfile(src,dest)

def cEnumify(s):
  s = cSymbolify(s)
  s = s.upper()
  return s

def cQuote(s):
  return '"' + s + '"'

# TODO: find all common MIB types and put them in here
m2c = { "DisplayString": "char", "Counter32":"ClUint32T", "Counter64":"ClUint64T", "Gauge32":"ClUint32T", "TruthValue":"ClBoolT","Int32":"ClInt32T","Uint8":"ClUint8T","INTEGER":"ClWordT","OBJECT IDENTIFIER":"OidT"}
def mib2c(s):
  t = m2c.get(s,None)
  if not t is None: return t
  return s

xlat = {"PRIORITY_LOW":"CL_OSAL_THREAD_PRI_LOW","PRIORITY_MEDIUM":"CL_OSAL_THREAD_PRI_MEDIUM","PRIORITY_HIGH":"CL_OSAL_THREAD_PRI_HIGH",True:"CL_TRUE",False:"CL_FALSE"}
def valXlat(s):
  t = xlat.get(s,None)
  if not t is None: return t
  return s

def writeFile(filename,data):
  if os.path.exists(os.path.dirname(filename)) == False: os.makedirs(os.path.dirname(filename))		    
  f = open(filename, "w")
  f.write(data)
  f.close()
  
def Log(s,severity="INFO"):
  print ("\n%s\n" % str(s))

refreshClick =  { "5 sec" : 5000,"10 sec" : 10000,"30 sec" : 30000, "1 min" : 60000, "OFF" : 7*24*60*60*1000}
