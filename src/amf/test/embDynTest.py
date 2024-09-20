import pdb
import os, sys, time,types, signal, random, datetime, errno
import subprocess
import time
import safplusMgtDb

if __name__ != '__main__':  # called from inside the source tree
    sys.path.append("../../python")  # Point to the python code, in source control

import safplus as sp
import clTest
import amfctrl

modelXML = "SAFplusAmf1Node1SG1CompModel.xml"

try: # only set this variable once, not every time reload occurs
    test = SAFplusInitialized  # Will throw an error if this variable does not exist
except:
    print("First run")
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
    numActiveSus = (len(nodes)+1)/2
    numStandbySus = len(nodes) -  numActiveSus

    entities = {
      # "Component/dynComp0" : { "maxActiveAssignments":4, "maxStandbyAssignments":2, "serviceUnit": "dynSu0", "instantiate": { "command" : "../test/exampleSafApp dynComp", "timeout":60*1000 }},
      "ServiceGroup/%s" % sg : { "adminState":"off", "preferredNumActiveServiceUnits":numActiveSus,"preferredNumStandbyServiceUnits":numStandbySus },
      "ServiceInstance/%sSi" % prefix : { "serviceGroup": sg },
      # "ComponentServiceInstance/%sCsi" % prefix : { "serviceInstance": "ServiceInstance/%sSi" % prefix, "name/testKey" : "testVal" }
    }

    count = 0
    sus = []
    for n in nodes:
        suname = "%sSu%d" % (prefix,count)
        comps = []
        for c in compList:
            name = os.path.basename(c.split(" ")[0])
            cname = "%s_in_%s" % (name,suname)
            idx=1
            while cname in comps:
                cname = "%s_in_%s_%d" % (name,suname,idx)
                idx+=1

            entities["Component/%s" % cname] = { "maxActiveAssignments":1, "maxStandbyAssignments":1, "serviceUnit": suname, "instantiate": { "command" : c, "timeout":15*1000 }}
            comps.append(cname)

        entities["ServiceUnit/%s" % suname] = { "components":",".join(comps),"node":n, "serviceGroup": sg }
        sus.append(suname)
        count += 1

    entities["ServiceGroup/%s" % sg]["serviceUnits"] = ",".join(sus)
    amfctrl.commit(entities)

    # clTest.testSuccess("")

def testCreateSingleApp():
    clTest.testCase("AMF-DYN-SG.TC001: Create app",AmfCreateApp("tc001",["../test/exampleSafApp tc001"],["node0"]))
    time.sleep(2)
    sp.mgtSet("/safplusAmf/ServiceGroup/tc001Sg/adminState","on")
    sp.mgtSet("/safplusAmf/ServiceUnit/tc001Su0/adminState","on")
    time.sleep(2)
    count = 0
    compPfx = "/safplusAmf/Component/exampleSafApp_in_tc001Su0/"
    while 1:
        count += 1
        time.sleep(2)
        ps = sp.mgt.get(compPfx + "presenceState")
        if ps == "instantiated":
            clTest.testSuccess("AMF-DYN-SG.TC002: App is instantiated")
            procId = 0
            pstatus = "N/A"
            try: # todo check process ID
                procId = sp.mgt.getInt(compPfx + "processId")
                pstatus = procStatus(procId)
                if pstatus in ["R","S","D"]:
                    clTest.testSuccess("AMF-DYN-SG.TC003: Instantiated app is running")
                else:  # Are there other valid process states?
                    clTest.testFailed("AMF-DYN-SG.TC003: Instantiated app is running","pid [%s] status [%s]" % (procId, pstatus))
            except Exception as e:
                clTest.testFailed("AMF-DYN-SG.TC003: Instantiated app is running","pid [%s] status [%s]" % (procId, pstatus))
            break
        elif count == 5:
            clTest.testFailed("AMF-DYN-SG.TC002: App is instantiated","comp state is [%s]" % ps)
            raise TestFailed("app never instantiated")

