// gcc -c custom.c -o custom.o -I/usr/include/python2.5
// gcc -shared custom.o -o pyasp.so

#include <Python.h>

/*
 * POSIX Includes.
 */
#include <assert.h>
#include <stdlib.h>

/*
 * Basic ASP Includes.
 */
#include <clCommon.h>
/*
 * ASP Client Includes.
 */
#include <clLogApi.h>
#include <clCpmApi.h>
#include <saAmf.h>
#include <clEoConfigApi.h>


#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
                                  __VA_ARGS__)

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

#include <clDebugApi.h>

/* #include "clCompAppMain.h" */

static int clCreateCsiPython(char* s, int maxLen,const SaNameT* compName, SaAmfCSIDescriptorT* csiDescriptor, SaAmfHAStateT haState);
static void clRunPython(char* cmd);
static void clCompAppTerminate(SaInvocationT invocation, const SaNameT *compName);
static ClRcT clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback);

static void clCompAppAMFCSISet(SaInvocationT       invocation,
                        const SaNameT       *compName,
                        SaAmfHAStateT       haState,
                               SaAmfCSIDescriptorT csiDescriptor);
static void clCompAppAMFCSIRemove(SaInvocationT  invocation,
                           const SaNameT  *compName,
                           const SaNameT  *csiName,
                                  SaAmfCSIFlagsT csiFlags);


pid_t               mypid;
SaAmfHandleT        amfHandle;
PyInterpreterState *thePythonInterpreter = NULL;


ClBoolT unblockNow = CL_FALSE;

/*
 * Description of this EO
 */

#define COMP_EO_NAME    getenv(ASP_COMPNAME)
static ClRcT clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback);

ClEoConfigT clEoConfig =
{
    "3RD_PARTY_COMP",               /* EO Name                                  */
    CL_OSAL_THREAD_PRI_MEDIUM,   /* EO Thread Priority                       */
    2,         /* No of EO thread needed                   */
    0,              /* Required Ioc Port                        */
    CL_EO_USER_CLIENT_ID_START + 0, 
    CL_EO_USE_THREAD_FOR_APP,   /* Thread Model                             */
    NULL,                       /* Application Initialize Callback          */
    NULL,                       /* Application Terminate Callback           */
    NULL,       /* Application State Change Callback        */
    NULL,       /* Application Health Check Callback        */
};


/*
 * Basic libraries used by this EO. The first 6 libraries are
 * mandatory, the others can be enabled or disabled by setting to
 * CL_TRUE or CL_FALSE.
 */
ClUint8T clEoBasicLibs[] =
{
  CL_TRUE,  /*     COMP_EO_BASICLIB_OSAL,      Lib: Operating System Adaptation Layer   */
  CL_TRUE,  /*     COMP_EO_BASICLIB_TIMER,     Lib: Timer                               */
  CL_TRUE,  /*     COMP_EO_BASICLIB_BUFFER,    Lib: Buffer Management                   */
  CL_TRUE,  /*     COMP_EO_BASICLIB_IOC,       Lib: Intelligent Object Communication    */
  CL_TRUE,  /*     COMP_EO_BASICLIB_RMD,       Lib: Remote Method Dispatch              */
  CL_TRUE,  /*     COMP_EO_BASICLIB_EO,        Lib: Execution Object                    */
  CL_FALSE, /*     COMP_EO_BASICLIB_OM,        Lib: Object Management                   */
  CL_FALSE, /*     COMP_EO_BASICLIB_HAL,       Lib: Hardware Adaptation Layer           */
  CL_FALSE, /*     COMP_EO_BASICLIB_DBAL,      Lib: Database Adaptation Layer           */
};

ClUint8T clEoClientLibs[] =
{
    CL_FALSE,    /* COMP_EO_CLIENTLIB_COR,      Lib: Common Object Repository            */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_CM,        Lib: Chassis Management                  */
    CL_TRUE,     /*     COMP_EO_CLIENTLIB_NAME,      Lib: Name Service                        */
    CL_TRUE,    /*     COMP_EO_CLIENTLIB_LOG,       Lib: Log Service                         */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_TRACE,     Lib: Trace Service                       */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_DIAG,      Lib: Diagnostics                         */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_TXN,       Lib: Transaction Management              */
    CL_FALSE,                  /*   NA */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_PROV,      Lib: Provisioning Management             */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_ALARM,     Lib: Alarm Management                    */
    CL_TRUE,    /*     COMP_EO_CLIENTLIB_DEBUG,     Lib: Debug Service                       */
    CL_FALSE,    /*     COMP_EO_CLIENTLIB_GMS,        Lib: Cluster/Group Membership Service    */
    CL_FALSE
};

