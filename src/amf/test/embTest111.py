import os, sys, time,types, signal, random, datetime
import subprocess
import pdb

if __name__ != '__main__':  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control

import safplusMgtDb
import safplus as sp
import clTest
import amfctrl

modelXML = "SAFplusAmf1Node1SG1Comp.xml"

try: # only set this variable once, not every time reload occurs
  test = SAFplusInitialized  # Will throw an error if this variable does not exist
except:
  print "First run"
  SAFplusInitialized=False

class TestFailed(Exception):
  def __init__(self,reason):
    self.why = reason

def now():
  return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%-S")

def procStatus(pid):
    for line in open("/proc/%d/status" % pid).readlines():
        if line.startswith("State:"):
            return line.split(":",1)[1].strip().split(' ')[0]
    return None

def startupAmf(tgtDir,outfile=None,infile="/dev/null"):
    if type(outfile) is types.StringType:
      outfile = open(outfile,"w+")
    if type(infile) is types.StringType:
      infile = open(infile,"r")
    # delete the old database
    try:
      os.remove(tgtDir + "/bin/safplusAmf.db")
    except OSError, e: pass # ok file does not exist

    cwd = os.path.abspath(tgtDir + "/bin")

    # now write the new data
    db = safplusMgtDb.PyDBAL(cwd + "/safplusAmf") # Root of Log service start from /log ->  docRoot= "version.log_BootConfig.log"
    db.LoadXML(modelXML)
    db.Finalize()

    # now start safplus_amf
    bufferingSize = 64  # 4096
    cwd = os.path.abspath(tgtDir + "/bin")
    args = (cwd + "/safplus_amf",)
    print "Environment is: ", os.environ
    amf = subprocess.Popen(args,bufferingSize,executable=None, stdin=infile, stdout=outfile, stderr=outfile, preexec_fn=None, close_fds=True, shell=False, cwd=cwd, env=None, universal_newlines=False, startupinfo=None, creationflags=0)
    print "SAFplus AMF is running pid [%d]" % amf.pid
    time.sleep(5)
    if amf.poll() != None:  # If some kind of config error then AMF will quit early
      time.sleep(20)
      pdb.set_trace()
      raise TestFailed("AMF quit unexpectedly")
      
    time.sleep(10)  # TODO: without a sleep here, process is hanging waiting for mgt checkpoint gate.  I think that this process is being chosen as active replica (which should be ok) but for some reason is not working

def connectToAmf():
  global SAFplusInitialized
  if not SAFplusInitialized:
    svcs = sp.Libraries.MSG | sp.Libraries.GRP | sp.Libraries.MGT_ACCESS
    sic = sp.SafplusInitializationConfiguration()
    sic.port = 52
    SAFplusInitialized = True
    sp.Initialize(svcs, sic)
    sp.logSeverity = sp.LogSeverity.DEBUG
    

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
      raise TestFailed(now() + ": Failover did not work: active %s, standby %s waited %d seconds" % (active,standby,maxTime))
    clTest.testSuccess("work assigned")
    return (active,standby)
  

def mgtHammer():
  for i in range(1,10000):
      (active, standby) = amfctrl.activeStandby("si")

class ZombieException(Exception):
  pass

def main(tgtDir):
    os.environ["ASP_NODENAME"] = "node0"
    try:
      amfpid = int(subprocess.check_output(["pgrep","-n", "safplus_amf"]).strip())
      status = procStatus(amfpid)
      if status == "Z": # zombie
        raise ZombieException()
      print "AMF is already running as process [%d]" % amfpid
    except (subprocess.CalledProcessError,ZombieException):
      startupAmf(tgtDir,"amfOutput.txt")  
      time.sleep(3) # Give the AMF time to initialize shared memory segments
    connectToAmf()
    print "AMF started"
    amfctrl.displaySgStatus("sg0")

    # mgtHammer()

    clTest.testCase("AMF-FAL-OVR.TC001: Component fail over",AmfFailComp)
    clTest.finalize();
    sp.Finalize()

def AmfFailComp():
    # Make sure sg is fully up
    (active, standby) = waitForSiRecovery("si",60)
    if not active:
      raise clTest.Malfunction(now() + "Initial conditions incorrect: No active was chosen")
    if not standby:
      raise clTest.Malfunction(now() + "Initial conditions incorrect: No standby was chosen")
    count = 0
    for i in range(1,5):
      print now() +":" + str(count) + " Killing Active Component"
      for j in range(1,3):
        # grab a random process from the active list and kill it.
        signalRandomCompInSi("si","active")
        # wait for AMF to react
        time.sleep(5)
        waitForSiRecovery("si")
        time.sleep(5)
        count += 1
      print now() + ":" + str(count) + " Killing Standby Component"
      for j in range(1,3):
        # grab a random process from the active list and kill it.
        signalRandomCompInSi("si","standby")
        # wait for AMF to react
        time.sleep(5)
        waitForSiRecovery("si")
        time.sleep(5)
        count += 1
      amfctrl.displaySgStatus("sg0")


if __name__ == '__main__':  # called from the target "test" directory
  main("..")

def Test():  # called from inside the source tree
  pdb.set_trace()
  main("..")
  #sys.path.append("../../python")  # Point to the python code, in source control
  #main("../../target/i686/3.13.0-37-generic")
