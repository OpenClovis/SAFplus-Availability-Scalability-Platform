import pdb
import os, sys, time,types, signal, random, datetime
import subprocess

# import safplusMgtDb

if __name__ != '__main__':  # called from inside the source tree
  sys.path.append("../../python")  # Point to the python code, in source control

import safplus as sp
import clTest
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

def procStatus(pid):
    for line in open("/proc/%d/status" % pid).readlines():
        if line.startswith("State:"):
            return line.split(":",1)[1].strip().split(' ')[0]
    return None


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

def AmfCreateApp(prefix, compList, nodes):
  sg = "%sSg" % prefix
  entities = { 
    # "Component/dynComp0" : { "maxActiveAssignments":4, "maxStandbyAssignments":2, "serviceUnit": "dynSu0", "instantiate": { "command" : "../test/exampleSafApp dynComp", "timeout":60*1000 }},   
    "ServiceGroup/%s" % sg : { "adminState":"off", "preferredNumActiveServiceUnits":1,"preferredNumStandbyServiceUnits":0 },
    "ServiceInstance/%sSi" % prefix : { "serviceGroup": sg }
  }

  count = 0
  sus = []
  for n in nodes:
    suname = "%sSu%d" % (prefix,count)
    comps = []
    for c in compList:
      name = os.path.basename(c.split(" ")[0])
      cname = "%s_in_%s" % (name,suname)
      entities["Component/%s" % cname] = { "maxActiveAssignments":1, "maxStandbyAssignments":1, "serviceUnit": suname, "instantiate": { "command" : c, "timeout":15*1000 }}
      comps.append(cname)   

    entities["ServiceUnit/%s" % suname] = { "components":",".join(comps),"node":n, "serviceGroup": sg }
    sus.append(suname)
    count += 1

  entities["ServiceGroup/%s" % sg]["serviceUnits"] = ",".join(sus)
  pdb.set_trace()
  amfctrl.commit(entities)

  # clTest.testSuccess("")


def main(tgtDir):
    os.environ["ASP_NODENAME"] = "node0"

    amfpid = int(subprocess.check_output(["pgrep","-n", "safplus_amf"]).strip())
    status = procStatus(amfpid)
    if status == "Z": # zombie
        raise ZombieException()
    print "AMF running as process [%d]" % amfpid

    connectToAmf()
   
    #amfctrl.displaySgStatus("sg0")
    # Create this node if it is missing
    node = sp.mgtGet("/safplusAmf/Node/node0")
    if node == "":
      sp.mgtCreate("/safplusAmf/Node/node0")

    clTest.testCase("AMF-DYN-SG.TC001: Create app",AmfCreateApp("tc001",["../test/exampleSafApp tc001"],["node0"]))
    sp.mgtSet("/safplusAmf/ServiceGroup/tc001Sg/adminState","on")

    pdb.set_trace()
    while 1:
      time.sleep(1)
      ps = sp.mgt.get("/safplusAmf/Component/exampleSafApp_in_tc001Su0/presenceState")
      if ps == "instantiated":
        break 

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
  main("..")
  #sys.path.append("../../python")  # Point to the python code, in source control
  #main("../../target/i686/3.13.0-37-generic")
