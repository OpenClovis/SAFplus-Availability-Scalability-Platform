import sys
import atexit
import safplus
import pdb

mgtGet = safplus.mgtGet
mgtSet = safplus.mgtSet
mgtCreate = safplus.mgtCreate
mgtDelete = safplus.mgtDelete
getProcessHandle = safplus.getProcessHandle

amfMgmtInitialize = safplus.amfMgmtInitialize
amfMgmtFinalize = safplus.amfMgmtFinalize
nameInitialize = safplus.nameInitialize

amfMgmtNodeLockAssignment = safplus.amfMgmtNodeLockAssignment
amfMgmtSGLockAssignment = safplus.amfMgmtSGLockAssignment
amfMgmtSULockAssignment = safplus.amfMgmtSULockAssignment
amfMgmtSILockAssignment = safplus.amfMgmtSILockAssignment


amfMgmtNodeLockInstantiation = safplus.amfMgmtNodeLockInstantiation
amfMgmtSGLockInstantiation = safplus.amfMgmtSGLockInstantiation
amfMgmtSULockInstantiation = safplus.amfMgmtSULockInstantiation

amfMgmtNodeUnlock = safplus.amfMgmtNodeUnlock
amfMgmtSGUnlock = safplus.amfMgmtSGUnlock
amfMgmtSUUnlock = safplus.amfMgmtSUUnlock
amfMgmtSIUnlock = safplus.amfMgmtSIUnlock


amfMgmtNodeRepair = safplus.amfMgmtNodeRepair
amfMgmtCompRepair = safplus.amfMgmtCompRepair
amfMgmtSURepair = safplus.amfMgmtSURepair

amfMgmtNodeRestart = safplus.amfMgmtNodeRestart
amfMgmtSURestart = safplus.amfMgmtSURestart
amfMgmtCompRestart = safplus.amfMgmtCompRestart

amfMgmtSGAdjust = safplus.amfMgmtSGAdjust

amfMgmtSISwap = safplus.amfMgmtSISwap

amfMgmtCompErrorReport = safplus.amfMgmtCompErrorReport
Recovery_NoRecommendation = safplus.Recovery.Recovery_NoRecommendation
Recovery_Restart = safplus.Recovery.Recovery_Restart
Recovery_Failover = safplus.Recovery.Recovery_Failover
Recovery_NodeSwitchover = safplus.Recovery.Recovery_NodeSwitchover
Recovery_NodeFailover = safplus.Recovery.Recovery_NodeFailover
Recovery_NodeFailfast = safplus.Recovery.Recovery_NodeFailfast


def isValidDirectory(path):
  try:
    data = mgtGet(str("{d=0}"+path))
    if not data: return False  # TODO, what about an empty list?
  except RuntimeError, e:
    return False
  # print "isValidDir: ", str(data)
  # pdb.set_trace()
  return True

class Commands:
  def __init__(self):
    self.commands = {}
    self.context = None

  def canonicalPath(self,location):
    #print "cur: ", self.context.curdir, " loc: ", location
    if location == "." or location == "" or location == None:
      return self.context.curdir
    if location[0] == "/": # not a relative path
      return location
    if self.context.curdir == "/":
      return "/" + location
    elif self.context.curdir:
      return self.context.curdir + "/" + location
    return location    

  def do_set(self, location, value):
    """syntax: set (location) (value)  
        Sets the leaf at the specified locations to the specified value
    """
    loc = self.canonicalPath(location)
    ret = safplus.mgtSet(str(loc),str(value))
    if ret == 4:  # CL_ERROR_NOT_EXIST
      return "<error>location [%s] does not exist</error>" % location
    elif ret == 0:
      return ""
    return str(ret)

  def do_create(self,*locations):
    """syntax: create (location)  
         Creates a new object at the specified location.  The type of the object is implied by its location in the tree.
    """
    result = []
    for location in locations:
      loc = self.canonicalPath(location)
      try:
        safplus.mgtCreate(str(loc))
      except RuntimeError, e:
        result.append("<error>location [%s] error [%s]</error>" % (location, str(e)))
    return "<top>" + "".join(result) + "</top>"

  def do_delete(self,*locations):
    """syntax: delete (location)  
         deletes the object at (location) and all children
    """
    result = []
    for l in locations:
      loc = self.canonicalPath(l)
      try:
        safplus.mgtDelete(str(loc))
      except RuntimeError, e:
        result.append("<error>location [%s] error [%s]</error>" % (location, str(e)))

    return "<top>" + "".join(result) + "</top>"

  def setContext(self,context):
    """Sets the context (environment) object so commands can access it while they are executing"""
    self.context = context

def Initialize():
  port = 52
  while True:
    try:
      svcs = safplus.Libraries.MSG | safplus.Libraries.GRP | safplus.Libraries.MGT_ACCESS | safplus.Libraries.NAME
      sic = safplus.SafplusInitializationConfiguration()
      sic.port = port
      atexit.register(safplus.Finalize)  # Register a clean up function so SAFplus does not assert on a dead mutex when quitting
  
      safplus.Initialize(svcs, sic)
      break
    except RuntimeError, e:
      errMsg = str(e) 
      if errMsg== 'Address already in use':
        port+=1
      else:
        print 'Error while initializing safplus. Error message: '+errMsg
        sys.exit('Exiting the program...')     
    
  return (Commands(),{})

def Finalize():
  safplus.Finalize()

def isListElem(elem,path):
  """Return the name of the list if this is an item in a list"""
  if elem.attrib.has_key("listkey"):
    return elem.tag
  return None
