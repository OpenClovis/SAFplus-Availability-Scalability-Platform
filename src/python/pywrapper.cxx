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
  PyObject* pyBuf = PyBuffer_FromReadWriteMemory(self.data, self.len());
  object retval = object(handle<>(pyBuf));
  return retval;
}

static object ckpt_get(Checkpoint& self, char* key)
{
  const Buffer& b = self.read(key);
  if (&b)
    {
      uint_t len = b.len();
      if (b.isNullT()) len--;
      PyObject* pyBuf = PyBuffer_FromMemory((void*)b.data, len);
      object retval = object(handle<>(pyBuf));
      return retval;
    }
  PyErr_SetString(PyExc_KeyError, key);
  throw_error_already_set();
  return object();
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
  def("amfNodeRestart",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&SAFplus::amfNodeRestart));
}
