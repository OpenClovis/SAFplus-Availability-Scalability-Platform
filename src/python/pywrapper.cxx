/* Added cltypes.h to Fix the PRIx64 error */
#include <cltypes.h>
#include <pthread.h>
#include <boost/python.hpp>

#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clCommon.hxx>
#include <clHandleApi.hxx>
#include <clTransaction.hxx>

#include <clCkptApi.hxx>
#include <clMgtApi.hxx>

#include <clNameApi.hxx>
#include <clCustomization.hxx>

#include <clAmfMgmtApi.hxx>
#include <clGroupCliApi.hxx>

using namespace SAFplus;
using namespace boost::python;

static object buffer_get(Buffer& self)
{
  //PyObject* pyBuf = PyBuffer_FromReadWriteMemory(self.data, self.len());
  Py_buffer pyBuf;
  int rc = PyBuffer_FillInfo(&pyBuf, NULL, self.data, self.len(), true, PyBUF_CONTIG_RO);
  PyObject* pyObj = PyMemoryView_FromBuffer(&pyBuf);
  object retval = object(handle<>(pyObj));
  //object retVal(handle<>(pyBuf));
  return retval;
}

static object ckpt_get(Checkpoint& self, char* key)
{
  const Buffer& b = self.read(key);
  if (&b)
    {
      uint_t len = b.len();
      if (b.isNullT()) len--;
      //PyObject* pyBuf = PyBuffer_FromMemory((void*)b.data, len);
      //PyObject* pyBuf = PyMemoryView_FromBuffer((void*)b.data, len, PyBUF_READ);
      Py_buffer pyBuf;
      int rc = PyBuffer_FillInfo(&pyBuf, NULL, (void*)b.data, len, true, PyBUF_CONTIG_RO);
      PyObject* pyObj = PyMemoryView_FromBuffer(&pyBuf);
      object retval = object(handle<>(pyObj));
      return retval;
    }
  PyErr_SetString(PyExc_KeyError, key);
  throw_error_already_set();
  return object();
}

