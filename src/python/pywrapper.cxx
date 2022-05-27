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
    dictionary["id"] = std::to_string(nodeConfig->id());
//    dictionary["lastSUFailure"] = std::to_string(nodeConfig->lastSUFailure());
    dictionary["name"] = nodeConfig->name();
//    dictionary["restartable"] = std::to_string(nodeConfig->restartable());
//    dictionary["serviceUnitFailureEscalationPolicy"] = std::to_string(nodeConfig->serviceUnitFailureEscalationPolicy());
    boost::python::list listSUs;
    const int numOfSUs = nodeConfig->serviceunits_size();
    for(int i = 0; i < numOfSUs; ++i)
    {
        listSUs.append(nodeConfig->serviceunits(i));
    }
    dictionary["serviceUnits"] =listSUs;
//    dictionary["userDefinedType"] = std::to_string(nodeConfig->userDefinedType());

    return dictionary;
}

static dict nodeGetStatus(const Handle & self, const std::string & key)
{
    // get information into nodeStatus
    SAFplus::Rpc::amfMgmtRpc::NodeStatus* nodeStatus;
    ClRcT rc = amfMgmtNodeGetStatus(self, key, &nodeStatus);
    Py_Initialize();
    dict dictionary;
    if (rc != CL_OK)
    {
        dictionary["error"] = rc;
        return dictionary;
    }

    // set information into dictionary
    dictionary["presenceState"] = std::to_string(nodeStatus->presencestate());

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
    boost::python::list listSIs;
    const int numOfSIs = SGConfig->serviceinstances_size();
    for(int i = 0; i < numOfSIs; ++i)
    {
        listSIs.append(SGConfig->serviceinstances(i));
    }
    dictionary["serviceInstances"] = listSIs;
//    dictionary["serviceUnitRestart"] = std::to_string(SGConfig->serviceUnitRestart());
    boost::python::list listSUs;
    const int numOfSUs = SGConfig->serviceunits_size();
    for(int i = 0; i < numOfSUs; ++i)
    {
        listSUs.append(SGConfig->serviceunits(i));
    }
    dictionary["serviceUnits"] = listSUs;

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
    boost::python::list listComps;
    const int numOfComps = SUConfig->components_size();
    for(int i = 0; i < numOfComps; ++i)
    {
        listComps.append(SUConfig->components(i));
    }
    dictionary["components"] = listComps;
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
    dictionary["serviceGroup"] = SUConfig->servicegroup();

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
    boost::python::list listSIs;
    const int numOfSIs = SIConfig->componentserviceinstances_size();
    for(int i = 0; i < numOfSIs; ++i)
    {
        listSIs.append(SIConfig->componentserviceinstances(i));
    }
    dictionary["componentServiceInstances"] = listSIs;
//    dictionary["id"] = std::to_string(SIConfig->id());
//    dictionary["isFullActiveAssignment"] = std::to_string(SIConfig->isFullActiveAssignment());
//    dictionary["isFullStandbyAssignment"] = std::to_string(SIConfig->isFullStandbyAssignment());
    dictionary["name"] = SIConfig->name();
    dictionary["preferredActiveAssignments"] = std::to_string(SIConfig->preferredactiveassignments());
    dictionary["preferredStandbyAssignments"] = std::to_string(SIConfig->preferredstandbyassignments());
    dictionary["rank"] = std::to_string(SIConfig->rank());
    dictionary["serviceGroup"] = SIConfig->servicegroup();

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
    boost::python::list listDeps;
    const int numOfDeps = CSIConfig->dependencies_size();
    for(int i = 0; i < numOfDeps; ++i)
    {
        listDeps.append(CSIConfig->dependencies(i));
    }
    dictionary["dependencies"] = listDeps;
//    dictionary["id"] = std::to_string(CSIConfig->id());
    dictionary["name"] = CSIConfig->name();
    dictionary["serviceInstance"] = CSIConfig->serviceinstance();
    dictionary["type"] = CSIConfig->type();

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
    boost::python::list listActives;
    const int numOfActives = CSIStatus->activecomponents_size();
    for(int i = 0; i < numOfActives; ++i)
    {
        listActives.append(CSIStatus->activecomponents(i));
    }
    dictionary["activeComponents"] = listActives;
//    dictionary["isProxyCSI"] = CSIStatus->isProxyCSI();
    boost::python::list listStandbys;
    const int numOfStandbys = CSIStatus->standbycomponents_size();
    for(int i = 0; i < numOfStandbys; ++i)
    {
        listStandbys.append(CSIStatus->standbycomponents(i));
    }
    dictionary["standbyComponents"] = listStandbys;

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
    dictionary["csiType"] = ComponentConfig->csitype();
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
    dictionary["serviceUnit"] = ComponentConfig->serviceunit();
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

ClRcT addNewComponent(const SAFplus::Handle& mgmtHandle, const std::string & compName, const std::string & suName, const std::string & binary)
{
    SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
    comp->set_name(compName.c_str());
    comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_x_active_or_y_standby);
    SAFplus::Rpc::amfMgmtRpc::Execution* exe = new SAFplus::Rpc::amfMgmtRpc::Execution();
    char cmd[SAFplus::CL_MAX_NAME_LENGTH];
    strcpy(cmd, binary.c_str());
//    strcat(cmd, " ");
//    strcat(cmd,compName.c_str());
    exe->set_command(cmd);
    uint64_t timeout = 10000;
    exe->set_timeout(timeout);
    SAFplus::Rpc::amfMgmtRpc::Instantiate* inst = new SAFplus::Rpc::amfMgmtRpc::Instantiate();
    inst->set_allocated_execution(exe);
    comp->set_allocated_instantiate(inst);
    comp->set_serviceunit(suName.c_str());

    SAFplus::Rpc::amfMgmtRpc::Timeouts* timeouts = new SAFplus::Rpc::amfMgmtRpc::Timeouts();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* terminateTimeout = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    terminateTimeout->set_uint64(35000);
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* quiescingComplete = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    quiescingComplete->set_uint64(45000);
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* workRemoval = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    workRemoval->set_uint64(55000);
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* workAssignment = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    workAssignment->set_uint64(65000);
    timeouts->set_allocated_terminate(terminateTimeout);
    timeouts->set_allocated_quiescingcomplete(quiescingComplete);
    timeouts->set_allocated_workremoval(workRemoval);
    timeouts->set_allocated_workassignment(workAssignment);
    comp->set_allocated_timeouts(timeouts);

    ClRcT rc = SAFplus::amfMgmtComponentCreate(mgmtHandle,comp);
    return rc;
}

