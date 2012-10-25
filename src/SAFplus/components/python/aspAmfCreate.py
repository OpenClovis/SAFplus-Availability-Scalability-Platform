"""This module contains helper functions that generate sets of internally consistent AMF entities that model typical application deployment scenarios.  They are essentially convenience functions so that the user does not have to create all the AMF entities direct instantiation.
"""

import os
import asp
from misc import *
from aspLog import *
from aspAmfEntity import *

DefaultStandbyWorkPerComp = 100
DefaultActiveWorkPerComp  = 100

class NameMgr:
  def __init__(self):
    self.names = {}
    self.cnt = 0
    
  def fix(self,name):
    if self.names.has_key(name):
      self.cnt+=1
      return self.fix(name + "_" + str(self.cnt))
    else:
      self.names[name] = True
      return name


def Modifiers2Sg(mods,sg):
  """Helper function that translates and moves configuration modifiers into a Python-level SG representation.
  This SG can then be sent to the AMF (aspAmf.Session.InstallEntities()) to write these configuration modifiers into the AMF.
  
  Parameters:
    mods:     A Dot() object or other entity with SG configuration parameters expressed as member variables.  The SG configuration parameters are exactly as written in ClAmsSGConfigT C structure located in clAmsEntities.h
    sg:       An object of type aspAmfEntity.AmfServiceGroup that should receive the parameters.
    """
  Log("Modifiers %s" % str(mods))
  # Explicitly configured overrides anything that is currently in there...
  try: sg.activeServiceUnits  = int(mods.numPrefActiveSUs)
  except AttributeError: pass
  try: sg.standbyServiceUnits = int(mods.numPrefStandbySUs)
  except AttributeError: pass
  try: sg.maxActiveSIsPerSU  = int(mods.maxActiveSIsPerSU)
  except AttributeError: pass
  try: sg.maxStandbySIsPerSU = int(mods.maxStandbySIsPerSU)
  except AttributeError: pass
  try: sg.loadingStrategy = int(mods.loadingStrategy)
  except AttributeError: pass
  try: sg.failback = yesNo2Bool(mods.failbackOption)
  except AttributeError: pass
  try: sg.autoRepair = yesNo2Bool(mods.autoRepair)
  except AttributeError: pass
  try: sg.processValidationPeriod = duration2ms(mods.compRestartDuration)
  except AttributeError: pass
  try: sg.compRestartCountMax = int(mods.compRestartCountMax)
  except AttributeError: pass
  try: sg.suRestartDuration = duration2ms(mods.suRestartDuration)
  except AttributeError: pass
  try: sg.suRestartCountMax = int(mods.suRestartCountMax)
  except AttributeError: pass
  try: sg.collocation = yesNo2Bool(mods.isCollocationAllowed)
  except AttributeError: pass
  try: sg.redundancyDegradationRatio = int(mods.alpha)
  except AttributeError: pass
  try: sg.autoAdjust = yesNo2Bool(mods.autoAdjust)
  except AttributeError: pass
  try: sg.autoAdjustInterval = duration2ms(mods.autoAdjustProbation)
  except AttributeError: pass
  try: sg.reductionProcedure = yesNo2Bool(mods.reductionProcedure)
  except AttributeError: pass
  try: sg._redundancyModel = int(mods.redundancyModel)
  except AttributeError: pass
  try: sg.instantiateDuration = duration2ms(mods.instantiateDuration) 
  except AttributeError: pass