def testCreateNsuMcomp(prefix, numSu,numComp):
    compLst = [ "../test/exampleSafApp %s_c%d" % (prefix,x) for x in range(0,numComp)]
    nodeLst = ["node0"] * numSu
    sg = prefix + "Sg"
    clTest.testCase("AMF-DYN-SG.TC001: Create %d su %d comps" % (numSu, numComp),AmfCreateApp(prefix,compLst,nodeLst))
    time.sleep(7)
    sp.mgtSet("/safplusAmf/ServiceGroup/%sSg/adminState" % prefix,"on")
    for x in range(0,numComp):
        sp.mgtSet("/safplusAmf/ServiceUnit/%sSu%d/adminState" % (prefix,x),"on")

    for compNum in range(0,numComp):
        count = 0
        compPfx = "/safplusAmf/Component/exampleSafApp_in_%sSu%d/" % (prefix,compNum)
        while 1:
            count += 1
            time.sleep(2)
            ps = sp.mgt.get(compPfx + "presenceState")
            if ps == "instantiated":
                clTest.testSuccess("AMF-DYN-SG.TC022: App is instantiated")
                procId = 0
                pstatus = "N/A"
                try: # todo check process ID
                    procId = sp.mgt.getInt(compPfx + "processId")
                    pstatus = procStatus(procId)
                    if pstatus in ["R","S","D"]:
                        clTest.testSuccess("AMF-DYN-SG.TC023: Instantiated app is running")
                    else:  # Are there other valid process states?
                        clTest.testFailed("AMF-DYN-SG.TC023: Instantiated app is running","pid [%s] status [%s]" % (procId, pstatus))
                except Exception as e:
                    clTest.testFailed("AMF-DYN-SG.TC023: Instantiated app is running","pid [%s] status [%s]" % (procId, pstatus))
                break
            elif count == 5:
                clTest.testFailed("AMF-DYN-SG.TC022: App is instantiated","comp state is [%s]" % ps)
                raise TestFailed("app never instantiated")





def waitForInstantiated(ent, loops=10):
    count = 0
    while count < loops:
        count += 1
        ps = sp.mgt.get(ent + "/presenceState")
        if ps == "instantiated":
            return True
        time.sleep(2)
    return False

def waitForPresenceState(ent, state, loops=10):
    count = 0
    while count < loops:
        count += 1
        ps = sp.mgt.get(ent + "/presenceState")
        if ps == state:
            return True
        time.sleep(2)
    return False

def findServiceUnitsOf(entity):
    """give a service group or service unit, and get a list of its service units"""
    if "ServiceGroup" in entity:
        sus = sp.mgt.getList(entity + "/serviceUnits")
    else: sus = [entity]
    return sus

def findComponentsOf(entity):
    """give a service group or service unit (full entity path), and get a list of its components"""
    if "ServiceGroup" in entity:
        sus = sp.mgt.getList(entity + "/serviceUnits")
    else: sus = [entity]

    comps = []
    for su in sus:
        comps += sp.mgt.getList(su + "/components")
    return comps

def testStartStop(entPfx):
    """This test will look at the current state and run one start/stop or stop/start cycle based on it"""
    started = False
    stopped = False
    procIdList = []
    while not (started and stopped):
        adm = sp.mgt.get(entPfx + "/adminState")
        if adm == "on":
                # First check that its entirely started
            sus = findServiceUnitsOf(entPfx)
            for su in sus:
                if not waitForInstantiated(su): raise TestFailed("[%s] never became uninstantiated")
            # Including checking processIDs -- note this will ONLY work for single node tests
            comps = findComponentsOf(entPfx)
            for compPfx in comps:
                procId = sp.mgt.getInt(compPfx + "/processId")
                procIdList.append(procId)
                pstatus = procStatus(procId)
                if not pstatus in ["R","S","D"]:
                    raise TestFailed("Process in bad state")
            started = True
            # Second, turn it off
            sp.mgtSet(entPfx + "/adminState","off")
        elif adm == "off":
            sus = findServiceUnitsOf(entPfx)
            for su in sus:
                waitForPresenceState(su,"uninstantiated")
            if procIdList:  # If we have a bunch of process Ids from the start, lets make sure they do not exist
                time.sleep(2)  # After the component deregisters, it can take any amount of time to actually quit.
                try:
                    pstatus = procStatus(procId)
                    if pstatus in ["R","S","D"]:
                        raise TestFailed("Process [%d] did not stop" % procId)
                except IOError as e:
                    if e.errno == errno.ENOENT: pass # No such file or directory -- Great! the process has already quit.
                    else: raise

            stopped = True
            sp.mgtSet(entPfx + "/adminState","on")
        else:
            raise TestFailed("Cannot get adminState from [%s]. Got [%s]" % (entPfx,adm))


