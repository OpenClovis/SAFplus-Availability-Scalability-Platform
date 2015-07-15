import types, pdb

import SAFplus as sp
import microdom

AmfPfx = "SAFplusAmf"
SiPfx = "ServiceInstance"

def csv2List(csvString):
  """Convert comma separated values to a Python list"""
  if not csvString.strip(): return []  # turn "" into [], otherwise it is ['']
  return [x.strip() for x in csvString.split(",")]

def getEntity(ent):
  """Given an entity string, or list of entity strings, load them from the AMF and convert to Python objects.  If the entity does not exist, return None"""
  if type(ent) is types.ListType:
    return [getEntity(x) for x in ent]
  xml = sp.mgtGet(ent)
  if xml:
    return microdom.LoadString(xml)
  return None

try: # only set this variable once, not every time reload occurs
  test = SAFplusInitialized  # Will throw an error if this variable does not exist
except:
  print "First run"
  SAFplusInitialized=False

class Error:
  def __init__(self,problemString):
    self.errString = problemString

def removePrefix(text, prefix):
    """Given a string or recursive list of strings, remove the string prefix (or any of a list of prefixes) if it exists.  Only chops the prefix off once per text"""
    if type(prefix) in types.StringTypes: prefix = [prefix]
    if type(text) is types.ListType:
      return [ removePrefix(x,prefix) for x in text]

    for pfx in prefix:
      if text.startswith(pfx):
        return text[len(pfx):]
    return text #or whatever

def getSgEntities(sgName):
  sgx = sp.mgtGet("/SAFplusAmf/ServiceGroup/" + sgName)
  if sgx: sg = microdom.LoadString(sgx)
  else:
    raise Error("no service group [%s], or no access" % sgName)
  suNames = csv2List(sg.serviceUnits.data_)
  sg.comp = {} # add a new variable to the sg object
  sg.su = {}  # add a new variable to the sg object that is a dictionary of su objects
  sg.si = {}
  sg.csi = {}
  for sun in suNames:
    if not sun: continue
    sut = sp.mgtGet(sun)
    if sut:
      sumd = microdom.LoadString(sut)
      sg.su[sumd.name.data_] = sumd # Add this su to the sg's dictionary
      sumd.sg = sg # Set up parent reference
      compNames = csv2List(sumd.components.data_)
      sumd.comp = {} # add a new variable to the su that is a dictionary of comp objects
      for cn in compNames:
        if not cn: continue
        cux = sp.mgtGet(cn)
        if cux:
          compMicrodom = microdom.LoadString(cux)
          sg.comp[compMicrodom.name.data_] = compMicrodom
          sumd.comp[compMicrodom.name.data_] = compMicrodom          
        else:
          raise Error("no component [%s], or no access" % cn)
    else:
      raise Error("no service unit [%s], or no access" % sun)

  siNames = [x.strip() for x in sg.serviceInstances.data_.split(",")]
  for sin in siNames:
    if not sin: continue
    six = sp.mgtGet(sin)
    if six:
      sim = microdom.LoadString(six)
      sg.si[sim.name.data_] = sim # Add this su to the sg's dictionary
      sim.csi = {}
      sim.sg = sg  # Set up parent reference
      csiNames = csv2List(sim.componentServiceInstances.data_)
      for csin in csiNames:
        if not csin: continue
        csix = sp.mgtGet(sin)
        csim = microdom.LoadString(csix)
        sg.csi[csim.name.data_] = csim
        sim.csi[csim.name.data_] = csim
        csim.si = sim
        csim.sg = sg
    else:
          raise Error("no service instance [%s], or no access" % sin)


  return sg

def activeStandby(si):
  if type(si) in types.StringTypes:
    six = sp.mgtGet("/" + "/".join ([AmfPfx,SiPfx,si]))
    if six: si = microdom.LoadString(six)
    else:
      raise Error("no service instance [%s], or no access" % si)
    # print six
    return (csv2List(si.activeAssignments.data_),csv2List(si.standbyAssignments.data_))
  

def displaySgStatus(sg):
  if type(sg) in types.StringTypes:
    sg = getSgEntities(sg)

  print "Service Group: %s" % sg.name.data_
  # print "Administrative State: %4s  Operational State: %10s  Service Units: Assigned: %d Idle: %d Spare: %d" % (sg.adminState.data_,"TBD",int(sg.numAssignedServiceUnits.data_),int(sg.numIdleServiceUnits.data_),int(sg.numSpareServiceUnits.data_) )
  print "  Administrative State: %4s" % (sg.adminState.data_)
  suList = sg.su.items()
  suList.sort()
  siList = sg.si.items()
  siList.sort()

  indent = 1
  # Show the physical entity tree
  print "  Service Units: ", ", ".join([x[0] for x in suList])
  print "  Service Instances: ", ", ".join([x[0] for x in siList])
  for (name,su) in suList:
    print "  Service Unit: ", name
    indent +=1
    print "    On Node: %s Administrative State: %4s  Presence: %s  Readiness State: %s  High Availability State: %s Readiness State: %s " % (su.node.data_, su.adminState.data_, su.presenceState.data_, su.readinessState.data_, su.haState.data_, su.haReadinessState.data_)
    compList = su.comp.items()
    compList.sort()
    print "    Components: ", ", ".join([x[0] for x in compList])
    for (name, comp) in compList:
      print "    Comp: ", name
      indent+=1
      print "      Presence: %s  Readiness State: %s  High Availability State: %s" % (comp.presenceState.data_, comp.readinessState.data_, comp.haState.data_)
      if comp.presenceState.data_ == "instantiated":
        print "%spid: %d" % ("  "*indent, int(comp.processId.data_))
      if comp.lastError.data_:
        print "%sLast Error: %s" % ("  "*indent, int(comp.lastError.data_))
       
      indent-=1
    indent-=1
  # Show the work
  for (name,si) in siList:
    print "%sService Instance: %s" % ("  "*indent, name)
    indent +=1
    numActive = int(si.numActiveAssignments.current.data_)
    numStandby = int(si.numStandbyAssignments.current.data_)
    print "%sAdministrative State: %4s  Assignment State: %s (%d/%d)" % ("  "*indent, si.adminState.data_, si.assignmentState.data_, numActive,numStandby)
    if numActive or numStandby:
      activeSus = removePrefix([ x.strip() for x in si.activeAssignments.data_.split(",")] ,"/SAFplusAmf/ServiceUnit/")
      standbySus = removePrefix([x.strip() for x in si.standbyAssignments.data_.split(",")],"/SAFplusAmf/ServiceUnit/")
      print "%sActive on: %s  Standby on: %s" % ("  "*indent,", ".join(activeSus), ", ".join(standbySus))
    indent-=1


def Test():
  pdb.set_trace()
  global SAFplusInitialized
  if not SAFplusInitialized:
    svcs = sp.Libraries.MSG | sp.Libraries.GRP
    sic = sp.SafplusInitializationConfiguration()
    sic.port = 55
    SAFplusInitialized = True
    sp.Initialize(svcs, sic)
  displaySgStatus("sg0")

  ret = activeStandby("si")
  print "Active: %s" % str(ret[0])
  print "Standby: %s" % str(ret[1])
