"""This module defines the fundamental AMF entity types representing the objects in the SAF AMF entity model.  These entities correspond to both physical things in the chassis (such as nodes), instances of running programs (components), and logical combinations of these entities (such as service units and redundancy groups).

These entities are used in two ways.  First, you may use them to create new entities in the AMF model by creating the objects via their constructors, configuring all member parameters and then passing them to aspAmf.Session.InstallApp to actually instantiate them in the AMF.  Second, you may examine the current AMF configuration and state by receiving these objects via the clusterInfo APIs.

Certain member functions (like isProtected, isOk, isRunning) only have meaning when used on objects that represent actual cluster state.  If called on user-created entities, an exception will be raised.

"""
import types

from misc import *
import asp
import asppycustom

# TYPES

## @var NodeType
#  SAF AMF Node Type
NodeType = asp.CL_AMS_ENTITY_TYPE_NODE

## @var AppType
#  SAF AMF Application Type
AppType  = asp.CL_AMS_ENTITY_TYPE_APP

## @var ServiceGroupType
#  SAF AMF Service Group Type
ServiceGroupType = asp.CL_AMS_ENTITY_TYPE_SG

## @var ServiceUnitType
#  SAF AMF Service Unit Type
ServiceUnitType  = asp.CL_AMS_ENTITY_TYPE_SU

## @var ServiceInstanceType
#  SAF AMF Service Instance Type
ServiceInstanceType = asp.CL_AMS_ENTITY_TYPE_SI

## @var WorkAssignmentType
#  Synonym for SAF AMF Service Instance Type, since "work assignment" better describes the idea
WorkAssignmentType  = asp.CL_AMS_ENTITY_TYPE_SI

## @var ComponentType
#  SAF AMF Component Type
ComponentType       = asp.CL_AMS_ENTITY_TYPE_COMP

## @var ComponentServiceInstanceType
#  SAF AMF  Type
ComponentServiceInstanceType = asp.CL_AMS_ENTITY_TYPE_CSI

## @var ComponentWorkAssignmentType
#  Synonym for SAF AMF Component Service Instance Type
ComponentWorkAssignmentType = asp.CL_AMS_ENTITY_TYPE_CSI

## @var ClusterType
#  SAF AMF Cluster Type
ClusterType = asp.CL_AMS_ENTITY_TYPE_CLUSTER

# STATES
## @var RunningState
#  SAF AMF Unlocked state
RunningState        = asp.CL_AMS_ADMIN_STATE_UNLOCKED
## @var IdleState
#  SAF AMF Locked Assignment state
IdleState           = asp.CL_AMS_ADMIN_STATE_LOCKED_A
## @var StoppedState
#  SAF AMF Locked Instantiation state
StoppedState        = asp.CL_AMS_ADMIN_STATE_LOCKED_I
## @var ShuttingDownState
#  SAF AMF Shutting Down state
ShuttingDownState   = asp.CL_AMS_ADMIN_STATE_SHUTTINGDOWN


