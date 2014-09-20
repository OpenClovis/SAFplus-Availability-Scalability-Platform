#include <saAmf.h>
#include <safplus.hxx>
#include <boost/thread/thread.hpp> 

//#define clprintf(sev,...) appLog(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __VA_ARGS__)
#define clprintf(sev,...) SAFplus::logMsgWrite(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __FILE__, __LINE__, __VA_ARGS__)

// This is the application name to use if the AMF did not start up this component
#define DEFAULT_APP_NAME "c0"

/* Access to the SAF AMF framework occurs through this handle */
SaAmfHandleT amfHandle;

/* This process's SAF name */
SaNameT      appName = {0};

/* set when SAFplus tells this process to quit */
bool quitting = false;
/* set when SAFplus assigns active work */
bool running = false;

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

int main(int argc, char *argv[])
{
    SaAisErrorT rc = SA_AIS_OK;
 
    SAFplus::logEchoToFd = 1;  // echo logs to stdout (fd 1) for debugging
    SAFplus::logSeverity = SAFplus::LOG_SEV_MAX;

    /* Connect to the SAF cluster */
    initializeAmf();

    /* Do the application specific initialization here. */
    
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();
    
    /* Do the application specific finalization here. */


    while (!quitting) boost::this_thread::sleep(boost::posix_time::milliseconds(250));
    /* Now finalize my connection with the SAF cluster */
    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
      logError("APP","FIN", "AMF finalization error[0x%X]", rc);
    else
      logInfo("APP","FIN", "AMF Finalized");   

    return 0;
}

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
            clprintf(SAFplus::LOG_SEV_INFO,"csa101: ACTIVE state requested; activating service");
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

            clprintf(SAFplus::LOG_SEV_INFO,"csa101: Standby state requested");
            running = 0;

            /* The AMF times the interval between the assignment and acceptance
               of the work (the time interval is configurable).
               So it is important to call this saAmfResponse function ASAP.
             */
            saAmfResponse(amfHandle, invocation, SA_AIS_OK);  
            break;
        }

        case SA_AMF_HA_QUIESCED:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active or quiescing HA state. The application 
             * must stop work associated with the CSI immediately.
             */
            clprintf(SAFplus::LOG_SEV_INFO, "csa101: Acknowledging new state quiesced");
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
              clprintf(SAFplus::LOG_SEV_INFO, "csa101: Signaling completion of QUIESCING");
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
            assert(0);
            break;
        }
    }

    return;
}

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

void initializeAmf(void)
  {
  SaAmfCallbacksT     callbacks;
  SaAisErrorT         rc = SA_AIS_OK;

  // TEMPORARY DEBUG to run outside of AMF
  if (1) // (!startedByAmf())
    {
    SAFplus::ASP_NODEADDR = 1;
    }

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
      saNameSet(&appName, DEFAULT_APP_NAME);
      }
    }

  if ( (rc = saAmfComponentRegister(amfHandle, &appName, NULL)) != SA_AIS_OK) 
    errorExit(rc);
#else

  // If spawned by the AMF, you can just pass NULL as the component name since the AMF has told this app its name (SAFplus::COMP_NAME variable)
  if ( (rc = saAmfComponentRegister(amfHandle, NULL, NULL)) != SA_AIS_OK)
    errorExit(rc);
#endif

  logInfo("APP","MAIN","Component [%s] registered successfully", appName.value);
  }


void dispatchLoop(void)
{
  SaAisErrorT         rc = SA_AIS_OK;
  SaSelectionObjectT amf_dispatch_fd;
  int maxFd;
  fd_set read_fds;

  /* This boilerplate code includes an example of how to simultaneously
     dispatch for 2 services (in this case AMF and CKPT).  But since checkpoint
     is not initialized or used, it is commented out */
  /* SaSelectionObjectT ckpt_dispatch_fd; */

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
      timeout.tv_sec = 2; timeout.tv_usec = 0;

      FD_ZERO(&read_fds);
      FD_SET(amf_dispatch_fd, &read_fds);
      /* FD_SET(ckpt_dispatch_fd, &read_fds); */
      if( select(maxFd + 1, &read_fds, NULL, NULL, &timeout) < 0)
        {
          char errorStr[80];
          int err = errno;
          if (EINTR == err) continue;

          errorStr[0] = 0; /* just in case strerror does not fill it in */
          strerror_r(err, errorStr, 79);
          clprintf (SAFplus::LOG_SEV_ERROR, "Error [%d] during dispatch loop select() call: [%s]",err,errorStr);
          break;
        }
      if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
      /* if (FD_ISSET(ckpt_dispatch_fd,&read_fds)) saCkptDispatch(ckptLibraryHandle, SA_DISPATCH_ALL); */
 
      if (running) clprintf(SAFplus::LOG_SEV_INFO,"csa101: Unthreaded Hello World!"); // show_progress());
      else clprintf(SAFplus::LOG_SEV_INFO,"csa101: idle");
    }while(!quitting);
}


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
      clprintf (SAFplus::LOG_SEV_INFO, "Name : [%s]",
        csiDescriptor.csiAttr.
        attr[i].attrName);
      clprintf (SAFplus::LOG_SEV_INFO, "Value : [%s]",
        csiDescriptor.csiAttr.
        attr[i].attrValue);
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