ClRcT addNewServiceUnit(const SAFplus::Handle& mgmtHandle, const std::string & suName, const std::string & compName, const std::string & sgName, const std::string & nodeName)
{
  SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su = new SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig();
  su->set_name(suName.c_str());
  su->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
  su->add_components(compName.c_str(), strlen(compName.c_str()));
  su->set_servicegroup(sgName.c_str(), strlen(sgName.c_str()));
  su->set_node(nodeName.c_str(), strlen(nodeName.c_str()));
  ClRcT rc = SAFplus::amfMgmtServiceUnitCreate(mgmtHandle,su);
  return rc;
}

ClRcT addNewServiceGroup(const SAFplus::Handle& mgmtHandle, const std::string & sgName, const std::string & suName, const std::string & siName)
{
  SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg = new SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig();
  sg->set_name(sgName.c_str());
  sg->add_serviceunits(suName.c_str(), strlen(suName.c_str()));
  sg->add_serviceinstances(siName.c_str(), strlen(siName.c_str()));
  sg->set_autorepair(true);
  ClRcT rc = SAFplus::amfMgmtServiceGroupCreate(mgmtHandle,sg);
  return rc;
}

ClRcT addNewNode(const SAFplus::Handle& mgmtHandle, const std::string & nodeName, const std::string & suName)
{
  SAFplus::Rpc::amfMgmtRpc::NodeConfig* node = new SAFplus::Rpc::amfMgmtRpc::NodeConfig();
  node->set_name(nodeName.c_str());
  node->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
  node->add_serviceunits(suName.c_str(), strlen(suName.c_str()));
  ClRcT rc = SAFplus::amfMgmtNodeCreate(mgmtHandle,node);
  return rc;
}