/*
 * clCompAppAMFPrintCSI
 * --------------------
 * Print information received in a CSI set request.
 */

static void clCompAppAMFPrintCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState)
{
    clprintf (CL_LOG_SEV_INFO,
              "CSI Flags : [%s]",
              STRING_CSI_FLAGS(csiDescriptor.csiFlags));

    if (SA_AMF_CSI_TARGET_ALL != csiDescriptor.csiFlags)
    {
        clprintf (CL_LOG_SEV_INFO, "CSI Name : [%s]", 
                  csiDescriptor.csiName.value);
    }

    if (SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags)
    {
        ClUint32T i = 0;
        
        clprintf (CL_LOG_SEV_INFO, "Name value pairs :");
        for (i = 0; i < csiDescriptor.csiAttr.number; i++)
        {
            clprintf (CL_LOG_SEV_INFO, "Name : [%s]",
                      csiDescriptor.csiAttr.
                      attr[i].attrName);
            clprintf (CL_LOG_SEV_INFO, "Value : [%s]",
                      csiDescriptor.csiAttr.
                      attr[i].attrValue);
        }
    }
    
    clprintf (CL_LOG_SEV_INFO, "HA state : [%s]",
              STRING_HA_STATE(haState));

    if (SA_AMF_HA_ACTIVE == haState)
    {
        clprintf (CL_LOG_SEV_INFO, "Active Descriptor :");
        clprintf (CL_LOG_SEV_INFO,
                  "Transition Descriptor : [%d]",
                  csiDescriptor.csiStateDescriptor.
                  activeDescriptor.transitionDescriptor);
        clprintf (CL_LOG_SEV_INFO,
                  "Active Component : [%s]",
                  csiDescriptor.csiStateDescriptor.
                  activeDescriptor.activeCompName.value);
    }
    else if (SA_AMF_HA_STANDBY == haState)
    {
        clprintf (CL_LOG_SEV_INFO, "Standby Descriptor :");
        clprintf (CL_LOG_SEV_INFO,
                  "Standby Rank : [%d]",
                  csiDescriptor.csiStateDescriptor.
                  standbyDescriptor.standbyRank);
        clprintf (CL_LOG_SEV_INFO, "Active Component : [%s]",
                  csiDescriptor.csiStateDescriptor.
                  standbyDescriptor.activeCompName.value);
    }
}


static void clPyGlueCsiSet(
                     const SaNameT       *compName,
                     SaAmfHAStateT       haState,
                     SaAmfCSIDescriptorT* csiDescriptor)
{
#define BSZ 2048
  char s[BSZ];
  int curlen = 0;
  int ok = 1;

  switch ( haState )
    {
    case CL_AMS_HA_STATE_ACTIVE:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.ActivateWorkAssignment(");
        break;
      }

    case CL_AMS_HA_STATE_STANDBY:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.StandbyWorkAssignment(");
        break;
      }

    case CL_AMS_HA_STATE_QUIESCED:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.AbortWorkAssignment(");
        break;
      }

    case CL_AMS_HA_STATE_QUIESCING:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.QuiesceWorkAssignment(");
        break;
      }

    default:
      {
        ok = 0;  // Its NOT ok
        break;
      }
    }

  if (ok)
    {
      curlen += clCreateCsiPython(s+curlen, BSZ-curlen, compName, csiDescriptor, haState);
      curlen += snprintf (s+curlen,BSZ-curlen, ")\n");
      clprintf (CL_LOG_SEV_INFO, "%s",s);
      clRunPython(s);

#if 0
      if (strlen(csiDescriptor->csiName.value))
        {
          /* Just for debugging use */
          curlen = snprintf(s,BSZ,"%s = ", csiDescriptor->csiName.value);
          curlen += clCreateCsiPython(s+curlen, BSZ-curlen, compName, csiDescriptor, haState);
          clprintf (CL_LOG_SEV_INFO, "%s",s);
          clRunPython(s);
        }
#endif

    }
}


