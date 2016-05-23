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

    count = 0
    while 1:
      count += 1
      time.sleep(1)
      ps = sp.mgt.get("/safplusAmf/Component/exampleSafApp_in_tc001Su0/presenceState")
      if ps == "instantiated":
        clTest.testSuccess("AMF-DYN-SG.TC002: App is instantiated")
        # todo check process ID
        break 
      elif count == 5:
        clTest.testFailed("AMF-DYN-SG.TC002: App is instantiated","comp state is [%s]" % ps)
        

    clTest.finalize();
    sp.Finalize()


if __name__ == '__main__':  # called from the target "test" directory
  main("..")

def Test():  # called from inside the source tree
  main("..")
  #sys.path.append("../../python")  # Point to the python code, in source control
  #main("../../target/i686/3.13.0-37-generic")
