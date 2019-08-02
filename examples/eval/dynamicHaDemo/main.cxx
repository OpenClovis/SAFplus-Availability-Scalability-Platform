
#include <clCommon.hxx>
#include <clCustomization.hxx>
#include <saAmf.h>
#include <clNameApi.hxx>
#include <safplus.hxx>
#include <boost/thread/thread.hpp> 
#include <clAmfMgmtApi.hxx>
#include <clHandleApi.hxx>

//#define clprintf(sev,...) appLog(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __VA_ARGS__)
#define clprintf(sev,...) SAFplus::logMsgWrite(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __FILE__, __LINE__, __VA_ARGS__)

// This is the application name to use if the AMF did not start up this component
#define DEFAULT_APP_NAME "c0"

#define FN_CALL(fn)                                                    \
do {                                                                    \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    returnCode = (fn);                                                                        \
                                                                        \
    if (returnCode != CL_OK)                                            \
    {                                                                   \
        logError("MGMT","RPC",                                  \
            "ALERT [%s:%d] : Fn [%s] returned [0x%x]\n",               \
             __FUNCTION__, __LINE__, #fn, returnCode);                 \
        return returnCode;                                              \
    }                                                                   \
} while (0)

/* Access to the SAF AMF framework occurs through this handle */
SaAmfHandleT amfHandle;

/* This process's SAF name */
SaNameT      appName = {0};

/* set when SAFplus tells this process to quit */
bool quitting = false;
/* set when SAFplus assigns active/standby work */
int running = false;

/* The application should fill these functions */
void safTerminate(SaInvocationT invocation, const SaNameT *compName);
void safAssignWork(SaInvocationT       invocation,
                   const SaNameT       *compName,
                   SaAmfHAStateT       haState,
                   SaAmfCSIDescriptorT csiDescriptor);
void safRemoveWork(SaInvocationT  invocation,
                   const SaNameT  *compName,
                   const SaNameT  *csiName,
                   SaAmfCSIFlagsT csiFlags);

/* Utility functions */
void initializeAmf(void);
void dispatchLoop(void);
void printCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState);
int  errorExit(SaAisErrorT rc);


/* This simple helper function just prints an error and quits */
int errorExit(SaAisErrorT rc)
{
    logCritical("APP","INI", "Component [%.*s] : PID [%d]. Initialization error [0x%x]\n", appName.length, appName.value, SAFplus::pid, rc);
    exit(-1);
    return -1;
}

namespace SAFplus
  {
  extern Handle           myHandle;  // This handle resolves to THIS process.
  extern SafplusInitializationConfiguration   serviceConfig;
};

using namespace SAFplus::Rpc::amfMgmtRpc;

/*
      <Node name="node0">
      <adminState>on</adminState>
      <autoRepair>true</autoRepair>
      <failFastOnCleanupFailure>false</failFastOnCleanupFailure>
      <failFastOnInstantiationFailure>false</failFastOnInstantiationFailure>
      <serviceUnits>su0</serviceUnits>
      </Node>    
 
      <ServiceGroup name="sg0">
      <adminState>on</adminState>
      <autoAdjust>false</autoAdjust>
      <autoAdjustInterval>0</autoAdjustInterval>
      <autoRepair>true</autoRepair>
      <maxActiveWorkAssignments>1</maxActiveWorkAssignments>
      <maxStandbyWorkAssignments>1</maxStandbyWorkAssignments>
      <preferredNumActiveServiceUnits>1</preferredNumActiveServiceUnits>
      <preferredNumIdleServiceUnits>1</preferredNumIdleServiceUnits>
      <preferredNumStandbyServiceUnits>1</preferredNumStandbyServiceUnits>
      <serviceInstances>si</serviceInstances>
      <serviceUnits>su0</serviceUnits>
      <serviceUnits>su1</serviceUnits>
    </ServiceGroup>

    <ServiceUnit name="su0">
      <adminState>on</adminState>
      <rank>0</rank>
      <failover>true</failover>
      <components>c0</components>
      <serviceGroup>sg0</serviceGroup>
      <node>node0</node>
    </ServiceUnit>
*/