ClRcT addNewServiceInstance(const SAFplus::Handle& mgmtHandle, const std::string & siName, const std::string & sgName, const std::string & csiName)
{
  SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si = new SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig();
  si->set_name(siName.c_str());
  si->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
  si->add_componentserviceinstances(csiName.c_str(), strlen(csiName.c_str()));
  si->set_servicegroup(sgName.c_str(), strlen(sgName.c_str()));
  ClRcT rc = SAFplus::amfMgmtServiceInstanceCreate(mgmtHandle,si);
  return rc;
}

ClRcT addNewComponentServiceInstance(const SAFplus::Handle& mgmtHandle, const std::string & csiName, const std::string & siName, const std::string & ipName = "ip", const std::string & ipValue = "192.168.65.111", const std::string & portName = "port", const std::string & portValue = "35678")
{
  SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi = new SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig();
  csi->set_name(csiName.c_str());
  csi->set_serviceinstance(siName.c_str(), strlen(siName.c_str()));
  csi->add_data();
  //Fill data
  SAFplus::Rpc::amfMgmtRpc::Data* data = csi->mutable_data(0);
  data->set_name(ipName.c_str());
  data->set_val(ipValue.c_str());
  data = csi->mutable_data(1);
  data->set_name(portName.c_str());
  data->set_val(portValue.c_str());
  ClRcT rc = SAFplus::amfMgmtComponentServiceInstanceCreate(mgmtHandle,csi);
  return rc;
}

ClRcT updateComponentServiceInstance(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
  SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi = new SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig();
  ssize_t len = boost::python::len(argv);
  for(int i = 0; i < len; i = i + 2)
  {
      std::string attr = boost::python::extract<std::string>(argv[i]);
      std::string val = boost::python::extract<std::string>(argv[i+1]);
      if(!attr.compare("name"))
      {
          csi->set_name(val);
      }
      else if(!attr.compare("serviceInstance"))
      {
          csi->set_serviceinstance(val);
      }
      else if(!attr.compare("type"))
      {
          csi->set_type(val);
      }
      else
      {
          std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
          return CL_ERR_UNSPECIFIED;
      }
  }

  ClRcT rc = SAFplus::amfMgmtComponentServiceInstanceConfigSet(mgmtHandle,csi);
  return rc;
}

ClRcT updateServiceInstance(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
  SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si = new SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig();
  ssize_t len = boost::python::len(argv);
  for(int i = 0; i < len; i = i + 2)
  {
      std::string attr = boost::python::extract<std::string>(argv[i]);
      std::string val = boost::python::extract<std::string>(argv[i+1]);
      if(!attr.compare("name"))
      {
          si->set_name(val);
      }
      else if(!attr.compare("adminState"))
      {
          if(std::stoi(val) == 0)
          {
              si->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_off);
          }
          else if(std::stoi(val) == 1)
          {
              si->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_idle);
          }
          else if(std::stoi(val) == 2)
          {
              si->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
          }
          else
          {
              std::cout << "Doesn't support adminState value: " << val << std::endl;
              return CL_ERR_UNSPECIFIED;
          }
      }
      else if(!attr.compare("preferredActiveAssignments"))
      {
          si->set_preferredactiveassignments(std::stoi(val));
      }
      else if(!attr.compare("preferredStandbyAssignments"))
      {
          si->set_preferredstandbyassignments(std::stoi(val));
      }
      else if(!attr.compare("rank"))
      {
          si->set_rank(std::stoi(val));
      }
      else if(!attr.compare("componentServiceInstances"))
      {
          si->add_componentserviceinstances(val);
      }
      else if(!attr.compare("serviceGroup"))
      {
          si->set_servicegroup(val);
      }
      else
      {
          std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
          return CL_ERR_UNSPECIFIED;
      }
  }

  ClRcT rc = SAFplus::amfMgmtServiceInstanceConfigSet(mgmtHandle,si);
  return rc;
}

