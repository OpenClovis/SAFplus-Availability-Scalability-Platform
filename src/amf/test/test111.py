import os, sys, time,types, pdb, signal, random, datetime
import subprocess

import dbalpy

if __name__ != '__main__':  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control

import safplus as sp
import amfctrl

modelXML = "SAFplusAmf1Node1SG1Comp.xml"

try: # only set this variable once, not every time reload occurs
  test = SAFplusInitialized  # Will throw an error if this variable does not exist
except:
  print "First run"
  SAFplusInitialized=False

class TestFailed:
  def __init__(self,reason):
    self.why = reason

def now():
  return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%-S")

def startupAmf(tgtDir,outfile=None,infile="/dev/null"):
    if type(outfile) is types.StringType:
      outfile = open(outfile,"w+")
    if type(infile) is types.StringType:
      infile = open(infile,"r")
    # delete the old database
    try:
      os.remove(tgtDir + "/bin/SAFplusAmf.idx")
    except OSError, e: pass # ok file does not exist
    try:
      os.remove(tgtDir + "/bin/SAFplusAmf.db")
    except OSError, e: pass # ok file does not exist

    # now write the new data
    db = dbalpy.PyDBAL(tgtDir + "/bin/SAFplusAmf") # Root of Log service start from /log ->  docRoot= "version.log_BootConfig.log"
    db.LoadXML(modelXML)
    db.Finalize()

    # now start safplus_amf
    bufferingSize = 64  # 4096
    cwd = os.path.abspath(tgtDir + "/bin")
    args = (cwd + "/safplus_amf",)
    cwd = os.path.abspath(tgtDir + "/bin")
    amf = subprocess.Popen(args,bufferingSize,executable=None, stdin=infile, stdout=outfile, stderr=outfile, preexec_fn=None, close_fds=True, shell=False, cwd=cwd, env=None, universal_newlines=False, startupinfo=None, creationflags=0)
    time.sleep(5)

def connectToAmf():
  global SAFplusInitialized
  if not SAFplusInitialized:
    svcs = sp.Libraries.MSG | sp.Libraries.GRP | sp.Libraries.MGT_ACCESS
    sic = sp.SafplusInitializationConfiguration()
    sic.port = 52
    SAFplusInitialized = True
    sp.Initialize(svcs, sic)
    

def signalRandomCompInSi(si,role="active",sig=signal.SIGKILL):
    (active, standby) = amfctrl.activeStandby(si)
    if role == "active":
      if not active:
        raise TestFailed("No active was chosen")
      suList = active
    elif role == "standby":
      if not standby:
        raise TestFailed("No standby was chosen")
      suList = standby
    else:
      assert(0) # Bad inputs
  
    # grab a random process from the active list and kill it.
    su = random.choice(amfctrl.getEntity(suList))  # first get a random SU
    comps = amfctrl.getEntity(amfctrl.csv2List(su.components.data_)) # then get a random comp out of it
    comp = random.choice(comps)
    print now() + ": Killing [%s] pid [%s]" % (comp.name.data_, str(comp.processId.data_))
    os.kill(comp.processId.data_, sig)

def waitForSiRecovery(si,maxTime=20):
    # wait until the failover happens
    count = 0
    while count < maxTime:
      (active, standby) = amfctrl.activeStandby(si)
      if active and standby: break
      time.sleep(1)
      count+=1
    if count<maxTime:
      print now() + ": New active %s, new standby %s" % (active, standby)
    else:
      raise TestFailed(now() + ": Failover did not work: active %s, standby %s" % (active,standby))
    return (active,standby)
  

def mgtHammer():
  for i in range(1,10000):
      (active, standby) = amfctrl.activeStandby("si")


def main(tgtDir):
    try:
      amfpid = subprocess.check_output(["pidof","safplus_amf"])
      print "AMF is already running as process [%d]" % int(amfpid.strip()) 
    except subprocess.CalledProcessError:
      startupAmf(tgtDir,"amfOutput.txt")  

    connectToAmf()

    amfctrl.displaySgStatus("sg0")

    mgtHammer()

    # Make sure sg is fully up
    (active, standby) = waitForSiRecovery("si",60)
    if not active:
      raise TestFailed(now() + ": No active was chosen")
    if not standby:
      raise TestFailed(now() + ": No standby was chosen")

    for i in range(1,2):
      print now() +": Killing Active Component"
      for i in range(1,10):
        # grab a random process from the active list and kill it.
        signalRandomCompInSi("si","active")
        # wait for AMF to react
        time.sleep(5)
        waitForSiRecovery("si")
      print now() + ": Killing Standby Component"
      for i in range(1,10):
        # grab a random process from the active list and kill it.
        signalRandomCompInSi("si","standby")
        # wait for AMF to react
        time.sleep(5)
        waitForSiRecovery("si")


if __name__ == '__main__':  # called from the target "test" directory
  main("..")

def Test():  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control
  main("../../target/i686/3.13.0-37-generic")