ClRcT addNewServiceUnit(const SAFplus::Handle& mgmtHandle, const char* suName, const char* compName, const char* sgName, const char* nodeName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new su");
  SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su = new SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig();
  su->set_name(suName);
  //const char* comp = "c12";
  //const char* sg = "sg0";
  //const char* node = "node1";
  su->set_adminstate(AdministrativeState::AdministrativeState_on);
  su->add_components(compName, strlen(compName));
  su->set_servicegroup(sgName, strlen(sgName));
  su->set_node(nodeName, strlen(nodeName));
  ClRcT rc = SAFplus::amfMgmtServiceUnitCreate(mgmtHandle,su);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new su returns [0x%x]", rc);
  return rc;
}

ClRcT addNewServiceInstance(const SAFplus::Handle& mgmtHandle, const char* siName, const char* sgName, const char* csiName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new si");
  SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si = new SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig();
  si->set_name(siName);
  si->set_adminstate(AdministrativeState::AdministrativeState_on);
  si->add_componentserviceinstances(csiName, strlen(csiName));
  si->set_servicegroup(sgName, strlen(sgName));  
  ClRcT rc = SAFplus::amfMgmtServiceInstanceCreate(mgmtHandle,si);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new si returns [0x%x]", rc);
  return rc;
}

ClRcT addNewServiceGroup(const SAFplus::Handle& mgmtHandle, const char* sgName, const char* suName, const char* siName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating sg");
  SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg = new SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig();
  sg->set_name(sgName);  
  sg->add_serviceunits(suName, strlen(suName));
  sg->add_serviceinstances(siName, strlen(siName));
  sg->set_autorepair(true);
  ClRcT rc = SAFplus::amfMgmtServiceGroupCreate(mgmtHandle,sg);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating sg returns [0x%x]", rc);
  return rc;
}

ClRcT addNewComponentServiceInstance(const SAFplus::Handle& mgmtHandle, const char* csiName, const char* siName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new csi");
  SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi = new SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig();
  csi->set_name(csiName);  
  csi->set_serviceinstance(siName, strlen(siName));  
  csi->add_data();
  csi->add_data();
  //Fill data
  Data* data = csi->mutable_data(0);
  data->set_name("ip");
  data->set_val("192.168.65.111");
  data = csi->mutable_data(1);
  data->set_name("port");
  data->set_val("35678");
  ClRcT rc = SAFplus::amfMgmtComponentServiceInstanceCreate(mgmtHandle,csi);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating new csi returns [0x%x]", rc);
  return rc;
}

ClRcT updateServiceGroup(const SAFplus::Handle& mgmtHandle, const char* sgName, const char* suName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating sg");
  SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg = new SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig();
  sg->set_name(sgName);  
  sg->add_serviceunits(suName, strlen(suName));
  ClRcT rc = SAFplus::amfMgmtServiceGroupConfigSet(mgmtHandle,sg);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating sg returns [0x%x]", rc);
  return rc;
}

ClRcT updateNode(const SAFplus::Handle& mgmtHandle, const char* nodeName, const char* suName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating node");
  SAFplus::Rpc::amfMgmtRpc::NodeConfig* node = new SAFplus::Rpc::amfMgmtRpc::NodeConfig();
  node->set_name(nodeName);  
  node->add_serviceunits(suName, strlen(suName));
  ClRcT rc = SAFplus::amfMgmtNodeConfigSet(mgmtHandle,node);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating node returns [0x%x]", rc);
  return rc;
}

ClRcT updateServiceUnit(const SAFplus::Handle& mgmtHandle, const char* suName, const char* compName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating su");
  SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su = new SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig();
  su->set_name(suName);  
  su->add_components(compName, strlen(compName));
  ClRcT rc = SAFplus::amfMgmtServiceUnitConfigSet(mgmtHandle,su);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating su returns [0x%x]", rc);
  return rc;
}

