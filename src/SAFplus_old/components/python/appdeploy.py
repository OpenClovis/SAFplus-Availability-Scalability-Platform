"""@package appdeploy
Dynamically deploy applications to the cluster

"""
# Python imports
import tempfile
import os, os.path
import re

# 3rdparty imports 
import pexpect

# ASP Runtime Director imports
import psh
import pxsshfix as pxssh

from dot import *
from misc import *

import clusterinfo
import xml2dict
import aspAmf
import aspAmfCreate
from aspLog import log

TEMP_DIR = "/tmp/openclovis/"
APP_CFG_FILE = "appcfg.xml"
APP_CFG_FILE_VAR = (1,0,0,0)




class DeployError(Error):
    """Basic error exception class for deployment problems.
    """
    pass


def copyApp(fromdir,to,appDir=None):
  "Private: Copy application files to the other blades"

  logdir = os.getenv("ASP_LOGDIR")
  if logdir:
    logFileName = logdir + os.sep + 'appdeploy.log'
  else:
    logFileName = 'appdeploy.log'

  try:
    fout = file(logFileName, 'w+')
  except:  # lack of debug logging should not be fatal
    fout = None

  exp = pexpect.spawn('bash')
  exp.delaybeforesend = 0 # to make sessions a lot faster
  i = exp.expect([pexpect.TIMEOUT, '[$#>]'], timeout=10)
  if i==0: # timeout
        raise DeployError("Could not start bash session")
  exp.setwinsize(200, 1024)

  exp.logfile = fout
  exp.sendline("export PS1='_expectpromptlocal_'")  
  pshell = psh.Psh(exp, '_expectpromptlocal_')

  # the "to" argument can be a string containing the node name, slot number or IP address, OR it can be an AmfNodeEntity
  if type(to) is type(""):
    node = clusterinfo.ci.nodes[to]
  else:
    node = to

  try:
    fout2 = file(logFileName + "1", 'w+')
  except:
    fout2 = None

  retries = 0
  while retries<4:
    retries += 1
    tgtexp = pxssh.pxssh()
      
    tgtexp.logfile = fout2
    log.info("Connecting to %s user %s pw %s" % (node.localIp, node.localUser, node.localPasswd))
    try:
      result = tgtexp.login(node.localIp, node.localUser, node.localPasswd, login_timeout=30)
      retries = 10000
    except pxssh.ExceptionPxssh,e: # Could not synchronize with original prompt
      if retries>3: raise DeployError("Cannot connect to %s exception: %s" % (node.localIp,str(e)))

  if not result:
      raise DeployError("Cannot connect to %s" % node.localIp)
  tgtexp.setwinsize(200, 1024)
  tgtexp.delaybeforesend = 0 # to make sessions a lot faster
  time.sleep(1) 
  tgtexp.sendline("export PS1='_tgtexpectprompt_'")
  tgtshell = psh.Psh(tgtexp, '_tgtexpectprompt_')

  if appDir:
    # I need to copy deploy/<dir> to computer:<asp_base>/<dir>/<AppName>
    for d in ["bin","etc","lib"]:
      p = fromdir + os.sep + d
      if os.path.isdir(p) and os.listdir(p):
        appDir = node.aspdir + os.sep + d + os.sep + appDir
        tgtshell.mkdirs(appDir)
        pshell.scp(p + os.sep + "*", node.localUser + "@" + node.localIp + ":" + appDir, node.localPasswd)
  else:
    pshell.scp(fromdir + os.sep + "*", node.localUser + "@" + node.localIp + ":" + node.aspdir, node.localPasswd)




def deployAppTgz(file,amfSession=None):
  """DO NOT USE Deploy an application tar file to the cluster and tell the AMF about it

  This top-level function takes a .tgz file, deploys it to all blades, and programs the appropriate entities into the AMF.

  Parameters:
    file:         Filename and path of the application .tgz archive
    amfSession:   Optional: a aspAmf.Session instance (otherwise I will create one)

  Exceptions:
    misc.Error:   Raised when shell commands return error codes

  Returns: [service_groups]
    service_groups: A list of Service Groups (primarily so you can start this app, but you can also traverse to get all application-specific entities).
  """
  
  (tgtdir,cfg) = openAppFile(file)
  return deploy(tgtdir,cfg,tgtblades,amfSession)

def copySoftware(appFile,tgtblades,abortIfCantAccess=False):
  """Copy and install software from the controller node to a list of target blades.  Unlike deploy this function does not modify the AMF state, it ONLY copies the application onto nodes.

  Parameters:
    appFile:      Object of type aspApp.AppFile describing what the bundle to be installed.
    tgtblades:    List of objects of type aspAmfEntity.AmfNode indicating what nodes to install the bundle onto.
    abortIfCantAccess: If TRUE the function will raise an exception if a Node is not accessible, otherwise it will continue onto the other Nodes

  Exceptions (only if abortIfCantAccess=True):
    psh.ExceptionErrorCode, pxssh.EOF: These exceptions are node access issues

  Returns: (errors,notes)
    errors is a list of strings describing any installation failures (if abortIfCantAccess=False installation failures will be put here)
    notes is a list of strings describing any logs, etc.
  """
  return copySwInternal(appFile.cfg,appFile.dir, tgtblades,abortIfCantAccess)

