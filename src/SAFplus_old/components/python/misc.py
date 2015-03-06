"""@namespace misc
Miscellaneous routines including logging, pretty printers, and enhanced functionality wrappers for some standard library type functions.
"""

import inspect
import types
import sys
import time
import threading
from string import Template
#NOTE DO NOT IMPORT ASP: put asp using routines in aspmisc.py

## @var ContextUndefined
# Undefined error context
ContextUndefined = None

## @var ContextProgramReturnCode
# The exception code is a program's return code
ContextProgramReturnCode = "program return code"

## @var ContextErrno
# The exception code is an errno (see errno.h system header file)
ContextErrno = "errno"

## @var ContextSignal
# The exception code is a signal (see signal.h system header file
ContextSignal = "signal"

def yesNo2Bool(yn):
  """Convert a string to a boolean.
     If yn is a string, this routine will attempt to convert all case variations of "true", "yes", "y" or "1" to True and all variations of "false", "no" "n" or "0" to False. 
     If yn is not a string, it will be cast to a boolean and returned.
     @param yn string to convert
  """
  if type(yn) in types.StringTypes:
    lwr = yn.lower()
    if lwr == "y" or lwr == "yes" or lwr == "true" or lwr == 't' or lwr == "1": return True
    if lwr == "n" or lwr == "no" or lwr == "false" or lwr == 'f' or lwr == "0": return False 
  if yn: return True
  return False


def duration2ms(t):
  """Converts a string describing a time interval in a number of milliseconds.
     The string can have the format '# unit'. (ex: 5 sec).  If no unit is specified, milliseconds is assumed
     Acceptable units are all case or plural variations of 
     week day hour hr min sec ms millisec
     @param t A time interval  Can be an integer, float or string.  
  """
  if type(t) is types.IntType:
    return t
  if type(t) is types.FloatType:
    return int(t)
  if type(t) in types.StringTypes:
    if ":" in t:
      split = t.split(":")
      if len(split) == 2:
        (min,sec) = split
        return int(((float(min)*60) + float(sec))*1000)
      elif len(split) == 3:
        (hour, min,sec) = split
        return int(((float(hour)*60*60)+(float(min)*60) + float(sec))*1000)
    else: # Number unit format, i.e. "5 sec"
      split = t.split()
      if len(split) == 1:  # No unit specified, assume ms
        return float(split[0])
      (num,unit) = split
      ret = float(num)
      unit = unit.lower()
      if ("ms" in unit) or ("millisec" in unit):
        return int(ret)
      elif "sec" in unit:
        return int(ret*1000)
      elif "min" in unit:
        return int(ret*(60*1000))
      elif ("hour" in unit) or ("hr" in unit):
        return int(ret*(60*60*1000))
      elif "day" in unit:
        return int(ret*(24*60*60*1000))
      elif "week" in unit:
        return int(ret*(24*60*60*1000))
  raise ValueError("%s is not understood as a duration" % t)


def List2Str(lst):
  """Helper function that converts a list to a comma separated string"""
  return ", ".join(lst)

def Sort(x):
  """Helper function that does a inplace sort inline (i.e. x is returned).  This is convenient for use in web template files.
  Note that a copy is NOT made.  This function modifies its parameter!

  @param x container to be sorted
  @returns x
  """
  x.sort()
  return x

class Error(Exception):
  """Base error object for the asppybinding library"""
  def __init__(self,reasonStr, errCode=0,errContext=None):
    """Constructor
    @param reasonStr  A human readable string describing the problem
    @param errCode    The ASP error code
    @param errContext An object or container of objects that relate to this error
    """
    Exception.__init__(self, reasonStr)
    self.errCode = errCode
    self.errContext = errContext

class Unimplemented(Exception):
  """An exception that indicates functionality is not ready"""
  pass

class TODO(Exception):
  """An exception that indicates there is more work to be done here"""
  pass

def Tenglish(s,**kw):
  """Default english internationalization substitution function (there's nothing to do)"""
  return Template(str(s)).substitute(**kw)

lcl = threading.local()
lcl.T = Tenglish

def setT(fn):
  """Set the default translation function
  @param fn The translation function to be used.  Here is a template:
  @code
    def Tenglish(s,**kw):
      return Template(str(s)).substitute(**kw)
  """
  lcl.T = fn

def T(s,**kw):
  """Call this function to do a translation
  @param 
  """
  fn = lcl.__dict__.get("T",Tenglish)
  return fn(s,**kw)


def Implements(obj,typ):
  """Returns True if the object implements (is or is derived from) a class
  @param obj The object in question
  @param typ The class type in question
  @returns boolean
  """
  if typ is types.DictType:
    try:
      t = obj.items
      return True
    except AttributeError:
      return False
  else:
    TODO("Implements function type %s" % str(typ))



def thisLine(back=0):
  """Return line number of the nth caller (caller's caller, etc) based on the back argument 
     @param back How far to unwind the stack.  0 means the caller, 1 is the caller's caller, etc.
  """
  frame = inspect.currentframe().f_back
  while back>0:
    frame = frame.f_back
    back -= 1
  return frame.f_lineno

def thisFile(back=0):
  """Return file of the nth caller (caller's caller, etc) based on the back argument
     @param back How far to unwind the stack.  0 means the caller, 1 is the caller's caller, etc.
  """ 
  frame = inspect.currentframe().f_back
  while back>0:
    frame = frame.f_back
    back -= 1
  return frame.f_code.co_filename 

def flatten(listOflists):
  """Turn a list of list of lists,etc into just 1 list.
     flatten([1,2,[3,4],[5,[6],7,[]]]) -> [1, 2, 3, 4, 5, 6, 7]
  """
  ret = []

  # termination case, arg is not a list
  if not type(listOflists) is type(ret):
    return [listOflists]
  # recursive case, it is a list
  for list in listOflists:
    ret += flatten(list)

  return ret

def Tagify(s):
  """Turn a string into something that can be used as an XML attribute"""
  return s.replace(".","_").replace(" ","_")
  
def ziprepeat(xlst,ylst):
  """Does a python "zip" but repeats ylst over again if it is too short  
  """
  yidx=0
  for x in xlst:
    if not ylst: yield (x,None)
    if yidx >= len(ylst): yidx = 0
    yield (x,ylst[yidx])
    yidx += 1