static dict nodeGetConfig(const Handle & self, const std::string & key)
{
    // get information into nodeConfig
    SAFplus::Rpc::amfMgmtRpc::NodeConfig* nodeConfig;
    ClRcT rc = amfMgmtNodeGetConfig(self, key, &nodeConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["adminstate"] = std::to_string(nodeConfig->adminstate());
    dictionary["autoRepair"] = std::to_string(nodeConfig->autorepair());
//    dictionary["canBeInherited"] = std::to_string(nodeConfig->canBeInherited());
//    dictionary["currentRecovery"] = std::to_string(nodeConfig->currentRecovery());
//    dictionary["disableAssignmentOn"] = std::to_string(nodeConfig->disableAssignmentOn());
    dictionary["failFastOnCleanupFailure"] = std::to_string(nodeConfig->failfastoncleanupfailure());
    dictionary["failFastOnInstantiationFailure"] = std::to_string(nodeConfig->failfastoninstantiationfailure());
//    dictionary["id"] = std::to_string(nodeConfig->id());
//    dictionary["lastSUFailure"] = std::to_string(nodeConfig->lastSUFailure());
    dictionary["name"] = nodeConfig->name();
//    dictionary["restartable"] = std::to_string(nodeConfig->restartable());
//    dictionary["serviceUnitFailureEscalationPolicy"] = std::to_string(nodeConfig->serviceUnitFailureEscalationPolicy());
//    dictionary["serviceUnits"] = nodeConfig->serviceunits();
//    dictionary["userDefinedType"] = std::to_string(nodeConfig->userDefinedType());

    return dictionary;
}

static dict SGGetConfig(const Handle & self, const std::string & key)
{
    // get information into SGConfig
    SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* SGConfig;
    ClRcT rc = amfMgmtSGGetConfig(self, key, &SGConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["adminstate"] = std::to_string(SGConfig->adminstate());
    dictionary["autoAdjust"] = std::to_string(SGConfig->autoadjust());
//    dictionary["autoAdjustInterval"] = std::to_string(SGConfig->autoadjustinterval());
    dictionary["autoRepair"] = std::to_string(SGConfig->autorepair());
//    dictionary["componentRestart"] = std::to_string(SGConfig->componentRestart());
//    dictionary["id"] = std::to_string(SGConfig->id());
    dictionary["maxActiveWorkAssignments"] = std::to_string(SGConfig->maxactiveworkassignments());
    dictionary["maxStandbyWorkAssignments"] = std::to_string(SGConfig->maxstandbyworkassignments());
    dictionary["name"] = SGConfig->name();
    dictionary["preferredNumActiveServiceUnits"] = std::to_string(SGConfig->preferrednumactiveserviceunits());
    dictionary["preferredNumIdleServiceUnits"] = std::to_string(SGConfig->preferrednumidleserviceunits());
    dictionary["preferredNumStandbyServiceUnits"] = std::to_string(SGConfig->preferrednumstandbyserviceunits());
//    dictionary["serviceInstances"] = std::to_string(SGConfig->serviceInstances());
//    dictionary["serviceUnitRestart"] = std::to_string(SGConfig->serviceUnitRestart());
//    dictionary["serviceUnits"] = std::to_string(SGConfig->serviceUnits());

    return dictionary;
}

static dict SUGetConfig(const Handle & self, const std::string & key)
{
    // get information into SUConfig
    SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* SUConfig;
    ClRcT rc = amfMgmtSUGetConfig(self, key, &SUConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["adminstate"] = std::to_string(SUConfig->adminstate());
//    dictionary["assignedServiceInstances"] = std::to_string(SUConfig->assignedServiceInstances());
//    dictionary["compRestartCount"] = std::to_string(SUConfig->compRestartCount());
//    dictionary["components"] = std::to_string(SUConfig->components());
//    dictionary["currentRecovery"] = std::to_string(SUConfig->currentRecovery());
    dictionary["failover"] = std::to_string(SUConfig->failover());
//    dictionary["id"] = std::to_string(SUConfig->id());
//    dictionary["lastCompRestart"] = std::to_string(SUConfig->lastCompRestart());
//    dictionary["lastRestart"] = std::to_string(SUConfig->lastRestart());
    dictionary["name"] = SUConfig->name();
    dictionary["node"] = SUConfig->node();
    dictionary["rank"] = std::to_string(SUConfig->rank());
//    dictionary["restartable"] = std::to_string(SUConfig->restartable());
//    dictionary["saAmfSUHostNodeOrNodeGroup"] = std::to_string(SUConfig->saAmfSUHostNodeOrNodeGroup());
//    dictionary["serviceGroup"] = std::to_string(SUConfig->serviceGroup());

    return dictionary;
}

static dict SIGetConfig(const Handle & self, const std::string & key)
{
    // get information into SIConfig
    SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* SIConfig;
    ClRcT rc = amfMgmtSIGetConfig(self, key, &SIConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["adminstate"] = std::to_string(SIConfig->adminstate());
//    dictionary["componentServiceInstances"] = std::to_string(SIConfig->componentServiceInstances());
//    dictionary["id"] = std::to_string(SIConfig->id());
//    dictionary["isFullActiveAssignment"] = std::to_string(SIConfig->isFullActiveAssignment());
//    dictionary["isFullStandbyAssignment"] = std::to_string(SIConfig->isFullStandbyAssignment());
    dictionary["name"] = SIConfig->name();
    dictionary["preferredActiveAssignments"] = std::to_string(SIConfig->preferredactiveassignments());
    dictionary["preferredStandbyAssignments"] = std::to_string(SIConfig->preferredstandbyassignments());
    dictionary["rank"] = std::to_string(SIConfig->rank());
//    dictionary["serviceGroup"] = std::to_string(SIConfig->serviceGroup());

    return dictionary;
}

static dict CSIGetConfig(const Handle & self, const std::string & key)
{
    // get information into CSIConfig
    SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* CSIConfig;
    ClRcT rc = amfMgmtCSIGetConfig(self, key, &CSIConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
//    dictionary["dependencies"] = std::to_string(CSIConfig->dependencies());
//    dictionary["id"] = std::to_string(CSIConfig->id());
    dictionary["name"] = CSIConfig->name();
    dictionary["serviceInstance"] = CSIConfig->serviceinstance();
//    dictionary["type"] = std::to_string(CSIConfig->type());

    return dictionary;
}

static dict CSIGetStatus(const Handle & self, const std::string & key)
{
    // get information into CSIStatus
    SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceStatus* CSIStatus;
    ClRcT rc = amfMgmtCSIGetStatus(self, key, &CSIStatus);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
//    dictionary["activeComponents"] = std::to_string(CSIStatus->activeComponents());
//    dictionary["isProxyCSI"] = CSIStatus->isProxyCSI();
//    dictionary["standbyComponents"] = std::to_string(CSIStatus->standbycomponents());

    return dictionary;
}

static dict ComponentGetConfig(const Handle & self, const std::string & key)
{
    // get information into ComponentConfig
    SAFplus::Rpc::amfMgmtRpc::ComponentConfig* ComponentConfig;
    ClRcT rc = amfMgmtComponentGetConfig(self, key, &ComponentConfig);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["capabilityModel"] = std::to_string(ComponentConfig->capabilitymodel());
//    dictionary["cleanup"] = std::to_string(ComponentConfig->cleanup());
//    dictionary["csiType"] = std::to_string(ComponentConfig->csiType());
//    dictionary["currentRecovery"] = std::to_string(ComponentConfig->currentRecovery());
    dictionary["delayBetweenInstantiation"] = std::to_string(ComponentConfig->delaybetweeninstantiation());
//    dictionary["id"] = std::to_string(ComponentConfig->id());
//    dictionary["instantiate"] = std::to_string(ComponentConfig->instantiate());
//    dictionary["instantiateLevel"] = std::to_string(ComponentConfig->instantiateLevel());
    dictionary["instantiationSuccessDuration"] = std::to_string(ComponentConfig->instantiationsuccessduration());
    dictionary["maxActiveAssignments"] = std::to_string(ComponentConfig->maxactiveassignments());
    dictionary["maxDelayedInstantiations"] = std::to_string(ComponentConfig->maxdelayedinstantiations());
    dictionary["maxInstantInstantiations"] = std::to_string(ComponentConfig->maxinstantinstantiations());
    dictionary["maxStandbyAssignments"] = std::to_string(ComponentConfig->maxstandbyassignments());
    dictionary["name"] = ComponentConfig->name();
//    dictionary["proxied"] = std::to_string(ComponentConfig->proxied());
//    dictionary["proxyCSI"] = std::to_string(ComponentConfig->proxyCSI());
    dictionary["recovery"] = ComponentConfig->recovery();
    dictionary["restartable"] = std::to_string(ComponentConfig->restartable());
//    dictionary["serviceUnit"] = std::to_string(ComponentConfig->serviceUnit());
//    dictionary["terminate"] = std::to_string(ComponentConfig->terminate());
//    dictionary["timeouts"] = std::to_string(ComponentConfig->timeouts());

    return dictionary;
}

static ClRcT addSUIntoNode(const Handle & mgmtHandle, const std::string & nodeName, const std::string & suName)
{
    ClRcT rc = CL_OK;
    SAFplus::Rpc::amfMgmtRpc::NodeConfig* node = new SAFplus::Rpc::amfMgmtRpc::NodeConfig();
    node->set_name(nodeName.c_str());
    node->add_serviceunits(suName.c_str(), strlen(suName.c_str()));
    rc = SAFplus::amfMgmtNodeConfigSet(mgmtHandle,node);
    return rc;
}

static ClRcT addSUIntoSG(const Handle & mgmtHandle, const std::string & sgName, const std::string & suName)
{
    ClRcT rc = CL_OK;
    SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg = new SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig();
    sg->set_name(sgName.c_str());
    sg->add_serviceunits(suName.c_str(), strlen(suName.c_str()));
    rc = SAFplus::amfMgmtServiceGroupConfigSet(mgmtHandle,sg);
    return rc;
}

static ClRcT addCompIntoSU(const Handle & mgmtHandle, const std::string & suName, const std::string & compName)
{
    ClRcT rc = CL_OK;
    SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su = new SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig();
    su->set_name(suName.c_str());
    su->add_components(compName.c_str(), strlen(compName.c_str()));
    rc = SAFplus::amfMgmtServiceUnitConfigSet(mgmtHandle,su);
    return rc;
}

BOOST_PYTHON_MODULE(pySAFplus)
{
  logInitialize();
  utilsInitialize();

  // SAFplus initialization
  class_<SafplusInitializationConfiguration>("SafplusInitializationConfiguration")
    .def_readwrite("port", &SafplusInitializationConfiguration::iocPort)
    .def_readwrite("msgQueueLen", &SafplusInitializationConfiguration::msgQueueLen)
    .def_readwrite("msgThreads", &SafplusInitializationConfiguration::msgThreads);

  enum_<LibDep::Bits>("Libraries")
    .value("LOG",LibDep::Bits::LOG)
    .value("OSAL",LibDep::Bits::OSAL)
    .value("UTILS",LibDep::Bits::UTILS)
    .value("IOC",LibDep::Bits::IOC)
    .value("MSG",LibDep::Bits::MSG)
    .value("OBJMSG",LibDep::Bits::OBJMSG)
    .value("GRP",LibDep::Bits::GRP)
    .value("CKPT",LibDep::Bits::CKPT)
    .value("FAULT",LibDep::Bits::FAULT)
    .value("NAME",LibDep::Bits::NAME)
    .value("HEAP",LibDep::Bits::HEAP)
    .value("BUFFER",LibDep::Bits::BUFFER)
    .value("TIMER",LibDep::Bits::TIMER)
    .value("DBAL",LibDep::Bits::DBAL)
    .value("MGT_ACCESS",LibDep::Bits::MGT_ACCESS);
  def("Initialize",SAFplus::safplusInitialize);
  def("Finalize",SAFplus::safplusFinalize);


  // Basic Logging

  def("logMsgWrite",SAFplus::logStrWrite);
  enum_<LogSeverity>("LogSeverity")
    .value("EMERGENCY", LOG_SEV_EMERGENCY)
    .value("ALERT", LOG_SEV_ALERT)
    .value("CRITICAL", LOG_SEV_CRITICAL)
    .value("ERROR", LOG_SEV_ERROR)
    .value("WARNING", LOG_SEV_WARNING)
    .value("NOTICE", LOG_SEV_NOTICE)
    .value("INFO", LOG_SEV_INFO)
    .value("DEBUG", LOG_SEV_DEBUG)
    .value("TRACE", LOG_SEV_TRACE)
    ;

 
  // Handles

  enum_<HandleType>("HandleType")
    .value("Pointer",PointerHandle)
    .value("Transient",TransientHandle)
    .value("Persistent",PersistentHandle);

  class_<Handle>("Handle")
    .def(init<HandleType,uint64_t, uint32_t,uint16_t,uint_t>())
        .def("getNode", &Handle::getNode)
        .def("getCluster", &Handle::getCluster)
        .def("getProcess", &Handle::getProcess)
        .def("getIndex", &Handle::getIndex)
        .def("getType", &Handle::getType)
        .def("getPort", &Handle::getPort)
        .def("create",static_cast< Handle (*)(int) > (&Handle::create))
    .staticmethod("create");

  class_<WellKnownHandle,bases<Handle> >("WellKnownHandle",no_init)        
        .def("getNode", &WellKnownHandle::getNode)
        .def("getCluster", &WellKnownHandle::getCluster)
        .def("getProcess", &WellKnownHandle::getProcess)
        .def("getIndex", &WellKnownHandle::getIndex)
        .def("getType", &WellKnownHandle::getType);

  class_<Transaction>("Transaction")
    .def("commit",&Transaction::commit)
    .def("abort",&Transaction::abort);

  // Expose global variables
  boost::python::scope().attr("SYS_LOG") = SYS_LOG;   
  boost::python::scope().attr("APP_LOG") = APP_LOG;   
  boost::python::scope().attr("logSeverity") = SAFplus::logSeverity;

  // Checkpoint

  class_<Buffer,boost::noncopyable>("Buffer", no_init)
    .def("len",&Buffer::len)
    //.def_readwrite("data",&Buffer::data);
    //.add_property("rovalue",&Buffer::get,return_internal_reference<1 >());
    //.def("get",&Buffer::get,return_internal_reference<1,with_custodian_and_ward<1, 1> >());
    .def("get",&buffer_get, (arg("self")), "doc");

  void (Checkpoint::*wss) (const std::string&, const std::string&,bool overwrite,Transaction&) = &Checkpoint::write;
  //Buffer& (Checkpoint::*rcs) (const char*) const = &Checkpoint::read;

  class_<Checkpoint,boost::noncopyable> ckpt("Checkpoint",init<uint_t, uint_t, uint_t>());

  ckpt.def(init<Handle,unsigned int, unsigned int, unsigned int>());
  ckpt.def("write", wss);
  ckpt.def("read", &ckpt_get,(arg("self"),arg("key")), "doc");
  ckpt.attr("REPLICATED") = (int) Checkpoint::REPLICATED;
  ckpt.attr("NESTED") = (int) Checkpoint::NESTED;
  ckpt.attr("PERSISTENT") = (int) Checkpoint::PERSISTENT;
  ckpt.attr("NOTIFIABLE") = (int) Checkpoint::NOTIFIABLE;
  ckpt.attr("SHARED") = (int) Checkpoint::SHARED;
  ckpt.attr("RETAINED") = (int) Checkpoint::RETAINED;
  ckpt.attr("LOCAL") = (int) Checkpoint::LOCAL;
  ckpt.attr("VARIABLE_SIZE") = (int) Checkpoint::VARIABLE_SIZE;
  ckpt.attr("EXISTING") = (int) Checkpoint::EXISTING;

  enum_<SAFplus::Rpc::amfMgmtRpc::Recovery>("Recovery")
    .value("Recovery_NoRecommendation",SAFplus::Rpc::amfMgmtRpc::Recovery_NoRecommendation)
    .value("Recovery_Restart",SAFplus::Rpc::amfMgmtRpc::Recovery_Restart)
    .value("Recovery_Failover",SAFplus::Rpc::amfMgmtRpc::Recovery_Failover)
    .value("Recovery_NodeSwitchover",SAFplus::Rpc::amfMgmtRpc::Recovery_NodeSwitchover)
    .value("Recovery_NodeFailover",SAFplus::Rpc::amfMgmtRpc::Recovery_NodeFailover)
    .value("Recovery_NodeFailfast",SAFplus::Rpc::amfMgmtRpc::Recovery_NodeFailfast)
    ;

  //class_<Checkpoint>("Checkpoint",init<Handle,unsigned int flags, unsigned int size, unsigned int rows>())
  //    .def(init<uint_t flags, uint_t size, uint_t rows>);
  //.def(write, &Checkpoint::write)
  //  .def(read, &Checkpoint::read);

  // Management information access

  def("mgtGet",static_cast< std::string (*)(const std::string&) > (&SAFplus::mgtGet));
  def("mgtSet",static_cast< ClRcT (*)(const std::string&,const std::string&) > (&SAFplus::mgtSet)); 
  def("mgtCreate",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtCreate));
  def("mgtDelete",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtDelete));

  def("getProcessHandle",static_cast< Handle (*)(int pid, int nodeNum) > (&SAFplus::getProcessHandle));
  def("mgtGet",static_cast< std::string (*)(Handle src, const std::string&) > (&SAFplus::mgtGet));

  def("amfMgmtInitialize",static_cast< ClRcT (*)(Handle &) > (&SAFplus::amfMgmtInitialize));
  def("amfMgmtFinalize",static_cast< ClRcT (*)(const Handle &) > (&SAFplus::amfMgmtFinalize));
  def("nameInitialize", &SAFplus::nameInitialize);

  def("amfMgmtNodeLockAssignment",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeLockAssignment));
  def("amfMgmtSGLockAssignment",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSGLockAssignment));
  def("amfMgmtSULockAssignment",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSULockAssignment));
  def("amfMgmtSILockAssignment",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSILockAssignment));

  def("amfMgmtNodeLockInstantiation",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeLockInstantiation));
  def("amfMgmtSGLockInstantiation",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSGLockInstantiation));
  def("amfMgmtSULockInstantiation",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSULockInstantiation));

  def("amfMgmtNodeUnlock",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeUnlock));
  def("amfMgmtSGUnlock",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSGUnlock));
  def("amfMgmtSUUnlock",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSUUnlock));
  def("amfMgmtSIUnlock",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSIUnlock));

  def("amfMgmtNodeRepair",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeRepair));
  def("amfMgmtCompRepair",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtCompRepair));
  def("amfMgmtSURepair",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSURepair));

  def("amfMgmtNodeRestart",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeRestart));
  def("amfMgmtSURestart",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSURestart));
  def("amfMgmtCompRestart",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtCompRestart));

  def("grpCliClusterViewGet",&SAFplus::grpCliClusterViewGet);

  def("amfMgmtSGAdjust",static_cast< ClRcT (*)(const Handle &, const std::string &, bool) > (&SAFplus::amfMgmtSGAdjust));

  def("amfMgmtSISwap",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSISwap));

  def("amfMgmtCompErrorReport",static_cast< ClRcT (*)(const Handle &, const std::string &, SAFplus::Rpc::amfMgmtRpc::Recovery recommendedRecovery) > (&SAFplus::amfMgmtCompErrorReport));

  def("amfMgmtNodeErrorReport",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeErrorReport));
  def("amfMgmtNodeErrorClear",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeErrorClear));
  def("amfMgmtNodeJoin",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfNodeJoin));
  def("amfMgmtNodeShutdown",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtNodeShutdown));
  def("amfNodeRestart",static_cast< ClRcT (*)(const Handle &, const std::string &, bool) > (&SAFplus::amfNodeRestart));
  def("amfMiddlewareRestart",static_cast< ClRcT (*)(const Handle &, const std::string &, bool, bool) > (&SAFplus::amfMiddlewareRestart));
  def("amfMgmtAssignSUtoSI",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &) > (&SAFplus::amfMgmtAssignSUtoSI));
  def("amfMgmtForceLockInstantiation",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtForceLockInstantiation));

  def("amfMgmtCompAddressGet",static_cast< Handle (*)(const std::string &) > (&SAFplus::amfMgmtCompAddressGet));

  def("amfMgmtComponentGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&ComponentGetConfig));
  def("amfMgmtNodeGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&nodeGetConfig));
  def("amfMgmtSGGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SGGetConfig));
  def("amfMgmtSUGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SUGetConfig));
  def("amfMgmtSIGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SIGetConfig));
  def("amfMgmtCSIGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&CSIGetConfig));
  def("amfMgmtCSIGetStatus",static_cast< dict (*)(const Handle &, const std::string &) > (&CSIGetStatus));


  def("addSUIntoNode",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addSUIntoNode));
  def("addSUIntoSG",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addSUIntoSG));
  def("addCompIntoSU",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addCompIntoSU));
  def("amfMgmtCommit",static_cast< ClRcT (*)(const Handle &) > (&amfMgmtCommit));
}