static void clPyGlueCsiRemove(
                       const SaNameT       *compName,
                       const SaNameT       *csiName,
                       SaAmfCSIFlagsT      csiFlags)
{
  char s[1024];
  snprintf(s,1024,"eoApp.RemoveWorkAssignment('%s',%s)\n", csiName->value, (csiFlags & CL_AMS_CSI_FLAG_TARGET_ALL) ? "True":"False");
  clRunPython(s);
}



static void clRunPython(char* cmd)
{ 
  #define BSZ 2048
  char pyBuf[BSZ];
  //clprintf (CL_LOG_SEV_INFO, "clRunPython called with [%s]", cmd);
  //clOsalMutexLock(&pyMutex);
  PyThreadState *tstate;
  //PyObject *result;
  
  snprintf(pyBuf,BSZ,"safplus.Callback(\"\"\"%s\"\"\")\n",cmd);
  //clprintf (CL_LOG_SEV_INFO, "clRunPython requesting python lock [%s]", cmd);

  /* interp is your reference to an interpreter object. */
  tstate = PyThreadState_New(thePythonInterpreter);
  PyEval_AcquireThread(tstate);
  
  //clprintf(CL_LOG_SEV_INFO,"Stage 1.  Passing to Python layer: %s",pyBuf);
  int ret = PyRun_SimpleString(pyBuf);
  //clprintf (CL_LOG_SEV_INFO, "clRunPython requesting release of python lock [%s]", cmd);

  PyEval_ReleaseThread(tstate);

  if (ret != 0)  clprintf(CL_LOG_SEV_ERROR,"Ran: %s.  There was an error.",pyBuf);

  PyThreadState_Delete(tstate);
}
#undef BSZ



#if 0
static void clRunPython(char* cmd)
{ 
  #define BSZ 2048
  char pyBuf[BSZ];
  clprintf (CL_LOG_SEV_INFO, "clRunPython called with [%s]", cmd);
  //clOsalMutexLock(&pyMutex);
  thrdState = PyThreadState_Get();
  
  #if 0
  if (quit)
    {
      clprintf(CL_LOG_SEV_INFO,"Python has quit, so not running: %s",cmd);
      //clOsalMutexUnlock(&pyMutex);
      return;
    }
  #endif

#if 0
  PyEval_AcquireLock();
  PyThreadState_Swap(thrdState);
#endif

  snprintf(pyBuf,BSZ,"clCmdFromAsp(\"\"\"%s\"\"\")\n",cmd);

  clprintf (CL_LOG_SEV_INFO, "clRunPython requesting python lock [%s]", cmd);
  PyEval_AcquireThread(thrdState);
  clprintf(CL_LOG_SEV_INFO,"Stage 1.  Passing to Python layer: %s",pyBuf);
  int ret = PyRun_SimpleString(pyBuf);
  clprintf (CL_LOG_SEV_INFO, "clRunPython requesting release of python lock [%s]", cmd);
  PyEval_ReleaseThread(thrdState);

  if (ret != 0)  clprintf(CL_LOG_SEV_ERROR,"Ran: %s.  There was an error.",pyBuf);

#if 0
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
#endif

  //clprintf (CL_LOG_SEV_INFO, "clRunPython unlocking mutex [%s]", cmd);
  //clOsalMutexUnlock(&pyMutex);
  //clprintf (CL_LOG_SEV_INFO, "clRunPython unlocked mutex [%s]", cmd);
}
#undef BSZ
#endif