ClRcT addNewComponent(const SAFplus::Handle& mgmtHandle, const char* binary, const char* compName, const char* suName)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating component c99");
  SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
  comp->set_name(compName);
  comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_x_active_or_y_standby);
  Execution* exe = new Execution();
  char cmd[SAFplus::CL_MAX_NAME_LENGTH];
  strcpy(cmd, binary);
  strcat(cmd, " ");
  strcat(cmd,compName);
  exe->set_command(cmd);
  uint64_t timeout = 10000;
  exe->set_timeout(timeout);
  Instantiate* inst = new Instantiate();
  inst->set_allocated_execution(exe);
  comp->set_allocated_instantiate(inst);
  comp->set_serviceunit(suName);
  ClRcT rc = SAFplus::amfMgmtComponentCreate(mgmtHandle,comp);
  //rc = SAFplus::amfMgmtComponentConfigSet(mgmtHandle,comp);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating component returns [0x%x]", rc);
  return rc;  
}

ClRcT updateComponent(const SAFplus::Handle& mgmtHandle)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating component");
  SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
  comp->set_name("c1xx");
  ClRcT rc = SAFplus::amfMgmtComponentConfigSet(mgmtHandle,comp);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating component returns [0x%x]", rc);
  return rc;
}

ClRcT deleteComponent(const SAFplus::Handle& mgmtHandle)
{
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test deleting component");
  std::string compName("c1xx");
  ClRcT rc = SAFplus::amfMgmtComponentDelete(mgmtHandle,compName);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test deleting component returns [0x%x]", rc);
  return rc;
}

#if 0
void* standbyLoop(void* arg)
{
  SAFplus::Handle mgmtHandle;
  ClRcT rc = SAFplus::amfMgmtInitialize(mgmtHandle);
  if (rc != CL_OK)
  {
    clprintf(SAFplus::LOG_SEV_ERROR,"Basic HA app: amfMgmtInitialize error rc [0x%x]", rc);
    return NULL;
  }
 
  rc = amfMgmtComponentDelete(mgmtHandle,std::string("c0"));
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: componentDelete returns [0x%x]", rc);  
  rc = addNewComponent(mgmtHandle);
  /*if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
  }*/
  rc = updateServiceUnit(mgmtHandle,"su0");  
  if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
  }
  rc = SAFplus::amfMgmtFinalize(mgmtHandle);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: finalize returns [0x%x]", rc);
  return NULL;
}
#endif

ClRcT dynamicModelUpdate()
{
  ClRcT rc = CL_OK;
#if 0
  SAFplus::Handle mgmtHandle;    
  FN_CALL(SAFplus::amfMgmtInitialize(mgmtHandle));
  /*if (rc != CL_OK)
  {
    clprintf(SAFplus::LOG_SEV_ERROR,"Basic HA app: amfMgmtInitialize error rc [0x%x]", rc);
    return rc;
  } */
  FN_CALL(addNewComponent(mgmtHandle, "./basicApp", "c12", "newsu");
  /*if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
  }*/
  if (rc == CL_OK)
  {
    rc = addNewServiceUnit(mgmtHandle,"newsu", "c12", "sg0", "node1");  
  }
  if (rc == CL_OK)
  {
    rc = updateServiceGroup(mgmtHandle,"sg0", "newsu");
  }
  if (rc == CL_OK)
  {
    rc = updateNode(mgmtHandle,"node1", "newsu");
  }
  if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
  }
  rc = SAFplus::amfMgmtFinalize(mgmtHandle);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: finalize returns [0x%x]", rc);
#endif
  return rc;
}


ClRcT dynamicModelCreate()
{
  SAFplus::Handle mgmtHandle;
  ClRcT rc = CL_OK;
  FN_CALL(SAFplus::amfMgmtInitialize(mgmtHandle));
  /*if (rc != CL_OK)
  {
    clprintf(SAFplus::LOG_SEV_ERROR,"Basic HA app: amfMgmtInitialize error rc [0x%x]", rc);
    return rc;
  }*/
  FN_CALL(addNewServiceGroup(mgmtHandle, "newsg","newsu","newsi"));
  FN_CALL(addNewServiceInstance(mgmtHandle, "newsi","newsg","newcsi"));
  FN_CALL(addNewComponentServiceInstance(mgmtHandle, "newcsi","newsi"));
  FN_CALL(addNewServiceUnit(mgmtHandle, "newsu","newcomp","newsg", "node1"));
  FN_CALL(addNewComponent(mgmtHandle, "./basicApp", "newcomp", "newsu"));

  /*if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
  }
  if (rc == CL_OK)
  {
    rc = addNewServiceUnit(mgmtHandle,"newsu", "c12", "sg0", "node1");  
  }
  if (rc == CL_OK)
  {
    rc = updateServiceGroup(mgmtHandle,"sg0", "newsu");
  }
  if (rc == CL_OK)
  {
    rc = updateNode(mgmtHandle,"node1", "newsu");
  }*/
  if (rc == CL_OK)
  {
    rc = SAFplus::amfMgmtCommit(mgmtHandle);
    clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: dynamicModelCreate commit returns [0x%x]", rc);
  }
  rc = SAFplus::amfMgmtFinalize(mgmtHandle);
  clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: dynamicModelCreate finalize returns [0x%x]", rc);
  return rc;
}