def CreateServiceInstance(sg,name=None,kvdict=None,cfg=None):
  """Create a service instance and related component service instances.
  One CSI is created per component (process) defined in the sg and every item in kvdict is put into every CSI.  Therefore every CSI contains the exact same set of name/value pairs within an SI.  Note that this is a small simplification from the SAF definition which allows CSIs to contain different name/value pairs.  Since components can just ignore name/value pairs that do not pertain to them, this simplification is considered harmless.  If you don't like it then go ahead an write your own Create routine (its not hard!)

  Note that this function just created RAM entities; it does not create them in the AMF.  Do that using aspAmf.Session.InstallApp()
  
  Parameters:
    sg:     An object of type aspAmfEntity.AmfServiceGroup that is the parent of the SI
    name:   The string name of the SI
    kvdict: A dictionary of key/value strings that will become the name/value pairs in each CSI.

  Returns: (created, modified)
    created:  A list of aspAmfEntity objects that were created during this function call
    modified: A list of aspAmfEntity objects that were modified during this function call (in this case, just the SG is modified)
  """

  created = []
  modified = []

  if not cfg:
    if sg.appVer:
      cfg = sg.appVer.cfg.values()[0]  # GAS TODO: Right now, just picking the first defined application in the bundle
      comps = cfg.programNames.values()
    else:
      if sg.su:  # Does the SG even have any SUs?
        comps = [x.name for x in sg.su[0].comp]  # If we don't have config, then pull it from the running AMF
      else: raise Error("Service Group '%s' must have Service Units before Service Instances can be created" % sg.name)
  else:
    comps = cfg.programNames.values()
  


  if not name:
    basename = sg.name[0:sg.name.find("SG")]
    name = basename + "SIi" + str(index) + "_" + name 

  svcInst = AmfServiceInstance(name)
  created.append(svcInst)
  svcInst.sg = sg
  sg.si.append(svcInst)
  modified.append(sg)  # did not actually modify the content...

  index = 0
  for proc in comps:
    # Using 'proc' as the csi type, which should have been created during aspAmfCreate, 
    csi = AmfComponentServiceInstance(name + "CSIi" + str(index) + "_" + name + "_" + proc,kvdict,proc)

    # Add the CSI to its SI 
    svcInst.csi.append(csi)

    # GAS TODO: Identify all instances of 'proc' and make sure they can take this CSI type
    # I don't need to do this right now, because I am using the default csiType

    created.append(csi)
    index += 1


  return (created,modified) 
    

def CreateServiceUnits(sg,nodesInSg,appnames,cfg,basename,csiLut,index=0,namesTaken=None):
    """ Add a bunch of SUs into an existing SG -- used during upgrade.
  This function creates a Service Unit for every node passed in the nodesInSg list.  

  Note that this function just creates RAM entities; it does not create them in the AMF.  Do that by passing all entities to aspAmf.Session.InstallApp() when all of your creating is done.
  
  Parameters:
    sg:         An object of type aspAmfEntity.AmfServiceGroup that is the parent of the SUs
    nodesInSg:  A list of nodes; an SU will be created in each one of these nodes. 
    appnames:   A list of strings that name every application.  Every SU have children SAF components, one for each of these names. 
    cfg:        A Dot() object that specifies any extended configuration that should be applied to the created SUs or Components, including:
                appDir:  The location of the application binaries (for example the full component instantiation path for the first application will be cfg["appDir"]/appname[0])
    basename:   The root name of every SU created.
    csiLut:     csi Look up table: Understanding the CSILUT requires detailed understanding of the SAF information model.
                Basically, it is a mapping by application of the csiTypes that can be assigned to that application.
    index:      (default 0) What number to start with when numbering the new SUs.

  Returns: (created,modified,appInstLut)
    created:  A list of aspAmfEntity objects that were created during this function call
    modified: A list of aspAmfEntity objects that were modified during this function call (in this case, just the SG is modified)
    appInstLut: A look up table of all program instances (i.e on particular nodes) indexed by name.  This can be used to connect CSIs to app instances if you are creating your CSIs AFTER your SUs (if they are already created, use the csiLut parameter to this function).
    """
    if namesTaken is None: namesTaken = NameMgr()

    appDir = cfg["appDir"]

    if len(appDir): appDir = appDir + os.sep
    modified = [sg]
    created  = []

    # A look up table of all program instances indexed by name
    # This will be used to connect CSIs to app instances
    appInstLut = {}


    for n in nodesInSg:
      modified.append(n)
      nodeName = n.name
      svcUnit = AmfServiceUnit(namesTaken.fix(basename + "SU" + "i" +  str(index) + "_on_" + nodeName), sg, n)
      sg.su.append(svcUnit)
      n.su.append(svcUnit)
      created.append(svcUnit)
      for app in appnames:
        comp = AmfComponent(namesTaken.fix(basename + "Ci" + str(index) + "_" + app + "_on_" + nodeName))
        comp.su = svcUnit
        comp.instantiateCommand = appDir + app
        # Either defined in configuration, equivalent to the # of SIs per SU (because having SIs that don't have CSIs is a very esoteric situation), or choose a very big number
        comp.numMaxStandbyCSIs = int(cfg.get("standbyWorkPerComp",cfg.get("maxStandbySIsPerSU",DefaultStandbyWorkPerComp)))  
        comp.numMaxActiveCSIs = int(cfg.get("activeWorkPerComp",cfg.get("maxStandbySIsPerSU",DefaultActiveWorkPerComp)))  # Either defined in configuration, equivalent to the # of SIs per SU, or choose a very big number
        Log("Created component %s, command %s" % (comp.name, comp.instantiateCommand))

        if csiLut:
          if csiLut.has_key(app):
            comp.supportedCsis = set(csiLut[app])
            #comp.supportedCsis = set(x.csiType for x in csiLut[app])  # used when csiLut was a list of CSIs, not a set of csi types
          else:
            Log("Cannot find any CSIs that could be assigned to application [%s].  Work may not be assignable.  Available apps are [%s]." % (app,str(csiLut.keys())))

        Log("Created component %s, command %s" % (comp.name, comp.instantiateCommand))
        if not appInstLut.has_key(app): appInstLut[app] = []
        appInstLut[app].append(comp)
        svcUnit.comp.append(comp)
        created.append(comp)
    return (created,modified,appInstLut)        