ClRcT updateServiceUnit(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
  SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su = new SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig();
  ssize_t len = boost::python::len(argv);
  for(int i = 0; i < len; i = i + 2)
  {
      std::string attr = boost::python::extract<std::string>(argv[i]);
      std::string val = boost::python::extract<std::string>(argv[i+1]);
      if(!attr.compare("name"))
      {
          su->set_name(val);
      }
      else if(!attr.compare("adminState"))
      {
          if(std::stoi(val) == 0)
          {
              su->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_off);
          }
          else if(std::stoi(val) == 1)
          {
              su->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_idle);
          }
          else if(std::stoi(val) == 2)
          {
              su->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
          }
          else
          {
              std::cout << "Doesn't support adminState value: " << val << std::endl;
              return CL_ERR_UNSPECIFIED;
          }
      }
      else if(!attr.compare("rank"))
      {
          su->set_rank(std::stoi(val));
      }
      else if(!attr.compare("failover"))
      {
          su->set_failover(std::stoi(val));
      }
      else if(!attr.compare("components"))
      {
          su->add_components(val);
      }
      else if(!attr.compare("node"))
      {
          su->set_node(val);
      }
      else if(!attr.compare("serviceGroup"))
      {
          su->set_servicegroup(val);
      }
      else
      {
          std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
          return CL_ERR_UNSPECIFIED;
      }
  }

  ClRcT rc = SAFplus::amfMgmtServiceUnitConfigSet(mgmtHandle,su);
  return rc;
}

ClRcT updateNode(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
    SAFplus::Rpc::amfMgmtRpc::NodeConfig* node = new SAFplus::Rpc::amfMgmtRpc::NodeConfig();
    ssize_t len = boost::python::len(argv);
    for(int i = 0; i < len; i = i + 2)
    {
        std::string attr = boost::python::extract<std::string>(argv[i]);
        std::string val = boost::python::extract<std::string>(argv[i+1]);
        if(!attr.compare("name"))
        {
            node->set_name(val);
        }
        else if(!attr.compare("adminState"))
        {
            if(std::stoi(val) == 0)
            {
                node->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_off);
            }
            else if(std::stoi(val) == 1)
            {
                node->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_idle);
            }
            else if(std::stoi(val) == 2)
            {
                node->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
            }
            else
            {
                std::cout << "Doesn't support adminState value: " << val << std::endl;
                return CL_ERR_UNSPECIFIED;
            }
        }
        else if(!attr.compare("autoRepair"))
        {
            node->set_autorepair(std::stoi(val));
        }
        else if(!attr.compare("failFastOnInstantiationFailure"))
        {
            node->set_failfastoninstantiationfailure(std::stoi(val));
        }
        else if(!attr.compare("failFastOnCleanupFailure"))
        {
            node->set_failfastoncleanupfailure(std::stoi(val));
        }
        else if(!attr.compare("serviceUnits"))
        {
            node->add_serviceunits(val);
        }
        else
        {
            std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
            return CL_ERR_UNSPECIFIED;
        }
    }

    ClRcT rc = SAFplus::amfMgmtNodeConfigSet(mgmtHandle,node);
    return rc;
}