def copySwInternal(cfg,srcDir,tgtblades,abortIfCantAccess=False):
  """Private software copy implementation"""
  notes = []
  errors = []

 # GAS TODO:  applications should be in their own dirs in TAR file and deployment should be in the for loop below
  firstCfg = cfg.values()[0]
  appDir = firstCfg["appDir"]

  # Push it to other blades
  alreadyRunning = []
  notAccessible  = []
  for tgtblade in tgtblades:
    if tgtblade.isRunning():
      try:
        copyApp(srcDir + os.sep + "deploy", tgtblade, appDir)
      except psh.ExceptionErrorCode, e:
        if e.error == 1:
          alreadyRunning.append(tgtblade.name)
        else:
          if abortIfCantAccess: raise
          else: notAccessible.append(tgtblade.name)
      except pxssh.EOF,e:
          if abortIfCantAccess: raise
          else: notAccessible.append(tgtblade.name)
    else:
      if abortIfCantAccess:
        raise DeployError("Deployment target node [%s] (IP: [%s]) is not accessible" % (tgtblade.name, tgtblade.localIp))
      else: notAccessible.append(tgtblade.name)

  if len(notAccessible):
    errors.append("Deployment target nodes %s are not accessible.  You must manually copy the software to these nodes, if it is not already there." % List2Str(notAccessible))
  if len(alreadyRunning):
    errors.append("This software is already running on nodes %s. To overwrite, stop all instances before deployment." % List2Str(alreadyRunning))

  return (errors,notes)

 

def deploy(fromdir,cfg,tgtblades=None,amfSession=None,abortIfCantAccess=False,copy=True,deploy=True):
  """Deploy an opened and verified application to the cluster and tell the AMF about it.

  This function takes an directory containing an application and an application configuration.
  It deploys the app all necessary blades, and programs the appropriate entities into the AMF.
  It is a functional breakdown of the deployAppTgz to be used when you want to do a 3 step deploy:
  1. Open .tgz (openAppFile())
  2. Modify the application or its configuration (custom)
  3. deploy it (this function)

  Parameters:
    cfg:            Parsed application configuration (object of type dot.Dot()), describing the deployment details
    tgtblades:      Optional: What blades should be used.  If not specified, all blades are candidates for deployment
    amfSession:     Optional: a aspAmf.Session instance (otherwise I will create one)
    abortIfCantAccess Optional: If a blade is not accessible, abort the deployment (default FALSE)
    copy:           Optional: pass False to NOT copy the software to the nodes (default true)
    deploy:           Optional: pass False to NOT deploy the sg into the AMF (default true)

  Exceptions:
    DeployError(base misc.Error):   Raised when shell commands return error codes

  Returns: ([service_groups],([errors],[notes])
    - service_groups: A list of Service Groups (primarily so you can start this app, but you can also traverse to get all application-specific entities).
  """

  
  errors = []
  notes = []

  # If the user specifically chooses blades and one is down, it is an error
  # otherwise we will deploy to "live" nodes
  ci = clusterinfo.ci

  if tgtblades:
    specifiedNodes = True
  else:
    # Get all "live" blades
    tgtblades = filter(lambda x: x.isRunning(), ci.nodeList)
    specifiedNodes = False

  if copy:
    (e,n) = copySwInternal(cfg,fromdir,tgtblades,abortIfCantAccess)
    errors += e
    notes += n 
  else:
    notes += ["You selected to not install the software, so you must do that manually if it is not already installed."]
  
  if not deploy:
    notes += ["You selected to not deploy the software, so no application deployment is created in the AMF."]
    return ([],(errors,notes))

  ##
  # Create in the AMF

  # GAS Note:  This will have to change if we provide multiple apps in a single xml file
  (appname,appcfg) = cfg.items()[0]
  appver = appcfg.ver

  (entities,(e,n)) = createModel(fromdir,cfg,tgtblades,specifiedNodes)
  notes += n
  errors += e

  (e,n) = installModel(appname, appver, entities, amfSession)
  notes += n
  errors += e

  ret = filter(lambda x: x.type == aspAmf.ServiceGroupType, entities)
  return (ret, (errors,notes))