class AmfEntity:
  """Base class that models all SAF Entities.
  Never directly created.

  Derived class entities can be either created (by clusterinfo) as a representation of cluster (SAF AMF) state and configuration, or can be created by the user.  The purpose of user-created entities is to eventually tell the system actually create the entities in the SAF AMF using the @ref aspAmf.Session.InstallApp API.

  Certain member functions (like isProtected, isOk, isRunning) only have meaning when used on objects that represent actual cluster state.  If called on user-created entities, an exception will be raised.
  """

  # Member variables

  ## @var name
  #  Name of this entity as known by the AMF

  ## @var type
  #  SAF AMF type of the entity (see module-global variable that define available types)

  ## @var cconfig
  #  SAF AMF configuration of the entity.  Note that this variable only exists for entities that are pulled out of a running cluster (as opposed to created by you).  Various Entity member function use this variable and so will raise an exception if the member function is used on an entity that is not part of the AMF.

  ## @var cstate
  #  SAF AMF entity state.  Note that this variable only exists for entities that are pulled out of a running cluster (as opposed to created by you).  Various Entity member function use this variable and so will raise an exception if the member function is used on an entity that is not part of the AMF.


  def __init__(self,typee,name):
    """Constructor
    Parameters:
      typee:  The AMF type of this entity. See the globals defined in this file (NodeType,AppType,ServiceGroupType, etc)
      name:   The name of this entity
    """
    self.type = typee
    self.name = name
    self.associatedData = None

  def __cmp__(self,other):
    """The comparison operator simply compares entity names since the AMF mandates that entity names are unique"""
    if other is None: return -1
    return cmp(self.name,other.name)

  def __hash__(self): # Every AMF entity must have a unique name, so we can just hash on it
    return self.name.__hash__()

  def __str__(self):
    return "AmfEntity: name: %s, type: %d" % (self.name,self.type)

  def setInitialState(self,state):
    """Set the initial state of this entity (call before you pass the object to aspAmf)
       @param state Object of type AmfEntityStateChange
    """    
    self.initialState = state

  def copyLocalState(self,entity):
    """Copy the locally kept state in this entity from one entity to another of the same type.
       For example, node IP addresses, username/pw, ASP directory are all locally kept state.
       @param entity The entity to copy data from
    """    
    return

  def setConfig(self, Cconfig):
    "Private: Associate an underlying C config object with this entity"
    self.cconfig = Cconfig

  def setState(self, Cstate):
    "Private,Deprecated: Associate an underlying C state object with this entity"
    self.cstatus = Cstate

  def setStatus(self, Cstate):
    "Private: Associate an underlying C state object with this entity"
    self.cstatus = Cstate

  # Requires overloading
  def isRunning(self):
    """Return true if this entity is running.  This indicates whether basic services are available, not whether the entity is 100%"""
    raise Unimplemented()

  def isProtected(self):
    """Return true if this service instance has at least 1 running standby assignment, None if not applicable"""
    return None

  def isIdle(self):
    """Return True if the entity is running but has no assigned work, or None if the concept of 'idle' is not applicable"""
    # The default case just returns not idle
    return None

  def isOk(self):
    """Return True if the entity is 100%"""
    # Note this strange method of checking != to the negative is because the functions are tri-state:
    # True, False, and None (which means Not Applicable)
    return ((self.isRunning() != False) and (self.isIdle() != True) and (self.isProtected() != False))

  # Pythonized versions of the underlying "C" data
  def adminState(self):
      """Return the administrative status of this entity"""
      if self.cconfig:
        return self.cconfig.adminState
      else: return None

  def enabled(self):
      """Return True if this entity is enabled, False if not enabled, or none if not applicable (for example the entity has not been applied into the AMF)."""
      if self.cstate:
        return self.cstate.operState == asp.CL_AMS_OPER_STATE_ENABLED
      else: return None


class AmfEntityStateChange:
  """Class describing the current state of an entity
  """
  def __init__(self):
    pass

  def setState(self,state):
    """@param state RunningState,IdleState,StoppedState
    """
    self.runState = StoppedState




