#include <boost/python.hpp>

#include <clLogApi.hxx>
#include <clHandleApi.hxx>

using namespace SAFplus;

int test;

BOOST_PYTHON_MODULE(pySafplus)
{
  logInitialize();
  utilsInitialize();
  using namespace boost::python;
  def("logMsgWrite",SAFplus::logStrWrite);
  enum_<LogSeverity>("Severity")
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

  class_<Handle>("Handle")
        .def("getNode", &Handle::getNode)
        .def("getCluster", &Handle::getCluster)
        .def("getProcess", &Handle::getProcess)
        .def("getIndex", &Handle::getIndex)
        .def("getType", &Handle::getType);

  class_<WellKnownHandle,bases<Handle> >("WellKnownHandle",no_init)        
        .def("getNode", &WellKnownHandle::getNode)
        .def("getCluster", &WellKnownHandle::getCluster)
        .def("getProcess", &WellKnownHandle::getProcess)
        .def("getIndex", &WellKnownHandle::getIndex)
        .def("getType", &WellKnownHandle::getType);

  boost::python::scope().attr("SYS_LOG") = SYS_LOG;   
  boost::python::scope().attr("APP_LOG") = APP_LOG;   

  class_<Buffer>("Buffer", no_init)
    .def("len",&Buffer::len)
    .def

  class_<Checkpoint>("Checkpoint")
        .def("getNode", &Handle::getNode)
        .def("getCluster", &Handle::getCluster)
        .def("getProcess", &Handle::getProcess)
        .def("getIndex", &Handle::getIndex)
        .def("getType", &Handle::getType);

}
