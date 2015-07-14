import os, sys, time,types, pdb, signal, random
import subprocess

import dbalpy

if __name__ != '__main__':  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control

import SAFplus as sp
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
    svcs = sp.Libraries.MSG | sp.Libraries.GRP
    sic = sp.SafplusInitializationConfiguration()
    sic.port = 52
    SAFplusInitialized = True
    sp.Initialize(svcs, sic)
    

def main(tgtDir):
    # startupAmf(tgtDir,"amfOutput.txt")  
    connectToAmf()

    amfctrl.displaySgStatus("sg0")
    (active, standby) = amfctrl.activeStandby("si")
    if not active:
      raise TestFailed("No active was chosen")
    if not standby:
      raise TestFailed("No standby was chosen")

    # grab a random process from the active list and kill it.
    activeSu = random.choice(amfctrl.getEntity(active))
    activeComps = amfctrl.getEntity(amfctrl.csv2List(activeSu.components.data_))
    comp = random.choice(activeComps)
    print "Killing [%s] pid [%s]" % (comp.name.data_, str(comp.processId.data_))
    os.kill(comp.processId.data_, signal.SIGKILL)

    # wait for AMF to react
    time.sleep(5)

    # wait until the failover happens
    count = 0
    while count < 10:
      (active, standby) = amfctrl.activeStandby("si")
      if active and standby: break
      time.sleep(2)
      count+=1
    if count<10:
      print "New active %s, new standby %s" % (active, standby)
    else:
      raise TestFailed("Failover did not work: active %s, standby %s" % (active,standby))

if __name__ == '__main__':  # called from the target "test" directory
  main("..")

def Test():  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control
  main("../../target/i686/3.13.0-37-generic")