class AmfNode(AmfEntity):
  """Class that models a SAF Node.
  A node is an individual machine; blade in a blade server or a pizza box in a rack.
  """

  # Member variables

  ## @var slot 
  # What slot this node is in (if a chassis is being used), or a unique small integer identifier for this node (from asp.conf)

  ## @var su
  #  List of SAF AMF service units that exist on this node.

  ## @var aspdir
  #  Location of ASP that is running on this node

  ## @var localIp
  #  IP address of this node on the cluster's private intranet

  ## @var localUser
  #  SSH username configured on this node (in the cluster's private intranet only).  Note that if SSH trusting is enabled between the nodes in the cluster, then this username does not have to be configured.  For use in software deployment.

  ## @var localPasswd
  #  SSH password configured on this node (in the cluster's private intranet only).  Note that if SSH trusting is enabled between the nodes in the cluster, then this password does not have to be configured.  For use in software deployment.

  ## Translation of some underlying enums to understandable strings
  classXlat={ asp.CL_AMS_NODE_CLASS_A: "A", asp.CL_AMS_NODE_CLASS_B: "Controller", asp.CL_AMS_NODE_CLASS_C: "Worker", asp.CL_AMS_NODE_CLASS_D: "D" }

  def __init__(self,name,slot):
    AmfEntity.__init__(self, NodeType, name)
    self.slot = int(slot)
    self.su = []
    self.aspdir  = "unknown"
    self.aspVersion = "unknown"
    self.localIp    = "unknown"
    self.localUser = "unknown"
    self.localPasswd = "unknown"

  def setIntraclusterAccess(self, ip,user,passwd,aspdir):
    """Set parameters needed to ssh and scp to this node from other nodes in the cluster.
    
    The best-practices cluster configuration is to have each node interconnected via an isolated ethernet network (and to have another interface for external connections).  This info pertains to access to the node through this network.  By doing it this way, you can ensure security via physical mechanisms.  Of course if you don't have an isolated intracluster network then you can use network-wide IP addresses, usernames, passwords, etc.  However, the cluster will not be considered as secure since the password you pass into this routine will be stored in cleartext in RAM and on disk on other nodes in the cluster.

    Note that this information is only needed for software deployment.  If you are not using the software deployment feature these values need not be set.  Also note that you don't need to pass in a valid password if you configure SSH security keys on each node to allow passwordless login from other nodes in the cluster.

    @param ip A string containing the intracluster IP address of this node
    @param user A string containing Linux username on this node
    @param passwd A string containing the password for the user
    @param aspdir String containing the path to ASP installation on this node (ex /root/asp)
    """
    if ip is not None: self.localIp     = ip
    if user is not None: self.localUser   = user
    if passwd is not None: self.localPasswd = passwd
    if aspdir is not None: self.aspdir      = aspdir

  def copyLocalState(self,entity):
    self.localIp     = entity.localIp
    self.localUser   = entity.localUser
    self.localPasswd = entity.localPasswd
    self.aspdir      = entity.aspdir
    

  def IsUp(self):
    """Deprecated, use isPresent() or isRunning()"""
    # A "down" blade will not have an assigned slot number.
    return self.slot != 0

  def isPresent(self):
    """Return true if this node is there"""
    return self.slot != 0

  def isRunning(self):
    """Return True if this node is running ASP"""
    # A "down" blade will not have an assigned slot number.
    return self.isPresent() and (self.cconfig.adminState != StoppedState)

  def isIdle(self):
    """Return True if it is running but has not assigned work"""
    return self.isRunning() and (self.cconfig.adminState == IdleState)

  def safClass(self):
    """Return the SAF class A,B,C, or D"""
    return self.classXlat[self.cconfig.classType]

  def subClass(self):
    """Return a string containing the subclass or '' if there is no subclass"""
    text = self.cconfig.subClassType.value
    text = text.strip() # remove whitespace since the AMF returns ' ' if there is no subclass
    if text == "NodeSubClassUndefined":  # AMF sometimes return this when no subclass
      return ""
    return text.strip()  

  def isActive(self,sg=None):
    """Return true if this node has any active SUs"""
    for su in self.su:
      if not sg or su.sg == sg:
        if su.isActive(): return True
    return False

  def isStandby(self,sg=None):
    """Return true if this node has any standby SUs"""
    for su in self.su:
      if not sg or su.sg == sg:
        if su.isStandby(): return True
    return False

  def haRole(self,sg=None):
    """Return 'none', 'active', 'standby', or 'active/standby' depending on the roles of SUs on this node"""
    active = self.isActive(sg) and 1 or 0  # Inline if
    standby = self.isStandby(sg) and 1 or 0    
    return ["none","active","standby","active/standby"][active + (standby*2)]

  def isProtected(self):
    """Return true if every SG on this node is protected"""
    for su in self.su:
        if su.isRunning():  # If the SU is turned off, it shouldn't affect the protected status
          if su.sg and (not su.sg.isProtected()) and su.sg.redundancyModel() != asp.CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY: return False
    return True

  def failureCount(self):
    """Return the number of SU failovers that have occurred on this node since the last reset"""
    return self.cstatus.suFailoverCount

  def unpickleAssociatedData(self,s):
    """Private function to unpack data about this node that will be stored in the AMF db"""
    try:
      (self.localIp,self.localUser,self.localPasswd,self.aspdir) = s.split()
    except ValueError, e:
      self.log("Unpickle of associated data on node %s failed.  Data is %s " % (self.name, s))
  
  def pickleAssociatedData(self):
    """Private function to pack up data about this node that will be stored in the AMF db"""
    return str("%s %s %s %s" % (self.localIp,self.localUser,self.localPasswd,self.aspdir))
   
  def __str__(self):
    return "AmfNode %s" % self.name
 
  # Allow dictionary access
  def __len__(self):
    return len(self.__dict__)

  def __getitem__(self, key):
    return self.__dict__[key]

  def __setitem__(self, key, value):
    self.__dict__[key] = value

  def __iter__(self):
    return None

  def __contains__(self, item):
    return item in self.__dict__