static int clCreateCsiPython(char* s, int maxLen, const SaNameT* compName, SaAmfCSIDescriptorT* csiDescriptor, SaAmfHAStateT haState)
{
  //  #define maxLen 2048
  //  char s[maxLen];
  int curlen = 0;

  //snprintf(s+curlen,maxLen-curlen,"csi = { 'flags':'0' }\n"); 

  curlen += snprintf (s+curlen,maxLen-curlen, "{ 'csiFlags':'%s'",STRING_CSI_FLAGS(csiDescriptor->csiFlags));

  curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiHaState':'%s' ",STRING_HA_STATE(haState));

  if (CL_AMS_CSI_FLAG_TARGET_ALL != csiDescriptor->csiFlags)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiName':'%s' ", csiDescriptor->csiName.value);
    }

  if ((CL_AMS_CSI_FLAG_ADD_ONE == csiDescriptor->csiFlags) || (CL_AMS_CSI_FLAG_TARGET_ONE == csiDescriptor->csiFlags))
    {
      ClUint32T i = 0;
        
      //clprintf (CL_LOG_SEV_INFO, "   Name Value Pairs        : \n");
      for (i = 0; i < csiDescriptor->csiAttr.number; i++)
        {
          curlen += snprintf (s+curlen,maxLen-curlen, ", '%s':'%s' ", csiDescriptor->csiAttr.attr[i].attrName, csiDescriptor->csiAttr.attr[i].attrValue);
        }

      if (0)  
        {
          ClRcT rc;
          ClCpmCompCSIRefT csiRef = { 0 };
          unsigned int i,j;
          ClNameT clcName;
          
          clprintf(CL_LOG_SEV_INFO,"New CODE");

          strncpy (clcName.value, (ClCharT*) compName->value,compName->length);
          clcName.length = compName->length;
          clcName.value[clcName.length]=0;

          rc = clCpmCompCSIList(&clcName, &csiRef);
          if(rc != CL_OK)
            {
              clLogError("APP", "CSISET", "Comp CSI get returned [%#x]", rc);
            }
          else
            {
              for(i = 0; i < csiRef.numCSIs; ++i)
                {
                  ClCpmCompCSIT *pCSI = &csiRef.pCSIList[i];
                  clprintf(CL_LOG_SEV_INFO,"me: [%.*s] compared: [%.*s]",csiDescriptor->csiName.length,csiDescriptor->csiName.value, pCSI->csiDescriptor.csiName.length, pCSI->csiDescriptor.csiName.value);
                  if (strncmp((char*)csiDescriptor->csiName.value,(char*)pCSI->csiDescriptor.csiName.value,csiDescriptor->csiName.length)==0)
                    {
                      ClAmsCSIDescriptorT* cd = &pCSI->csiDescriptor;
                      for (j = 0; j < cd->csiAttributeList.numAttributes; j++)
                        {
                          curlen += snprintf (s+curlen,maxLen-curlen, ", '%s':'%s' ", cd->csiAttributeList.attribute[j].attributeName, cd->csiAttributeList.attribute[j].attributeValue);
                          clprintf(CL_LOG_SEV_INFO,s);
                        }
                   }
                }
              if(csiRef.pCSIList)
                clHeapFree(csiRef.pCSIList);
            }
        }
    }
    

  if (SA_AMF_HA_ACTIVE == haState)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiTransitionDescriptor':'%d' ", (int) csiDescriptor->csiStateDescriptor.activeDescriptor.transitionDescriptor);
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiActiveComponent':'%s' ", csiDescriptor->csiStateDescriptor.activeDescriptor.activeCompName.value);
    }
  else if (SA_AMF_HA_STANDBY == haState)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiStandbyRank':'%d' ", (int) csiDescriptor->csiStateDescriptor.standbyDescriptor.standbyRank);
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiActiveComponent':'%s' ", csiDescriptor->csiStateDescriptor.activeDescriptor.activeCompName.value);
    }


  curlen += snprintf (s+curlen,maxLen-curlen, "}");

  //clprintf (CL_LOG_SEV_INFO, "%s",s);
  //clRunPython(s);

  return curlen;

}


static void* amfDispatcher(void *param)
{
    SaSelectionObjectT dispatch_fd;
    fd_set read_fds;
    ClRcT rc;

    FD_ZERO(&read_fds);

    /*
     * Get the AMF dispatch FD for the callbacks
     */
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &dispatch_fd)) != SA_AIS_OK)
    {
        clprintf (CL_LOG_SEV_ERROR, "Error getting selection object -- unable to dispatch AMF events");
        return NULL;        
    }
    
    
    FD_SET(dispatch_fd, &read_fds);

    /*
     * Block on AMF dispatch file descriptor for callbacks
     */
    do
    {
        if( select(dispatch_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
		    clprintf (CL_LOG_SEV_ERROR, "Error in select() -- unable to dispatch AMF events");
			perror("");
            break;
        }
        saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
    }while(!unblockNow);      

    return NULL;
}