ClRcT updateServiceGroup(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
    SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg = new SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* autoadjustinterval = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    ssize_t len = boost::python::len(argv);
    for(int i = 0; i < len; i = i + 2)
    {
        std::string attr = boost::python::extract<std::string>(argv[i]);
        std::string val = boost::python::extract<std::string>(argv[i+1]);
        if(!attr.compare("name"))
        {
            sg->set_name(val);
        }
        else if(!attr.compare("adminState"))
        {
            if(std::stoi(val) == 0)
            {
                sg->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_off);
            }
            else if(std::stoi(val) == 1)
            {
                sg->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_idle);
            }
            else if(std::stoi(val) == 2)
            {
                sg->set_adminstate(SAFplus::Rpc::amfMgmtRpc::AdministrativeState::AdministrativeState_on);
            }
            else
            {
                std::cout << "Doesn't support adminState value: " << val << std::endl;
                return CL_ERR_UNSPECIFIED;
            }
        }
        else if(!attr.compare("autoRepair"))
        {
            sg->set_autorepair(std::stoi(val));
        }
        else if(!attr.compare("autoAdjust"))
        {
            sg->set_autoadjust(std::stoi(val));
        }
        else if(!attr.compare("autoAdjustInterval"))
        {
            autoadjustinterval->set_uint64(std::stoi(val));
            sg->set_allocated_autoadjustinterval(autoadjustinterval);
        }
        else if(!attr.compare("preferredNumActiveServiceUnits"))
        {
            sg->set_preferrednumactiveserviceunits(std::stoi(val));
        }
        else if(!attr.compare("preferredNumStandbyServiceUnits"))
        {
            sg->set_preferrednumstandbyserviceunits(std::stoi(val));
        }
        else if(!attr.compare("preferredNumIdleServiceUnits"))
        {
            sg->set_preferrednumidleserviceunits(std::stoi(val));
        }
        else if(!attr.compare("maxActiveWorkAssignments"))
        {
            sg->set_maxactiveworkassignments(std::stoi(val));
        }
        else if(!attr.compare("maxStandbyWorkAssignments"))
        {
            sg->set_maxstandbyworkassignments(std::stoi(val));
        }
        else if(!attr.compare("serviceUnits"))
        {
            sg->add_serviceunits(val);
        }
        else if(!attr.compare("serviceInstances"))
        {
            sg->add_serviceinstances(val);
        }
        else
        {
            std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
            return CL_ERR_UNSPECIFIED;
        }

    }
    ClRcT rc = SAFplus::amfMgmtServiceGroupConfigSet(mgmtHandle,sg);
    return rc;
}

