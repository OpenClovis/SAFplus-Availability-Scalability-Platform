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
#include <clFaultApi.hxx>
//#include <FaultSeverity.hxx>

using namespace SAFplus;
using namespace FaultEnums;
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

  void (Fault::*notify)(FaultEventData, FaultPolicy) = &Fault::notify;
  void (Fault::*faultInit) (Handle, Handle, int, Wakeable&) = &Fault::init;
  void (Fault::*faultInit2) (Handle) = &Fault::init;
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

  enum_<SAFplus::FaultState>("FaultState")
      .value("STATE_UNDEFINED", SAFplus::FaultState::STATE_UNDEFINED)
      .value("STATE_UP", SAFplus::FaultState::STATE_UP)
      .value("STATE_DOWN", SAFplus::FaultState::STATE_DOWN);

  enum_<SAFplus::FaultPolicy>("FaultPolicy")
      .value("Undefined", SAFplus::FaultPolicy::Undefined)
      .value("Custom", SAFplus::FaultPolicy::Custom)
      .value("AMF", SAFplus::FaultPolicy::AMF);

  enum_<FaultEnums::FaultAlarmState>("FaultAlarmState")
      .value("ALARM_STATE_CLEAR", FaultEnums::FaultAlarmState::ALARM_STATE_CLEAR)
      .value("ALARM_STATE_ASSERT", FaultEnums::FaultAlarmState::ALARM_STATE_ASSERT)
      .value("ALARM_STATE_SUPPRESSED", FaultEnums::FaultAlarmState::ALARM_STATE_SUPPRESSED)
      .value("ALARM_STATE_UNDER_SOAKING", FaultEnums::FaultAlarmState::ALARM_STATE_UNDER_SOAKING)
      .value("ALARM_STATE_INVALID", FaultEnums::FaultAlarmState::ALARM_STATE_INVALID);

  enum_<FaultEnums::AlarmCategory>("AlarmCategory")
      .value("ALARM_CATEGORY_INVALID", FaultEnums::AlarmCategory::ALARM_CATEGORY_INVALID)
      .value("ALARM_CATEGORY_COMMUNICATIONS", FaultEnums::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS)
      .value("ALARM_CATEGORY_QUALITY_OF_SERVICE", FaultEnums::AlarmCategory::ALARM_CATEGORY_QUALITY_OF_SERVICE)
      .value("ALARM_CATEGORY_PROCESSING_ERROR", FaultEnums::AlarmCategory::ALARM_CATEGORY_PROCESSING_ERROR)
      .value("ALARM_CATEGORY_EQUIPMENT", FaultEnums::AlarmCategory::ALARM_CATEGORY_EQUIPMENT)
      .value("ALARM_CATEGORY_ENVIRONMENTAL", FaultEnums::AlarmCategory::ALARM_CATEGORY_ENVIRONMENTAL);

  enum_<FaultEnums::FaultProbableCause>("FaultProbableCause")
      .value("ALARM_ID_INVALID", FaultEnums::FaultProbableCause::ALARM_ID_INVALID)
      .value("ALARM_PROB_CAUSE_LOSS_OF_SIGNAL", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_LOSS_OF_SIGNAL)
      .value("ALARM_PROB_CAUSE_LOSS_OF_FRAME", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_LOSS_OF_FRAME)
      .value("ALARM_PROB_CAUSE_FRAMING_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_FRAMING_ERROR)
      .value("ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR)
      .value("ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR)
      .value("ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR)
      .value("ALARM_PROB_CAUSE_DEGRADED_SIGNAL", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_DEGRADED_SIGNAL)
      .value("ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE)
      .value("ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR)
      .value("ALARM_PROB_CAUSE_LAN_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_LAN_ERROR)
      .value("ALARM_PROB_CAUSE_DTE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_DTE)
      .value("ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE)
      .value("ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED)
      .value("ALARM_PROB_CAUSE_BANDWIDTH_REDUCED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_BANDWIDTH_REDUCED)
      .value("ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE)
      .value("ALARM_PROB_CAUSE_THRESHOLD_CROSSED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_THRESHOLD_CROSSED)
      .value("ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED)
      .value("ALARM_PROB_CAUSE_CONGESTION", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_CONGESTION)
      .value("ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY)
      .value("ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM)
      .value("ALARM_PROB_CAUSE_VERSION_MISMATCH", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_VERSION_MISMATCH)
      .value("ALARM_PROB_CAUSE_CORRUPT_DATA", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_CORRUPT_DATA)
      .value("ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED)
      .value("ALARM_PROB_CAUSE_SOFTWARE_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_SOFTWARE_ERROR)
      .value("ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR)
      .value("ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED)
      .value("ALARM_PROB_CAUSE_FILE_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_FILE_ERROR)
      .value("ALARM_PROB_CAUSE_OUT_OF_MEMORY", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_OUT_OF_MEMORY)
      .value("ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE)
      .value("ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE)
      .value("ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR)
      .value("ALARM_PROB_CAUSE_POWER_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_POWER_PROBLEM)
      .value("ALARM_PROB_CAUSE_TIMING_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TIMING_PROBLEM)
      .value("ALARM_PROB_CAUSE_PROCESSOR_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM)
      .value("ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR)
      .value("ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM)
      .value("ALARM_PROB_CAUSE_RECEIVER_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_RECEIVER_FAILURE)
      .value("ALARM_PROB_CAUSE_TRANSMITTER_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TRANSMITTER_FAILURE)
      .value("ALARM_PROB_CAUSE_RECEIVE_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TRANSMITTER_FAILURE)
      .value("ALARM_PROB_CAUSE_TRANSMIT_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TRANSMIT_FAILURE)
      .value("ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR)
      .value("ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR)
      .value("ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR)
      .value("ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION)
      .value("ALARM_PROB_CAUSE_ADAPTER_ERROR", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_ADAPTER_ERROR)
      .value("ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE)
      .value("ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE)
      .value("ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM)
      .value("ALARM_PROB_CAUSE_FIRE_DETECTED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_FIRE_DETECTED)
      .value("ALARM_PROB_CAUSE_FLOOD_DETECTED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_FLOOD_DETECTED)
      .value("ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED)
      .value("ALARM_PROB_CAUSE_LEAK_DETECTED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_LEAK_DETECTED)
      .value("ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE)
      .value("ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION)
      .value("ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED)
      .value("ALARM_PROB_CAUSE_PUMP_FAILURE", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_PUMP_FAILURE)
      .value("ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN", FaultEnums::FaultProbableCause::ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN);

  enum_<FaultEnums::FaultSeverity>("FaultSeverity")
      .value("ALARM_SEVERITY_INVALID", FaultEnums::FaultSeverity::ALARM_SEVERITY_INVALID)
      .value("ALARM_SEVERITY_CRITICAL", FaultEnums::FaultSeverity::ALARM_SEVERITY_CRITICAL)
      .value("ALARM_SEVERITY_MAJOR", FaultEnums::FaultSeverity::ALARM_SEVERITY_MAJOR)
      .value("ALARM_SEVERITY_MINOR", FaultEnums::FaultSeverity::ALARM_SEVERITY_MINOR)
      .value("ALARM_SEVERITY_WARNING", FaultEnums::FaultSeverity::ALARM_SEVERITY_WARNING)
      .value("ALARM_SEVERITY_INDETERMINATE", FaultEnums::FaultSeverity::ALARM_SEVERITY_INDETERMINATE)
      .value("ALARM_SEVERITY_CLEAR", FaultEnums::FaultSeverity::ALARM_SEVERITY_CLEAR);

  class_<FaultSeverityManager>("FaultSeverityManager", no_init)
      .def("c_str",static_cast< const char* (*)(FaultSeverity) > (&FaultSeverityManager::c_str))
    .staticmethod("c_str");

  enum_<FaultEnums::FaultMessageType>("FaultMessageType")
      .value("MSG_UNDEFINED", FaultEnums::FaultMessageType::MSG_UNDEFINED)
      .value("MSG_ENTITY_JOIN", FaultEnums::FaultMessageType::MSG_ENTITY_JOIN)
      .value("MSG_ENTITY_LEAVE", FaultEnums::FaultMessageType::MSG_ENTITY_LEAVE)
      .value("MSG_ENTITY_FAULT", FaultEnums::FaultMessageType::MSG_ENTITY_FAULT)
      .value("MSG_ENTITY_STATE_CHANGE", FaultEnums::FaultMessageType::MSG_ENTITY_STATE_CHANGE)
      .value("MSG_ENTITY_JOIN_BROADCAST", FaultEnums::FaultMessageType::MSG_ENTITY_JOIN_BROADCAST)
      .value("MSG_ENTITY_LEAVE_BROADCAST", FaultEnums::FaultMessageType::MSG_ENTITY_LEAVE_BROADCAST)
      .value("MSG_ENTITY_FAULT_BROADCAST", FaultEnums::FaultMessageType::MSG_ENTITY_FAULT_BROADCAST)
      .value("MSG_ENTITY_STATE_CHANGE_BROADCAST", FaultEnums::FaultMessageType::MSG_ENTITY_STATE_CHANGE_BROADCAST);

  // Fault
  class_<Fault>("Fault", no_init)
    .def(init<>())
        .def("init", faultInit2)
        .def("notify", notify)
        .def("getFaultState", &Fault::getFaultState)
        .def("registerEntity", &Fault::registerEntity);

  class_<FaultEventData>("FaultEventData")
     .def(init<>());

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

  void (Checkpoint::*wss) (const std::string&, const std::string&,Transaction&) = &Checkpoint::write;
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

  //class_<Checkpoint>("Checkpoint",init<Handle,unsigned int flags, unsigned int size, unsigned int rows>())
  //    .def(init<uint_t flags, uint_t size, uint_t rows>);
  //.def(write, &Checkpoint::write)
  //  .def(read, &Checkpoint::read);

  // Management information access

  def("getProcessHandle",static_cast< SAFplus::Handle (*)(int, int) > (&SAFplus::getProcessHandle));
  def("mgtGet",static_cast< std::string (*)(const std::string&) > (&SAFplus::mgtGet)); 
  def("mgtSet",static_cast< ClRcT (*)(const std::string&,const std::string&) > (&SAFplus::mgtSet)); 
  def("mgtCreate",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtCreate));
  def("mgtDelete",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtDelete));
}