static PyObject* initializeAmf(PyObject *self, PyObject *args)
{
    SaNameT             appName = {0};
    SaAmfCallbacksT     callbacks;
    SaVersionT          version;
    ClIocPortT          iocPort;
    ClRcT               rc = SA_AIS_OK;    

    PyEval_InitThreads();    
    PyThreadState* pts = PyThreadState_Get();
    thePythonInterpreter = pts->interp;

    /* Put the EO name received from the environment in the appropriate variable */
    //strncpy(clEoConfig.EOname,getenv("ASP_COMPNAME"),CL_EO_MAX_NAME_LEN-1);
    
    mypid = getpid();

    /*
     * Initialize and register with CPM. 'version' specifies the
     * version of AMF with which this application would like to
     * interface. 'callbacks' is used to register the callbacks this
     * component expects to receive.
     */

    version.releaseCode  = 'B';
    version.majorVersion = 01;
    version.minorVersion = 01;
    
    callbacks.saAmfHealthcheckCallback          = NULL;
    callbacks.saAmfComponentTerminateCallback   = clCompAppTerminate;
    callbacks.saAmfCSISetCallback               = clCompAppAMFCSISet;
    callbacks.saAmfCSIRemoveCallback            = clCompAppAMFCSIRemove;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
        
    /*
     * Initialize AMF client library.
     */

    if ( (rc = saAmfInitialize(&amfHandle, &callbacks, &version)) != SA_AIS_OK) 
        goto errorexit;

    clprintf (CL_LOG_SEV_INFO, "AmfInitialize successful");

    if (1)
    {
        
    SaSelectionObjectT dispatch_fd;
    fd_set read_fds;

    FD_ZERO(&read_fds);

    /*
     * Get the AMF dispatch FD for the callbacks
     */
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &dispatch_fd)) != SA_AIS_OK)
    {
        clprintf (CL_LOG_SEV_ERROR, "Error getting selection object -- unable to dispatch AMF events");
        return NULL;        
    }
    }
    
    /*
     * Do the application specific initialization here.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

#if 0
    if (1)
      {
      void (*extensions[])(void) = { init_asp, initasppycustom, 0 };
      clPyGlueInit("clusterMgrApp",extensions);
      }
#endif

    /*
     * ---END_APPLICATION_CODE---
     */

    /*
     * Now register the component with AMF. At this point it is
     * ready to provide service, i.e. take work assignments.
     */

    if ( (rc = saAmfComponentNameGet(amfHandle, &appName)) != SA_AIS_OK) 
        goto errorexit;
    clprintf (CL_LOG_SEV_INFO, "component name get successful");

    if ( (rc = saAmfComponentRegister(amfHandle, &appName, NULL)) != SA_AIS_OK) 
        goto errorexit;
    clprintf (CL_LOG_SEV_INFO, "component register successful");

    /*
     * Print out standard information for this component.
     */

    clEoMyEoIocPortGet(&iocPort);
    
    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. Initializing\n", appName.value, mypid);
    clprintf (CL_LOG_SEV_INFO, "   IOC Address             : 0x%x\n", clIocLocalAddressGet());
    clprintf (CL_LOG_SEV_INFO, "   IOC Port                : 0x%x\n", iocPort);

    /*
     * ---BEGIN_APPLICATION_CODE---
     */
    //clPyGlueStart();
    /*
     * ---END_APPLICATION_CODE---
     */
    rc = clOsalTaskCreateDetached("AmfDispatcher",CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, amfDispatcher,NULL);
    

errorexit:

    if (rc != CL_OK)
      {
        char str[256];
        snprintf(str,255,"Component [%s] : PID [%d]. Initialization error [0x%x]\n", appName.value, mypid, rc);
        clprintf (CL_LOG_SEV_ERROR, str);
        //return PyInt_FromLong(rc);
        PyObject* errData = Py_BuildValue("is",rc,str);
        PyErr_SetObject(PyExc_SystemError,errData);
        return NULL;     
      }
    Py_RETURN_NONE;
    }


static PyObject* finalizeAmf(PyObject *self, PyObject *args)
{    
    ClRcT rc;
    
    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
	{
        clprintf (CL_LOG_SEV_ERROR, "AMF finalization error[0x%X]", rc);
        PyObject* errData = Py_BuildValue("is",rc,"AMF finalization error");
        PyErr_SetObject(PyExc_SystemError,errData);
        return NULL;
	}

    clprintf (CL_LOG_SEV_INFO, "AMF Finalized");

    Py_RETURN_NONE;    
}


/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