ClRcT updateComponent(const SAFplus::Handle& mgmtHandle, boost::python::list & argv)
{
    SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
    SAFplus::Rpc::amfMgmtRpc::Execution* exeInst = new SAFplus::Rpc::amfMgmtRpc::Execution();
    SAFplus::Rpc::amfMgmtRpc::Execution* exeTer = new SAFplus::Rpc::amfMgmtRpc::Execution();
    SAFplus::Rpc::amfMgmtRpc::Execution* exeCl = new SAFplus::Rpc::amfMgmtRpc::Execution();
    SAFplus::Rpc::amfMgmtRpc::Instantiate* inst = new SAFplus::Rpc::amfMgmtRpc::Instantiate();
    SAFplus::Rpc::amfMgmtRpc::Terminate* terminate = new SAFplus::Rpc::amfMgmtRpc::Terminate();
    SAFplus::Rpc::amfMgmtRpc::Cleanup* cleanup = new SAFplus::Rpc::amfMgmtRpc::Cleanup();
    SAFplus::Rpc::amfMgmtRpc::Timeouts* timeouts = new SAFplus::Rpc::amfMgmtRpc::Timeouts();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* terminateTimeout = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* quiescingComplete = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* workRemoval = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();
    SAFplus::Rpc::amfMgmtRpc::SaTimeT* workAssignment = new SAFplus::Rpc::amfMgmtRpc::SaTimeT();

    ssize_t len = boost::python::len(argv);
    for(int i = 0; i < len; i = i + 2)
    {
        std::string attr = boost::python::extract<std::string>(argv[i]);
        std::string val = boost::python::extract<std::string>(argv[i+1]);
        if(!attr.compare("name"))
        {
            comp->set_name(val);
        }
        else if(!attr.compare("capabilityModel"))
        {
            if(std::stoi(val) == 0)
            {
                comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_x_active_and_y_standby);
            }
            else if(std::stoi(val) == 1)
            {
                comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_x_active_or_y_standby);
            }
            else if(std::stoi(val) == 2)
            {
                comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_not_preinstantiable);
            }
            else
            {
                std::cout << "Doesn't support capabilityModel value: " << val << std::endl;
                return CL_ERR_UNSPECIFIED;
            }
        }
        else if(!attr.compare("maxActiveAssignments"))
        {
            comp->set_maxactiveassignments(std::stoi(val));
        }
        else if(!attr.compare("maxStandbyAssignments"))
        {
            comp->set_maxstandbyassignments(std::stoi(val));
        }
        else if(!attr.compare("commandInstantiate"))
        {
            exeInst->set_command(val);
        }
        else if(!attr.compare("timeoutInstantiate"))
        {
            exeInst->set_timeout(std::stoi(val));
            inst->set_allocated_execution(exeInst);
            comp->set_allocated_instantiate(inst);
        }
        else if(!attr.compare("commandTerminate"))
        {
            exeTer->set_command(val);
        }
        else if(!attr.compare("timeoutTerminate"))
        {
            exeTer->set_timeout(std::stoi(val));
            terminate->set_allocated_execution(exeTer);
            comp->set_allocated_terminate(terminate);
        }
        else if(!attr.compare("commandCleanup"))
        {
            exeCl->set_command(val);
        }
        else if(!attr.compare("timeoutCleanup"))
        {
            exeCl->set_timeout(std::stoi(val));
            cleanup->set_allocated_execution(exeCl);
            comp->set_allocated_cleanup(cleanup);
        }
        else if(!attr.compare("maxInstantInstantiations"))
        {
            comp->set_maxinstantinstantiations(std::stoi(val));
        }
        else if(!attr.compare("maxDelayedInstantiations"))
        {
            comp->set_maxdelayedinstantiations(std::stoi(val));
        }
        else if(!attr.compare("instantiationSuccessDuration"))
        {
            comp->set_instantiationsuccessduration(std::stoi(val));
        }
        else if(!attr.compare("delayBetweenInstantiation"))
        {
            comp->set_delaybetweeninstantiation(std::stoi(val));
        }
        else if(!attr.compare("terminateTimeout"))
        {
            terminateTimeout->set_uint64(std::stoi(val));
            timeouts->set_allocated_terminate(terminateTimeout);
        }
        else if(!attr.compare("quiescingcompleteTimeout"))
        {
            quiescingComplete->set_uint64(std::stoi(val));
            timeouts->set_allocated_quiescingcomplete(quiescingComplete);
        }
        else if(!attr.compare("workremovalTimeout"))
        {
            workRemoval->set_uint64(std::stoi(val));
            timeouts->set_allocated_workremoval(workRemoval);
        }
        else if(!attr.compare("workassignmentTimeout"))
        {
            workAssignment->set_uint64(std::stoi(val));
            timeouts->set_allocated_workassignment(workAssignment);
            comp->set_allocated_timeouts(timeouts);
        }
        else if(!attr.compare("serviceUnit"))
        {
            comp->set_serviceunit(val);
        }
        else if(!attr.compare("recovery"))
        {
            if(std::stoi(val) == 1)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_NoRecommendation);
            }
            else if(std::stoi(val) == 2)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_Restart);
            }
            else if(std::stoi(val) == 3)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_Failover);
            }
            else if(std::stoi(val) == 4)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_NodeSwitchover);
            }
            else if(std::stoi(val) == 5)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_NodeFailover);
            }
            else if(std::stoi(val) == 6)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_NodeFailfast);
            }
            else if(std::stoi(val) == 7)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_ClusterReset);
            }
            else if(std::stoi(val) == 8)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_ApplicationRestart);
            }
            else if(std::stoi(val) == 9)
            {
                comp->set_recovery(SAFplus::Rpc::amfMgmtRpc::Recovery_ContainerRestart);
            }
            else
            {
                std::cout << "Doesn't support recovery value: " << val << std::endl;
                return CL_ERR_UNSPECIFIED;
            }
        }
        else if(!attr.compare("restartable"))
        {
            comp->set_restartable(std::stoi(val));
        }
        else if(!attr.compare("csiType"))
        {
            comp->set_csitype(val);
        }
        else
        {
            std::cout << "Doesn't support setting this attribute: " << attr << std::endl;
            return CL_ERR_UNSPECIFIED;
        }
    }

    ClRcT rc = SAFplus::amfMgmtComponentConfigSet(mgmtHandle,comp);
    return rc;
}

