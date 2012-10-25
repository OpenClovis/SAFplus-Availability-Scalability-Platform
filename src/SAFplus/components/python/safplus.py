import safplusCApi
from amfpy import initializeAmf, finalizeAmf
from aspLog import Log, LogLevelError, LogLevelWarn, LogLevelInfo, LogLevelDebug
import misc
from clAppApi import ClAppApi as SafApp
import clAppApi

import os

# Pull in some environment variables for later use
DEPLOY_DIR = os.getenv("ASP_DIR")
NODE_NAME = os.getenv("NODENAME")
COMP_NAME = os.getenv("ASP_COMPNAME")
MANUAL_START = misc.yesNo2Bool(os.getenv("ASP_WITHOUT_CPM"))


def evalOrExec(cmd,globl,local):
    """I don't care whether it returns a value or not; I just want it to work!"""
    try:
      ret = eval(cmd,globl,local)
      return ret
    except Exception, e:
      exec cmd in globl,local
      return None

def Callback(cmd):
  """This is the entry function for all callbacks coming from the SAFplus infrastructure"""
  # This will log the 'raw' calls from the SAFplus layer to you
  # app.Log("SAFplus callback: " + str(cmd))
  return evalOrExec(cmd,globals(),{"eoApp":clAppApi.app})
