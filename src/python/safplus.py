# This wrapper around the C code lets us change the organization of the namespace in Python
import pdb
import pySAFplus as sp
import amfctrl
import xml.etree.ElementTree as ET

from pySAFplus import SafplusInitializationConfiguration, Libraries, Initialize, Finalize, logMsgWrite, LogSeverity, HandleType, Handle, WellKnownHandle, Transaction, SYS_LOG, APP_LOG, Buffer, Checkpoint, mgtGet, mgtSet, mgtCreate, mgtDelete, logSeverity, getProcessHandle, amfMgmtInitialize, amfMgmtFinalize, nameInitialize, amfMgmtNodeLockAssignment, amfMgmtSGLockAssignment, amfMgmtSULockAssignment, amfMgmtSILockAssignment, amfMgmtNodeLockInstantiation, amfMgmtSGLockInstantiation, amfMgmtSULockInstantiation, amfMgmtNodeUnlock, amfMgmtSGUnlock, amfMgmtSUUnlock, amfMgmtSIUnlock, amfMgmtNodeRepair, amfMgmtCompRepair, amfMgmtSURepair, amfMgmtNodeRestart, amfMgmtSURestart, amfMgmtCompRestart, grpCliClusterViewGet, amfMgmtSGAdjust, amfMgmtSISwap, amfMgmtCompErrorReport, Recovery, amfMgmtNodeErrorReport, amfMgmtNodeErrorClear, amfMgmtNodeJoin, amfMgmtNodeShutdown, amfNodeRestart, amfMgmtAssignSUtoSI, amfMiddlewareRestart

class Error(Exception):
  pass

class Dotter:
  pass

raiseException = Dotter()

def zz__bootstrap__():
   global __bootstrap__, __loader__, __file__
   import sys, pkg_resources, imp
   __file__ = pkg_resources.resource_filename(__name__,'pySAFplus.so')
   __loader__ = None; del __bootstrap__, __loader__
   imp.load_dynamic("pySAFplus",__file__)
# __bootstrap__()

def mgtGetItem(path,default=raiseException):
  data = mgtGet(path)
  try:
    t = ET.fromstring(data)
    return t.text
  except IndexError, e:
      pdb.set_trace()
      if default is raiseException:
        raise Error("Invalid element")
      else:
        return default

def csv2List(csvString):
  """Convert comma separated values to a Python list"""
  if not csvString.strip(): return []  # turn "" into [], otherwise it is ['']
  return [x.strip() for x in csvString.split(",")]

# The API of this object mirrors that of the CLI
mgt = Dotter()
mgt.get = mgtGetItem
mgt.getInt = lambda s,deflt = raiseException: int(mgtGetItem(s,deflt))
mgt.getFloat = lambda s,deflt = raiseException: float(mgtGetItem(s,deflt))
mgt.set = mgtSet
mgt.getList = lambda s,deflt = raiseException: csv2List(mgtGetItem(s,deflt))