class AmfServiceGroup(AmfEntity):
  """Class that models a SAF Service Unit.

  A Service Group is essentially a highly available, scalable application (with processes running on many nodes).  See SAF docs for more details.
  """

  # Member variables

  ## @var slot 
  # What slot this node is in (if a chassis is being used), or a unique small integer identifier for this node (from asp.conf)

  ## @var su
  #  List of SAF AMF service units owned by this service group

  ## @var si
  #  List of SAF AMF service instances owned by this service group

  ## @var app
  #  Reference to the Application (@ref aspApp.App) that this service group is running. 
  #  Use of this variable requires that an applicaton database (@ref aspApp.AppDb) has been created and connected to the clusterinf through the (@ref clusterinfo.ClusterInfo.setAppDb) function.
  #  If the service group was not deployed via a bundle (for example it is intrinsic to the model -- that is, IDE configured), or the application database is not created and connected, then this variable will be None

  ## @var appVer
  #  Reference to the software bundle (@ref aspApp.AppVer) that this service group is running.
  #  Use of this variable requires that an applicaton database (@ref aspApp.AppDb) has been created and connected to the clusterinf through the (@ref clusterinfo.ClusterInfo.setAppDb) function.
  #  If the service group was not deployed via a bundle (for example it is intrinsic to the model -- that is, IDE configured), or the application database is not created and connected, then this variable will be None

  def __init__(self,name):
    AmfEntity.__init__(self, ServiceGroupType, name)
    self.su = []
    self.si = []
    self.app = None
    self.appVer = None

    # Pythonized versions of the underlying "C" data

  def redundancyModel(self):
      """Return the redundancy model (integer enum)"""
      try:
        if self.cconfig:
          return self.cconfig.redundancyModel
      except AttributeError:
        return self.rModel
      
  def isRunning(self):
      """Return true if this service group is running"""
      ret = ((self.cconfig.adminState != StoppedState) and (self.cstatus.isStarted == 1) and (len(self.su)>0))
      if not ret: return ret
      for su in self.su: # Additionally, at least 1 SU should be running
        if su.isRunning(): return True

# This is more like "is up": and ((self.cstatus.numCurrActiveSUs > 0) or (self.cstatus.numCurrStandbySUs >= 1))
# I need cconfig.instantiatedSUList.numEntries > 0, but I don't have access to this list (isValid = FALSE)

  def isIdle(self):
      """Return true if this service group is running but no work is assigned"""
      return ((self.cstatus.isStarted == 1) and (self.cconfig.adminState == IdleState))

  def isProtected(self):
      """Return true if this service group has at least 1 running standby Service Unit"""
      if self.redundancyModel() == asp.CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY: return None  # Not Applicable
      return (self.cstatus.numCurrStandbySUs >= 1)

  def activeSu(self):
      """Return the active service unit as a ServiceUnit object, or None if there is none"""
      for su in self.su:
        if su.isActive(): return su
      return None

  def nodes(self):
      """Return a list of nodes that this Service Group is configured on"""
      return [x.node for x in self.su]

  def app(self):
      """return the application that this SG is running (aspApp.App object)"""
      return self.app
  
  def appVer(self):
      """return the specific application version instance that this SG is running (aspApp.AppFile object)"""
      return self.appVer

  