def startupAmf(tgtDir,outfile=None,infile="/dev/null"):
    #if type(outfile) is bytes:
    #    outfile = open(outfile,"w+")
    #if type(infile) is bytes:
    #    infile = open(infile,"r")
    # delete the old database

    if outfile:
        outfileFp = open(outfile,"w+")
    else:
        outfileFp = None

    if infile:
        infileFp = open(infile,"w+")
    else:
        infileFp = None

    try:
        os.remove(tgtDir + "/bin/safplusAmf.db")
    except OSError as e: pass # ok file does not exist

    #cwd = os.path.abspath(tgtDir + "/bin")

    # now write the new data
    #db = safplusMgtDb.PyDBAL(cwd + "/safplusAmf") # Root of Log service start from /log ->  docRoot= "version.log_BootConfig.log"
    db = safplusMgtDb.PyDBAL("safplusAmf")
    db.LoadXML(modelXML)
    db.Finalize()

    # now start safplus_amf
    bufferingSize = 64  # 4096
    cwd = os.path.abspath(tgtDir + "/bin")
    args = (cwd + "/safplus_amf",)
    print("Environment is: ", os.environ)
    amf = subprocess.Popen(args,bufferingSize,executable=None, stdin=infileFp, stdout=outfileFp, stderr=outfileFp, preexec_fn=None, close_fds=True, shell=False, cwd=cwd, env=None, universal_newlines=False, startupinfo=None, creationflags=0)
    print("SAFplus AMF is running pid [%d]" % amf.pid)
    time.sleep(5)
    if amf.poll() != None:  # If some kind of config error then AMF will quit early
        time.sleep(20)
        pdb.set_trace()
        raise TestFailed("AMF quit unexpectedly")

    time.sleep(10)

def main(tgtDir):
    os.environ["ASP_NODENAME"] = "node0"

    #amfpid = int(subprocess.check_output(["pgrep","-n", "safplus_amf"]).strip())
    #status = procStatus(amfpid)
    #if status == "Z": # zombie
    #    raise ZombieException()
    #print("AMF running as process [%d]" % amfpid)

    try:
        amfpid = int(subprocess.check_output(["pgrep","-n", "safplus_amf"]).strip())
        status = procStatus(amfpid)
        if status == "Z": # zombie
            raise ZombieException()
        print("AMF is already running as process [%d]" % amfpid)
    except (subprocess.CalledProcessError,ZombieException):
        startupAmf(tgtDir,"amfOutput.txt")
        time.sleep(3) # Give the AMF time to initialize shared memory segments

    connectToAmf()

    #amfctrl.displaySgStatus("sg0")
    # Create this node if it is missing
    node = sp.mgtGet("/safplusAmf/Node/node0")
    if node == "":
        sp.mgtCreate("/safplusAmf/Node/node0")

    if 1:
        testCreateSingleApp()
        for i in range(1,4): testStartStop("/safplusAmf/ServiceGroup/tc001Sg")
        sp.mgtSet("/safplusAmf/ServiceGroup/tc001Sg/adminState","on")
        for i in range(1,4): testStartStop("/safplusAmf/ServiceUnit/tc001Su0")

    if 1:
        testCreateNsuMcomp("tc020",3,3)

    clTest.finalize();
    sp.Finalize()


if __name__ == '__main__':  # called from the target "test" directory
    main("..")

def Test():  # called from inside the source tree
    main("..")
    #sys.path.append("../../python")  # Point to the python code, in source control
    #main("../../target/i686/3.13.0-37-generic")