def CreateApp(appnames, nodes, cfg, svcInsts, basename=None, associatedData={}, index = 0,namesTaken=None):
  """ Create the SAF entity hierarchy to run a set of applications on a set of nodes

  This is a huge top-level function that creates an entire SAF SG hierarchy.  To do all of this in one function certain assumptions must be made.  So if you have a strange desired configuration this function might not support it.  However it can still be useful tocall since it returns all created entities.  You may then modify them before committing them to the AMF.  Commit to AMF by passing all entities to aspAmf.Session.InstallApp() when all of your tweaking is done.

  Parameters:
  @param appnames is a process name or a list of process names.  All processes will be run within the same service unit
  @param nodes    is either a list of nodes, or a list of list of nodes (type AmfNode), depending on whether you want 1 or many service group instances
  @param cfg   is a dictionary of key/value pairs describing the detailed configuration of this application.  The values are essentially what is allowed in the "modifiers" section of the appcfg.xml file.
  @param svcInsts is a dictionary of dictionaries or a list of dictionaries, where each dictionary describes the key/value pairs in the service group.  All CSIs get all kv pairs defined in the SI; there is no way to specify different values for different CSIs within the same SI
  @param basename is a string prefix used to name of all created entities.  For example passing "foo" -> SG name "fooSGi0"  
  @param index    is a number that is used in the creation of names to identify an entity instance.  If multiple service groups are specified, the index is incremented.
  """

  Log("CreateApp(%s,%s,%s,%s,%s,%s,%s)"%(str(appnames), str(nodes), str(cfg), str(svcInsts), str(basename), str(associatedData), str(index)))

  if not len(nodes):
    return

  if namesTaken is None: namesTaken = NameMgr()

  ret = []
  sgs = []
  params = cfg.get("modifiers",None)
  appDir = cfg.get("appDir", "")
  if len(appDir): appDir = appDir + os.sep
  
  # Make nodes into a list of lists if it is not
  if not type(nodes[0]) is types.ListType:
    nodes = [nodes]

  # Make appnames into a list of strings if it is not
  if type(appnames) is types.StringType:
    appnames = [appnames]

  # Take the root name to be the first app's name if it is not passed in
  if basename is None:
    basename = appnames[0]

  # Make appnames into a list of dictionaries if it is just a single dictionary
  if type(svcInsts) is types.DictType:
    if not type(svcInsts.values()[0]) is types.DictType:
      svcInsts = [svcInsts]

  # [[a,b]] will make 1 sg spanning nodes a and b, [[a,b],[c,d,e]] makes 2 sgs...
  totalActiveSUs = 0
  origIndex = index

  for nodesInSg in nodes:
    sg = AmfServiceGroup(namesTaken.fix(basename + "SG" + "i" + str(index)))
    sg.associatedData = associatedData.get("SG", None)

    if 0: # Assign all SG parameters from the config to the new SG 
      redProc = params.get("automaticActiveAssignment",None)
      if redProc: sg.reductionProcedure = yesNo2Bool(redProc)

    if cfg.rModel in ["2N"]:
      sg._redundancyModel = asp.CL_AMS_SG_REDUNDANCY_MODEL_TWO_N
      sg.activeServiceUnits = 1
      sg.standbyServiceUnits = 1
      totalActiveSUs += 1  
    elif cfg.rModel in ["M+N"]:
      sg._redundancyModel = asp.CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N
      sg.activeServiceUnits = cfg.activePerSg
      sg.standbyServiceUnits = cfg.standbyPerSg
      totalActiveSUs += sg.activeServiceUnits
    elif cfg.rModel in ["custom"]:
      sg._redundancyModel = asp.CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM
      sg.activeServiceUnits = cfg.activePerSg
      sg.standbyServiceUnits = cfg.standbyPerSg
      totalActiveSUs += sg.activeServiceUnits
    
    sg.assignedServiceUnits = sg.instantiatedServiceUnits = sg.activeServiceUnits + sg.standbyServiceUnits

    Modifiers2Sg(params,sg)

    sgs.append(sg)

    if 1:
      (created,modified,appInstLut) = CreateServiceUnits(sg,nodesInSg,appnames,cfg,basename,None,index, namesTaken)
      ret += created
      ret += modified # the SG is part of modified, so that's how it gets into 'ret'
      Log("App instance Lookup Table is: %s" % str(appInstLut))
    else:
      deadcode = """  
    # A look up table of all program instances indexed by name
    # This will be used to connect CSIs to app instances
    appInstLut = {}

    for n in nodesInSg:
      ret.append(n)
#      Set nodename to the slot if the node has one, otherwise to the node's name
#      nodeName = str(n.slot) if n.slot!=0 else n.name
      nodeName = n.name
      svcUnit = AmfServiceUnit(basename + "SU" + "i" +  str(index) + "_" + nodeName, sg, n)
      sg.su.append(svcUnit)
      n.su.append(svcUnit)
      for app in appnames:
        comp = AmfComponent(basename + "_" + app + "_" + str(index) + "_" + nodeName)
        comp.su = svcUnit
        comp.instantiateCommand = appDir + app
        comp.numMaxStandbyCSIs = cfg.get("standbyWorkPerComp",100000)  # Either defined in configuration, or choose a very big number
        Log("Created component %s, command %s" % (comp.name, comp.instantiateCommand))
        if not appInstLut.has_key(app): appInstLut[app] = []
        appInstLut[app].append(comp)
        svcUnit.comp.append(comp)
        ret.append(comp)
"""        

    #ret += sg.su
    index += 1

  index = origIndex   # Reset the index

  siBasename = basename + "SI" + "i"
  if type(svcInsts) is types.ListType: # If I passed a list, then that is a list of work assignments.
    svcIt = []
    svcIndex = 0
    for x in svcInsts:
      svcIt.append((siBasename + str(svcIndex), x))
      svcIndex += 1
  elif Implements(svcInsts, types.DictType): # If I passed a dictionary, there is just 1 work assignment.
    svcIt = svcInsts.items()
  elif type(svcInsts) is types.NoneType: # If there is no work defined, then assign 1 work for each active.
    svcIt = []
    for svcIndex in range(0, totalActiveSUs):
      svcIt.append((siBasename + str(svcIndex), None))

  for ((name,sikvdict),sg) in ziprepeat(svcIt,sgs):
      svcInst = AmfServiceInstance(namesTaken.fix(basename + "SIi" + str(index) + "_" + name))
      sg.si.append(svcInst)
      ret.append(svcInst)
      for proc in appnames:
        csi = AmfComponentServiceInstance(namesTaken.fix(basename + "CSIi" + str(index) + "_" + name + "_" + proc),sikvdict, proc)
        # By default we make one applicable CSItype for each process, so we just call it the same name as the process

        # Add the CSI to its SI 
        svcInst.csi.append(csi)
        # Add the CSI to the processes that can take it
        pi = appInstLut[proc]
        for p in pi:
          if p.su.sg == sg:  # If the process is in this SG
            Log("Adding CSI type %s to component %s (%s)" % (csi.csiType,p.name,proc))
            p.supportedCsis.add(csi.csiType)
        # Add the CSI to the list of all created entities
        ret.append(csi)


  return ret