void* standbyLoop(void* arg)
{
  char* mode = (char*)arg;
  if (strcmp(mode,"create")==0)
  {
    dynamicModelCreate();
  }
  else if (strcmp(mode,"update")==0)
  {
    dynamicModelUpdate();
  }
  else
  {
    logNotice("APP", "---", "invalid argument for standby loop, the mode is [%s]", mode);
  }
  delete (char*)arg;
  return NULL;
}

//? <excerpt id="AMFBasicMain">
int main(int argc, char *argv[])
{
    SaAisErrorT rc = SA_AIS_OK;
 
    // You can override the component name that appears in logs like this.
    // Otherwise it gets set to the component name defined in the AMF data.
    // Typically you'd override it if your component name is inconveniently long.
    //SAFplus::logCompName = DEFAULT_APP_NAME;

    // If you wanted this component to use a "well-known" port you would set it like this
    // SAFplus::serviceConfig.iocPort = SAFplus::MsgApplicationPortStart + 5;
    // Otherwise, SAFplus AMF will assign a port.
    SAFplus::logEchoToFd = 1;
    initializeAmf(); // Connect to the AMF cluster

    // You can override the logging parameters, but it must be done AFTER
    // calling initializeAmf() because that function loads the logSeverity
    //  from the environment variable CL_LOG_SEVERITY.
    //SAFplus::logEchoToFd = 1;  // echo logs to stdout (fd 1) for debugging
    //SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;

    /* Do any application specific initialization here. */
    
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();
    
    /* Do the application specific finalization here. */

    /* Now finalize my connection with the SAF cluster */
    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
      logError("APP","FIN", "AMF finalization error[0x%X]", rc);
    else
      logInfo("APP","FIN", "AMF Finalized");   

    return 0;
}
//? </excerpt>

/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