class AmfServiceUnit(AmfEntity):
  """Class that models a SAF Service Unit.

  Essentially a Service Unit is the actualization on a particular node of a SAF Service Group.
  """

  # Member variables

  ## @var slot 
  # What slot this node is in (if a chassis is being used), or a unique small integer identifier for this node (from asp.conf)

  ## @var sg
  #  The SAF AMF service group that owns by this service unit

  ## @var si
  #  List of SAF AMF service instances owned by this service unit

  ## @var node
  #  The node that this Service Unit is configured to run on

  ## @var comp
  #  List of components "owned" by this service unit

  ## @var rank
  #  This service unit's rank.  For information about rank ordering, see the SAF AMF spec (www.saforum.org)

  def __init__(self,name, sg=None,node=None):
    AmfEntity.__init__(self, ServiceUnitType, name )
    self.sg   = sg
    self.node = node
    self.si   = []
    self.comp = []
    self.rank = 0

  def isRunning(self):
      """Return true if this service group is running"""
      # Not sure which is correct
      # return (self.cstatus.operState == asp.CL_AMS_OPER_STATE_ENABLED)
      #return (self.cstatus.readinessState == asp.CL_AMS_READINESS_STATE_INSERVICE)
      return self.cstatus.presenceState == asp.CL_AMS_PRESENCE_STATE_INSTANTIATED

  def isIdle(self):
      """Return true if this service group is running"""
      return (self.cstatus.operState==asp.CL_AMS_OPER_STATE_ENABLED) and (self.cconfig.adminState == IdleState)

  def isActive(self):
      """Return true if this SU has SIs assigned 'active' to it"""
      return (self.cstatus.numActiveSIs > 0)

  def isStandby(self):
      """Return true if this SU has SIs assigned 'standby' to it"""
      return (self.cstatus.numStandbySIs > 0)

  def haRole(self):
    """Return 'none', 'active', 'standby', or 'active/standby' depending on the roles of SUs on this node"""
    active = self.isActive() and 1 or 0
    standby = self.isStandby() and 1 or 0    
    return ["none","active","standby","active/standby"][active + (standby*2)]


class AmfServiceInstance(AmfEntity):
  """Class that models a SAF Service Instance; essentially this is a work assignment
  """

  # Member variables

  ## @var sg
  #  The SAF AMF service group that owns by this service instance

  ## @var csi
  #  List of the SAF component service instances (CSI) that are owned by this SI.

  ## @var activeSu
  #  Which Service Unit is configured "Active" for this SI (or None if there is no active)

  ## @var standbySu
  #  Which Service Unit is configured "Standby" for this SI (or None if there is no standby)

  ## @var rank
  #  This service instance's rank.  For information about rank ordering, see the SAF AMF spec (www.saforum.org)

  def __init__(self,name):
    AmfEntity.__init__(self,ServiceInstanceType,name)
    self.numStandbyAssignments = 1  # Default
    self.sg  = None  # What SG is this SI in?
    self.csi = []
    self.rank = 0
    self.activeSu = None
    self.standbySu = None

  def isRunning(self):
      """Return true if this service instance is assigned"""
      return (self.cstatus.numActiveAssignments >= 1)

  def isIdle(self):
      """Return true if this service instance is not assigned"""
      return (self.cconfig.adminState == IdleState)

  def isProtected(self):
      """Return true if this service instance has at least 1 running standby assignment"""
      return (self.cstatus.numStandbyAssignments >= 1)

  def redundancyModel(self):
      """Return the redundancy model (integer enum).
         The redundancy model of the work is that of the parent SG"""
      return self.sg.redundancyModel()

  def ActiveNode(self):
    """Return what node (AmfNode object) this SI is assigned active on, or None if it is not active anywhere"""
    return self.activeSu and self.activeSu.node or None

  def StandbyNode(self):
    """Return what node (AmfNode object) this SI is assigned standby on, or None if it is not standby anywhere"""
    if self.standbySu: return self.standbySu.node
    return None