static void clCompAppTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    SaAisErrorT rc = SA_AIS_OK;

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. Terminating\n",
              compName->value, mypid);

    /*
     * ---BEGIN_APPLICATION_CODE--- 
     */

    //clPyGlueTerminate();

    /*
     * ---END_APPLICATION_CODE---
     */
    
    /*
     * Unregister with AMF and respond to AMF saying whether the
     * termination was successful or not.
     */

    if ( (rc = saAmfComponentUnregister(amfHandle, compName, NULL)) != SA_AIS_OK)
        goto errorexit;

    saAmfResponse(amfHandle, invocation, SA_AIS_OK);

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. Terminated\n",
              compName->value, mypid);

    unblockNow = CL_TRUE;
    
    return;

errorexit:

    clprintf (CL_LOG_SEV_ERROR, "Component [%s] : PID [%d]. Termination error [0x%x]\n",
              compName->value, mypid, rc);

    return;
}

/*
 * clCompAppHealthCheck
 * --------------------
 * This function is invoked to perform a healthcheck on the application. The
 * health check logic is application specific.
 */

static ClRcT clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    /*
     * Add code for application specific health check below. The
     * defaults indicate EO is healthy and polling interval is
     * unaltered.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */
    
    ClRcT rc = CL_OK;

    /* If you want to attach to gdb */
    if (clDbgNoKillComponents) schFeedback->freq   = CL_EO_DONT_POLL;  
    else 
      {
        schFeedback->freq   = CL_EO_DEFAULT_POLL; 
        schFeedback->status = CL_CPM_EO_ALIVE;
        
        /* GAS TODO: get the return value out of this call */
        clRunPython("eoApp.HealthCheck()");
        return rc;
      }

    /*
     * ---END_APPLICATION_CODE---
     */

    return CL_OK;
}


/*
 * clCompAppAMFCSISet
 * ------------------
 * This function is invoked when a CSI assignment is made or the state
 * of a CSI is changed.
 */

static void clCompAppAMFCSISet(SaInvocationT       invocation,
                        const SaNameT       *compName,
                        SaAmfHAStateT       haState,
                        SaAmfCSIDescriptorT csiDescriptor)
{
    /*
     * ---BEGIN_APPLICATION_CODE--- 
     */
    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. CSISet called\n", compName->value, mypid);

    clPyGlueCsiSet(compName, haState,&csiDescriptor);

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. PyGlueCsiSet() returned OK\n", compName->value, mypid);

    /* if (haState != SA_AMF_HA_QUIESCING) */
    saAmfResponse(amfHandle, invocation, SA_AIS_OK);

    /* GAS TODO: put in a separate python-visible callback */
    if (haState == SA_AMF_HA_QUIESCING) saAmfCSIQuiescingComplete(amfHandle, invocation, SA_AIS_OK);

    /*
     * ---END_APPLICATION_CODE---
     */

    /*
     * Print information about the CSI Set
     */

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%d]. CSI Set Received\n", 
              compName->value, mypid);

    clCompAppAMFPrintCSI(csiDescriptor, haState);

}

/*
 * clCompAppAMFCSIRemove
 * ---------------------
 * This function is invoked when a CSI assignment is to be removed.
 */

static void clCompAppAMFCSIRemove(SaInvocationT  invocation,
                           const SaNameT  *compName,
                           const SaNameT  *csiName,
                           SaAmfCSIFlagsT csiFlags)
{
    clprintf (CL_LOG_SEV_INFO, "Node Addr [%d] Component [%s] : PID [%d]. CSI Remove Received\n", ASP_NODEADDR, compName->value, mypid);

    clprintf (CL_LOG_SEV_INFO, "   CSI                     : %s\n", csiName->value);
    clprintf (CL_LOG_SEV_INFO, "   CSI Flags               : 0x%d\n", csiFlags);

    /*
     * Add application specific logic for removing the work for this CSI.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */
    clPyGlueCsiRemove(compName, csiName, csiFlags);

    /*
     * ---END_APPLICATION_CODE---
     */

    saAmfResponse(amfHandle, invocation, SA_AIS_OK);

    return;
}

/******************************************************************************
 * Utility functions 
 *****************************************************************************/



static PyMethodDef AmfPyMethods[] = {
    {"initializeAmf",  initializeAmf, METH_VARARGS, "Call this function to register with the AMF and access the ASP cluster."},
    {"finalizeAmf",  finalizeAmf, METH_VARARGS, "Call this function to unregister."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initamfpy(void)
{
    (void) Py_InitModule("amfpy", AmfPyMethods);
}