void safTerminate(SaInvocationT invocation, const SaNameT *compName)
  {
  SaAisErrorT rc = SA_AIS_OK;

  clprintf (SAFplus::LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Terminating\n", compName->length, compName->value, SAFplus::pid);

  /*
   * Unregister with AMF and respond to AMF saying whether the
   * termination was successful or not.
   */
  if ( (rc = saAmfComponentUnregister(amfHandle, compName, NULL)) != SA_AIS_OK)
    {
    clprintf (SAFplus::LOG_SEV_ERROR, "Component [%.*s] : PID [%d]. Unregister failed with error [0x%x]\n", compName->length, compName->value, SAFplus::pid, rc);
    return;
    }

  /* Ok tell SAFplus that we handled it properly */
  saAmfResponse(amfHandle, invocation, SA_AIS_OK);

  clprintf (SAFplus::LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Terminated\n", compName->length, compName->value, SAFplus::pid);

  quitting = true;
  }

/******************************************************************************
 * Application Work Assignment Functions
 *****************************************************************************/

/*
 * safAssignWork
 * ------------------
 * This function is invoked when a CSI assignment is made or the state
 * of a CSI is changed.
 */
//? <excerpt id="AMFBasicWorkAssignment">
void safAssignWork(SaInvocationT       invocation,
                   const SaNameT       *compName,
                   SaAmfHAStateT       haState,
                   SaAmfCSIDescriptorT csiDescriptor)
{
    /* Print information about the CSI Set */

    clprintf (SAFplus::LOG_SEV_INFO, "Component [%.*s] : PID [%d]. CSI Set Received\n", 
              compName->length, compName->value, SAFplus::pid);

    printCSI(csiDescriptor, haState);

    /*
     * Take appropriate action based on state
     */

    switch ( haState )
    {
        /* AMF asks this process to take the standby HA state for the work
           described in the csiDescriptor variable */
        case SA_AMF_HA_ACTIVE:
        {
            /* Typically you would spawn a thread here to initiate active 
               processing of the work. */
            pthread_t thr;
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: ACTIVE state requested; activating service");
            running = 1;
            //pthread_create(&thr,NULL,activeLoop,NULL);


            /* The AMF times the interval between the assignment and acceptance
               of the work (the time interval is configurable).
               So it is important to call this saAmfResponse function ASAP.
             */
            saAmfResponse(amfHandle, invocation, SA_AIS_OK);
            
            break;
        }

        /* AMF asks this process to take the standby HA state for the work
           described in the csiDescriptor variable */
        case SA_AMF_HA_STANDBY:
        {
            /* If your standby has ongoing maintenance, you would spawn a thread
               here to do it. */

            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Standby state requested");
            running = 2;

            /* The AMF times the interval between the assignment and acceptance
               of the work (the time interval is configurable).
               So it is important to call this saAmfResponse function ASAP.
             */
            pthread_t thr;
            char* mode = new char[10];
            strcpy(mode,"create");
            pthread_create(&thr,NULL,standbyLoop, mode);
            saAmfResponse(amfHandle, invocation, SA_AIS_OK); 
#if 0
            SAFplus::Handle mgmtHandle;
            ClRcT rc = SAFplus::amfMgmtInitialize(mgmtHandle);
            if (rc != CL_OK)
            {
              clprintf(SAFplus::LOG_SEV_ERROR,"Basic HA app: amfMgmtInitialize error rc [0x%x]", rc);
              break;
            }
#if 1
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating component");
            SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
            comp->set_name("c1xx");
            rc = SAFplus::amfMgmtComponentCreate(mgmtHandle,comp);
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test creating component returns [0x%x]", rc);
            if (rc == CL_OK)
            {           
              rc = SAFplus::amfMgmtCommit(mgmtHandle);
              clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: commit returns [0x%x]", rc);
            }
            rc = SAFplus::amfMgmtFinalize(mgmtHandle);
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: finalize returns [0x%x]", rc);
#endif
#endif
            #if 0
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating component");
            SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp = new SAFplus::Rpc::amfMgmtRpc::ComponentConfig();
            comp->set_name("c0");
            comp->set_capabilitymodel(SAFplus::Rpc::amfMgmtRpc::CapabilityModel_not_preinstantiable);
            ClRcT rc = SAFplus::amfMgmtComponentConfigSet(comp);
            clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Test updating component returns [0x%x]", rc);
            #endif
            break;
        }

        case SA_AMF_HA_QUIESCED:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active or quiescing HA state. The application 
             * must stop work associated with the CSI immediately.
             */
            clprintf(SAFplus::LOG_SEV_INFO, "Basic HA app: Acknowledging new state quiesced");
            running = 0;

            saAmfResponse(amfHandle, invocation, SA_AIS_OK);
            break;
        }

        case SA_AMF_HA_QUIESCING:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active HA state. The application must stop work
             * associated with the CSI gracefully and not accept any new
             * workloads while the work is being terminated.
             */

            /* There are two typical cases for quiescing.  Chooose one!
               CASE 1: Its possible to quiesce rapidly within this thread context */
            if (1)
              {
              /* App code here: Now finish your work and cleanly stop the work*/
              clprintf(SAFplus::LOG_SEV_INFO, "Basic HA app: Signaling completion of QUIESCING");
              running = 0;
            
              /* Call saAmfCSIQuiescingComplete when stopping the work is done */
              saAmfCSIQuiescingComplete(amfHandle, invocation, SA_AIS_OK);
              }
            else
              {
              /* CASE 2: You can't quiesce within this thread context or quiesce
               rapidly. */

              /* Respond immediately to the quiescing request */
              saAmfResponse(amfHandle, invocation, SA_AIS_OK);

              /* App code here: Signal or spawn a thread to cleanly stop the work*/
              /* When that thread is done, it should call:
                 saAmfCSIQuiescingComplete(amfHandle, invocation, SA_AIS_OK);
              */
              }

            break;
        }

        default:
        {
            //assert(0);
            clprintf(SAFplus::LOG_SEV_INFO, "Basic HA app: unknown HA state received [%d]", haState);
            break;
        }
    }

    return;
}
//? </excerpt>