BOOST_PYTHON_FUNCTION_OVERLOADS(amfMgmtFinalize_overloads, SAFplus::amfMgmtFinalize, 1, 2)

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

  /*
  // members of MembersOfComponentConfig class
  class_<MembersOfComponentConfig>("MembersOfComponentConfig")
          .def(init<optional<std::string>>())
          .add_property("name", &MembersOfComponentConfig::getName, &MembersOfComponentConfig::setName);
*/
  // Management information access

  def("mgtGet",static_cast< std::string (*)(const std::string&) > (&SAFplus::mgtGet));
  def("mgtSet",static_cast< ClRcT (*)(const std::string&,const std::string&) > (&SAFplus::mgtSet)); 
  def("mgtCreate",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtCreate));
  def("mgtDelete",static_cast< ClRcT (*)(const std::string& pathSpec) > (&SAFplus::mgtDelete));

  def("getProcessHandle",static_cast< Handle (*)(int pid, int nodeNum) > (&SAFplus::getProcessHandle));
  def("mgtGet",static_cast< std::string (*)(Handle src, const std::string&) > (&SAFplus::mgtGet));

  def("amfMgmtInitialize",static_cast< ClRcT (*)(Handle &) > (&SAFplus::amfMgmtInitialize));
  //def("amfMgmtFinalize",static_cast< ClRcT (*)(const Handle &) > (&SAFplus::amfMgmtFinalize)); // comment. replaced with code below for defining default argument as the second one

  def("amfMgmtFinalize", SAFplus::amfMgmtFinalize, amfMgmtFinalize_overloads(
    (arg("amfMgmtHandle"),
     arg("finalizeRpc")=0)));

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
  def("amfMgmtNodeGetStatus",static_cast< dict (*)(const Handle &, const std::string &) > (&nodeGetStatus));
  def("amfMgmtSGGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SGGetConfig));
  def("amfMgmtSUGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SUGetConfig));
  def("amfMgmtSIGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&SIGetConfig));
  def("amfMgmtCSIGetConfig",static_cast< dict (*)(const Handle &, const std::string &) > (&CSIGetConfig));
  def("amfMgmtCSIGetStatus",static_cast< dict (*)(const Handle &, const std::string &) > (&CSIGetStatus));


  def("addSUIntoNode",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addSUIntoNode));
  def("addSUIntoSG",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addSUIntoSG));
  def("addCompIntoSU",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addCompIntoSU));
  def("amfMgmtCommit",static_cast< ClRcT (*)(const Handle &) > (&amfMgmtCommit));

  def("updateComponent",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateComponent));
  def("updateServiceGroup",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateServiceGroup));
  def("updateNode",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateNode));
  def("updateServiceUnit",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateServiceUnit));
  def("updateServiceInstance",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateServiceInstance));
  def("updateComponentServiceInstance",static_cast< ClRcT (*)(const Handle &, boost::python::list&) > (&updateComponentServiceInstance));

  def("addNewComponent",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &) > (&addNewComponent));
  def("addNewServiceGroup",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &) > (&addNewServiceGroup));
  def("addNewNode",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &) > (&addNewNode));
  def("addNewServiceUnit",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &, const std::string &) > (&addNewServiceUnit));
  def("addNewServiceInstance",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &) > (&addNewServiceInstance));
  def("addNewComponentServiceInstance",static_cast< ClRcT (*)(const Handle &, const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, const std::string &) > (&addNewComponentServiceInstance));

  def("amfMgmtComponentDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtComponentDelete));
  def("amfMgmtServiceGroupDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtServiceGroupDelete));
  def("amfMgmtNodeDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtNodeDelete));
  def("amfMgmtServiceUnitDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtServiceUnitDelete));
  def("amfMgmtServiceInstanceDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtServiceInstanceDelete));
  def("amfMgmtComponentServiceInstanceDelete",static_cast< ClRcT (*)(const Handle &, const std::string &) > (&amfMgmtComponentServiceInstanceDelete));

  def("amfMgmtSafplusInstallInfoGet",static_cast< std::string (*)(const Handle &, const std::string &) > (&SAFplus::amfMgmtSafplusInstallInfoGet));
}