class AmfComponentServiceInstance(AmfEntity):
  """Class that models a SAF Component Service Instance; essentially the part of a work assignment (SAF SI) that gets assigned to a particular process (SAF component)
  """

  # Member variables

  ## @var component
  # The component that this CSI is assigned to
 
  ## @var si
  # The parent service instance that owns this CSI

  ## @var kvdict
  # The key/value pairs that define this CSI (SAF calls them name/value pairs (nvps))

  def __init__(self,name,kvdict,csiType=None):
    AmfEntity.__init__(self,ComponentServiceInstanceType,name)
    if csiType==None:
      self.csiType   = name  # By default give it its own type
    else: self.csiType = csiType
    self.component = None
    self.si        = None
    assert((kvdict is None) or (type(kvdict) is types.InstanceType) or (type(kvdict) is types.DictType))
    self.kvdict    = kvdict

  def isRunning(self):
    """Return true if this service instance is assigned"""
    return self.si.isRunning()

  def isIdle(self):
      """Return true if this service instance is not assigned"""
      return self.si.isIdle()

  def isProtected(self):
      """Return true if this service instance has at least 1 running standby assignment"""
      return self.si.isProtected()

  def redundancyModel(self):
      """Return the redundancy model (integer enum).
         The redundancy model of the work is that of the parent SG"""
      return self.si.redundancyModel()

  def ActiveNode(self):
    """Return what node (AmfNode object) this CSI is assigned active on, or None if it is not active anywhere"""
    return self.si.ActiveNode()

  def StandbyNode(self):
    """Return what node (AmfNode object) this CSI is assigned standby on, or None if it is not standby anywhere"""
    return self.si.StandbyNode()


  

class AmfComponent(AmfEntity):
  """Class that models a SAF component; essentially a process
  """

  # Member variables

  ## @var su
  # The service unit that "owns" this component

  ## @var supportedCsis
  # The set of CSI types that can be assigned to this component

  ## @var timeouts
  # The component health timeouts -- this variable should be used for assignment only
  # for current state, use self.cconfig

  def __init__(self,name, compType=None):
    AmfEntity.__init__(self,ComponentType,name)

    self.su = None
    self.supportedCsis = set()

    # These Python level entities will be written to the component if the variable exists.
    self.timeouts = asp.ClAmsCompTimerDurationsT()
    self.timeouts.instantiate = 30000
    self.timeouts.terminate = 30000
    self.timeouts.cleanup = 30000
    self.timeouts.quiescingComplete = 30000
    self.timeouts.csiSet = 30000
    self.timeouts.csiRemove = 30000
    self.timeouts.instantiateDelay = 10000

  # Pythonized versions of the underlying "C" data
  def isRunning(self):
    """Return true if it is running"""
    return self.cstatus.presenceState == asp.CL_AMS_PRESENCE_STATE_INSTANTIATED

  def isIdle(self):
    """Return if it is running but has not assigned work"""
    return (self.cstatus.readinessState == asp.CL_AMS_READINESS_STATE_OUTOFSERVICE)
    #return self.isRunning() and (self.cstatus.numActiveCSIs == 0) and (self.cstatus.numStandbyCSIs == 0)

  def isActive(self):
      """Return true if this CSIs assigned 'active' to it"""
      return (self.cstatus.numActiveCSIs > 0)

  def isStandby(self):
      """Return true if this CSIs assigned 'standby' to it"""
      return (self.cstatus.numStandbyCSIs > 0)

  def haRole(self):
    """Return 'none', 'active', 'standby', or 'active/standby' depending on the roles of SUs on this node"""
    active = self.isActive() and 1 or 0
    standby = self.isStandby() and 1 or 0    
    return ["none","active","standby","active/standby"][active + (standby*2)]

  def adminState(self):
      """Return the administrative status of this entity"""
      return self.su.adminState()  # Component's admin state is SU's admin state

  def failureCount(self):
    """Return the number of failures that have occurred on this component since the last failover"""
    return self.cstatus.restartCount

  def command(self):
    """What command was run to start this up"""
    return self.cconfig.instantiateCommand