def createModel(fromdir,cfg,tgtblades,specifiedNodes):
  ci = clusterinfo.ci

  notes = []
  errors = []
  # Create the SAF AMF entities corresponding to this application
  for (appname,appcfg) in cfg.items():

    integration = appcfg.get("GuiIntegration",None)
    if integration:
      install = integration.install
      exec install in globals(), {}

    basename = appcfg.get("deployPrefix",appname) # Deploy preferentially with the deployment prefix, but use the app name if there isn't one
    nameIndex = 0
    for existingSg in ci.sgList:
      if basename + "SGi" in existingSg.name:
        i = int(existingSg.name.replace(basename + "SGi",""))
        if i >= nameIndex: nameIndex = i+1
    if nameIndex:
      notes.append("Name %s exists, so appending the numeral %d." % (basename,nameIndex))
 
    compNames = appcfg.programNames.values()
    nodes = []

    ninst = 1
    if appcfg.modifiers.has_key('instancesPerNode'):
      ninst = int(appcfg.modifiers.instancesPerNode)
      tgtblades = tgtblades * ninst
      
    if specifiedNodes:
      numNodes = len(tgtblades) # We deploy to where the user selected
    else:
      numNodes = min(appcfg.totalNodes*ninst,len(tgtblades)) # If no blades chosen, we just deploy the recommended instance(s)

    if appcfg.redundancyModel in ["2N", "2n", "2"]:
      log.info("%s uses 2N redundancy model" % basename)
      appcfg.rModel = "2N"
      appcfg.activePerSg = 1
      appcfg.standbyPerSg = 1
      nodesPerSg = 2
    elif appcfg.redundancyModel.lower() == "custom":
      appcfg.rModel = "custom"
      nodesPerSg = appcfg.totalNodes
      appcfg.setdefault("activePerSg", nodesPerSg)
      appcfg.setdefault("standbyPerSg", 0)      
    else:
      log.info("%s uses N+M redundancy model" % basename)
      regexp = r'\s*(?P<active>\d+)\s*\+\s*(?P<standby>\d+)\s*'
      m = re.match(regexp, appcfg.redundancyModel)
      d = m.groupdict()

      appcfg.rModel = "M+N"
# This configures the SG exactly as suggested in the appcfg.xml
#      appcfg.activePerSg = int(d['active'])
#      appcfg.standbyPerSg = int(d['standby'])
#      nodesPerSg = appcfg.activePerSg + appcfg.standbyPerSg

      try:  # If the # active is not a number then that's ok (see below)
        active = int(d['active'])
      except:
        active = None

      standby = int(d['standby'])

      if numNodes<3 and standby>0:  # If there's 1 or 2 nodes, use 1+1
        appcfg.activePerSg  = 1
        appcfg.standbyPerSg = 1
      elif active is None: # (N+1) Keep the specified # of standby and let active grow.
        appcfg.standbyPerSg = standby
        appcfg.activePerSg  = max(1,numNodes-standby) # But at least 1 active of course
      else:  # (3+1) Make sure there's at least 1 standby, otherwise keep the ratio intact
        sbratio = float(standby)/(float(active)+float(standby))
        nstandby = int(numNodes*sbratio)
        if nstandby < 1: nstandby = 1
        appcfg.standbyPerSg = nstandby
        appcfg.activePerSg  = max(1,numNodes-standby) # But at least 1 active of course

    # Assign each node to an SG.
    if 0:  # This logic generates multiple SGs if more nodes are passed than numNodes
      nodeCnt = 0
      while nodeCnt < numNodes:
        sgNodes = []     
        for perSg in range(0,nodesPerSg):
          if nodeCnt >= numNodes: break
          sgNodes.append(tgtblades[nodeCnt])
          nodeCnt +=1
        nodes.append(sgNodes) # Make a list of sgs which is therefore a list of lists of nodes
    else: # Instead, I'm just going to create a bigger SG
        nodes.append(tgtblades[0:numNodes])

    log.debug("Nodes: %s" % str([[x.name for x in y] for y in nodes]))
    entities = aspAmfCreate.CreateApp(compNames,nodes,appcfg,appcfg.get('work', None),basename, {},nameIndex)

    return (entities,(errors,notes))

 
def installModel(appname, appver, entities, amfSession=None):

    notes = []
    errors = []
    # Commit these entities into the AMF
    if amfSession is None:
      amfSession = aspAmf.Session()

    #log.debug("Entities: %s" % entities)
    for e in entities: log.debug("%s" % e.name)

    try:      
      amfSession.InstallApp(entities)
    except aspAmf.AmfError,e:
      notes.append(T("Error committing the configuration.  Application may be partially deployed: $err",err=str(e)))

    # Write the associated data
    appver = appname + " " + appver
    for e in entities:
      amfSession.SetAssociatedData(e,appver)

    try:      
      # Unlock the SUs, so that there is just 1 "on" switch (the SG)
      amfSession.Startup(filter(lambda x: x.type == aspAmf.ServiceUnitType, entities))
    except aspAmf.AmfError,e:
      errors.append(T("Error making some entities startable: $err",err=str(e)))

    # Return the SG names so it can be started up.
    #ret = []
    #for e in entities:
    #  if e.type == aspAmf.ServiceGroupType:
    #    ret.append(e)

    return  (errors,notes)


def Test():
  "Private: Unit test"

  sgs = deployAppTgz("exapp.tgz")
  print sgs
  s = aspAmf.Session()
  s.Startup(sgs)
