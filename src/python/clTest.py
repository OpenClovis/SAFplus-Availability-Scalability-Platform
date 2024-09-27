import pdb
import types
import datetime
import inspect
import os

resultsLog = None

printIndent = 0
caseName = ""
passed = 0
failed = 0
malfunction = 0

tcPassed = 0
tcFailed = 0
tcMalfunction = 0

class Malfunction(Exception):
  pass

def initialize():
  global printIndent, passed, failed, malfunction, tcFailed, tcPassed, tcMalfunction, caseName
  openResultsLog()
  printIndent = 0
  caseName = ""
  passed = 0
  failed = 0
  malfunction = 0

  tcPassed = 0
  tcFailed = 0
  tcMalfunction = 0
  
def finalize():
  global printIndent, passed, failed, malfunction, tcFailed, tcPassed, tcMalfunction, caseName, resultsLog
  testPrint("Test completed.  Cases:  [%d] passed, [%d] failed, [%d] malfunction.\n" % (tcPassed, tcFailed, tcMalfunction),-1);
  resultsLog.close()
  resultsLog = None

def openResultsLog():
  global resultsLog
  if not resultsLog:
    resultsLog = open("/var/log/testresults.log","a")

def now():
  return datetime.datetime.now().strftime("%b %d %H:%M:%S")
  
def fileLineFunction(back):
  if back<0: back*=-1
  back+=1 # to unwind this function
  fr = inspect.currentframe()
  for i in range(0,back+1):  
    if not fr.f_back: break  # Abort at the top level if the user wants to rewind too far
    fr = fr.f_back
  # m = inspect.getmodule(fr)
  return (fr.f_code.co_filename,fr.f_lineno,fr.f_code.co_name) 


def testPrint(s,flf=None):
  global resultsLog, printIndent
  openResultsLog()
  backup = -1
  if type(flf) == int:  # You can pass either -N to mean back up that many frames or a tuple
    backup = flf
    flf = None
  if not flf:
    flf=fileLineFunction(backup)
  resultsLog.write("[" + now() + "]:  (" + str(os.getpid()) + "), at " + flf[0] + ":" + str(flf[1]) + " (" + flf[2] + "): " + " "*printIndent + str(s) + "\n")

def testCaseStart(s,frame=0):
  global printIndent, passed, failed, malfunction, caseName
  printIndent+=2
  caseName = s
  passed = 0
  failed = 0
  malfunction = 0
  testPrint("Test case started [" + s + "]:",frame-1)
  
def testCaseEnd(synopsis,frame=0):
  global printIndent, passed, failed, malfunction, tcFailed, tcPassed, tcMalfunction, caseName
  s = "Test case completed [%s]. Subcases: [%d] passed, [%d] failed, [%d] malfunction.\nSynopsis:\n" % (caseName, passed, failed, malfunction)
  if failed: tcFailed+=1
  elif malfunction: tcMalfunction+=1
  elif passed: tcPassed+=1
  else: tcMalfunction+=1 # Its a malfunction if no tests were run inside a test case
  testPrint(s,frame-1)
  caseName = None

def testCase(name,test):
  testCaseStart(name,-1);
  try:
    test
  except Exception as e:
    testFailed(name,"exception: " + str(e))
  testCaseEnd("",-1)

def testSuccess(name):
  test(name, True, None)

def testFailed(name, more=None):
  test(name, False, more)


def test(name, predicate, errorPrintf):
  global printIndent, passed, failed, malfunction, tcFailed, tcPassed, tcMalfunction
  reason = ""
  if type(predicate) is types.FunctionType:
    try:
      predicate = predicate()
    except Malfunction as e:
      reason = str(e)
      malfunction += 1
      return
    except e:
      predicate = False
      reason = "Exception: " + str(e)
  if predicate:
    passed +=1
  else:
    failed +=1
    ep = errorPrintf
    if type(errorPrintf) is types.FunctionType:
      try:
        ep = errorPrintf()
      except e:
        ep = "info function failed: " + str(e)
    s = "%s: Failed.  Error info: %s %s" % (name, reason, str(ep))