/*
 * safRemoveWork
 * ---------------------
 * This function is invoked when a CSI assignment is to be removed.
 */

void safRemoveWork(SaInvocationT  invocation,
                   const SaNameT  *compName,
                   const SaNameT  *csiName,
                   SaAmfCSIFlagsT csiFlags)
{
    clprintf (SAFplus::LOG_SEV_INFO, "Component [%.*s] : PID [%d]. CSI Remove Received\n", 
              compName->length, compName->value, SAFplus::pid);

    clprintf (SAFplus::LOG_SEV_INFO, "   CSI                     : %.*s\n", csiName->length, csiName->value);
    clprintf (SAFplus::LOG_SEV_INFO, "   CSI Flags               : 0x%d\n", csiFlags);

    /*
     * Add application specific logic for removing the work for this CSI.
     */

    saAmfResponse(amfHandle, invocation, SA_AIS_OK);
}

//? <excerpt id="AMFBasicInitializeAmf">
void initializeAmf(void)
  {
  SaAmfCallbacksT     callbacks;
  SaAisErrorT         rc = SA_AIS_OK;

  callbacks.saAmfHealthcheckCallback          = NULL; /* rarely necessary because SAFplus monitors the process */
  callbacks.saAmfComponentTerminateCallback   = safTerminate;
  callbacks.saAmfCSISetCallback               = safAssignWork;
  callbacks.saAmfCSIRemoveCallback            = safRemoveWork;
  callbacks.saAmfProtectionGroupTrackCallback = NULL;

  /* Initialize AMF client library. */
  if ( (rc = saAmfInitialize(&amfHandle, &callbacks, &SAFplus::safVersion)) != SA_AIS_OK)
    errorExit(rc);

#if 1
  if ( (rc = saAmfComponentNameGet(amfHandle, &appName)) != SA_AIS_OK)
    {
    if (rc == SA_AIS_ERR_UNAVAILABLE)  // This component was not run by the AMF.  You must tell us what the component name is.
      {
      logInfo("APP","MAIN","Application NOT started by AMF");
      SAFplus::saNameSet(&appName, DEFAULT_APP_NAME);
      }
    }

  if ( (rc = saAmfComponentRegister(amfHandle, &appName, NULL)) != SA_AIS_OK) 
    errorExit(rc);
#else

  // If spawned by the AMF, you can just pass NULL as the component name since
  // the AMF has told this app its name (SAFplus::COMP_NAME variable)
  if ( (rc = saAmfComponentRegister(amfHandle, NULL, NULL)) != SA_AIS_OK)
    errorExit(rc);
#endif

  logInfo("APP","MAIN","Component [%s] registered successfully", appName.value);
  }
//? </excerpt>

//? <excerpt id="AMFBasicDispatchLoop">
void dispatchLoop(void)
{
  SaAisErrorT         rc = SA_AIS_OK;
  SaSelectionObjectT amf_dispatch_fd;
  int maxFd;
  fd_set read_fds;

  /* This boilerplate code includes an example of how to simultaneously
     dispatch for 2 services (in this case AMF and CKPT).  But since checkpoint
     is not initialized or used, it is commented out */

  /*
   * Get the AMF dispatch FD for the callbacks
   */
  if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
    errorExit(rc);
  /* if ( (rc = saCkptSelectionObjectGet(ckptLibraryHandle, &ckpt_dispatch_fd)) != SA_AIS_OK)
       errorExit(rc); */

  maxFd = amf_dispatch_fd;  /* maxFd = max(amf_dispatch_fd,ckpt_dispatch_fd); */
  do
    {
      struct timeval timeout;
      timeout.tv_sec = 1; timeout.tv_usec = 0;  // you may want to increase the idle timeout...

      FD_ZERO(&read_fds);
      FD_SET(amf_dispatch_fd, &read_fds);
      /* FD_SET(ckpt_dispatch_fd, &read_fds); */
      if(!(select(maxFd + 1, &read_fds, NULL, NULL, &timeout) >= 0))
        {
          char errorStr[80];
          int err = errno;
          if (EINTR == err) continue;

          errorStr[0] = 0; /* just in case strerror does not fill it in */
          strerror_r(err, errorStr, 79);
          clprintf (SAFplus::LOG_SEV_ERROR, "Error [%d] during dispatch loop select() call: [%s]",err,errorStr);
          break;
        }
      if (FD_ISSET(amf_dispatch_fd,&read_fds)) 
        {
        saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
        }
      else 
        {
        clprintf(SAFplus::LOG_SEV_TRACE,"Select returned but nothing to dispatch");
        }
      /* if (FD_ISSET(ckpt_dispatch_fd,&read_fds)) saCkptDispatch(ckptLibraryHandle, SA_DISPATCH_ALL); */
 
      /*if (running==1) clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Active.  Hello World!"); // show_progress());
      else if (running==2) clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Standby."); // show_progress());
      else 
        {
        clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: idle");
        SAFplus::name.set(SAFplus::ASP_COMPNAME,SAFplus::myHandle,SAFplus::NameRegistrar::MODE_NO_CHANGE);

        }*/
    }while(!quitting);
}
//? </excerpt>


#define STRING_HA_STATE(S)                                                  \
(   ((S) == SA_AMF_HA_ACTIVE)             ? "Active" :                \
    ((S) == SA_AMF_HA_STANDBY)            ? "Standby" :               \
    ((S) == SA_AMF_HA_QUIESCED)           ? "Quiesced" :              \
    ((S) == SA_AMF_HA_QUIESCING)          ? "Quiescing" :             \
                                            "Unknown" )

#define STRING_CSI_FLAGS(S)                                                 \
(   ((S) == SA_AMF_CSI_ADD_ONE)            ? "Add One" :               \
    ((S) == SA_AMF_CSI_TARGET_ONE)         ? "Target One" :            \
    ((S) == SA_AMF_CSI_TARGET_ALL)         ? "Target All" :            \
                                                  "Unknown" )


/*
 * printCSI
 * --------------------
 * Print information received in a CSI set request.
 */

void printCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState)
  {
  clprintf (SAFplus::LOG_SEV_INFO, "CSI Flags : [%s]", STRING_CSI_FLAGS(csiDescriptor.csiFlags));

  if (SA_AMF_CSI_TARGET_ALL != csiDescriptor.csiFlags)
    {
    clprintf (SAFplus::LOG_SEV_INFO, "CSI Name : [%s]", csiDescriptor.csiName.value);
    }

  if (SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags)
    {
    ClUint32T i = 0;

    clprintf (SAFplus::LOG_SEV_INFO, "Name value pairs :");
    for (i = 0; i < csiDescriptor.csiAttr.number; i++)
      {
      clprintf (SAFplus::LOG_SEV_INFO, "Name : [%s] Value : [%s]",csiDescriptor.csiAttr.attr[i].attrName, csiDescriptor.csiAttr.attr[i].attrValue);
      }
    }

  clprintf (SAFplus::LOG_SEV_INFO, "HA state : [%s]", STRING_HA_STATE(haState));

  if (SA_AMF_HA_ACTIVE == haState)
    {
    clprintf (SAFplus::LOG_SEV_INFO, "Active Descriptor :");
    clprintf (SAFplus::LOG_SEV_INFO,
      "Transition Descriptor : [%d]",
      csiDescriptor.csiStateDescriptor.
      activeDescriptor.transitionDescriptor);
    clprintf (SAFplus::LOG_SEV_INFO,
      "Active Component : [%s]",
      csiDescriptor.csiStateDescriptor.
      activeDescriptor.activeCompName.value);
    }
  else if (SA_AMF_HA_STANDBY == haState)
    {
    clprintf (SAFplus::LOG_SEV_INFO, "Standby Descriptor :");
    clprintf (SAFplus::LOG_SEV_INFO,
      "Standby Rank : [%d]",
      csiDescriptor.csiStateDescriptor.
      standbyDescriptor.standbyRank);
    clprintf (SAFplus::LOG_SEV_INFO, "Active Component : [%s]",
      csiDescriptor.csiStateDescriptor.
      standbyDescriptor.activeCompName.value);
    }
  }
