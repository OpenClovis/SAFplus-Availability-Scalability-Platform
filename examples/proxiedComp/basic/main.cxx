
#include <clCommon.hxx>
#include <saAmf.h>
#include <clNameApi.hxx>
#include <safplus.hxx>
#include <clAmfNotification.hxx>
#include <boost/thread/thread.hpp> 

#include <signal.h>
#include <sys/wait.h>
#include <clGlobals.hxx>////proxy-proxied support feature

//#define clprintf(sev,...) appLog(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __VA_ARGS__)
#define clprintf(sev,...) SAFplus::logMsgWrite(SAFplus::APP_LOG, sev, 0, "APP", "MAIN", __FILE__, __LINE__, __VA_ARGS__)

// This is the application name to use if the AMF did not start up this component
#define DEFAULT_APP_NAME "c0"

/* Access to the SAF AMF framework occurs through this handle */
SaAmfHandleT amfHandle;

/* This process's SAF name */
SaNameT      appName = {0};

char userAppProcessName[] = {"hello_world"};
unsigned long userAppPid;

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
//proxy-proxied support feature
void safProxiedCompInstantiate(SaInvocationT invocation, const SaNameT *compName);
void safProxiedCompCleanup();
//End proxy-proxied support feature
                

/* Utility functions */
void initializeAmf(void);
void dispatchLoop(void);
void printCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState);
int  errorExit(SaAisErrorT rc);

void sigchild_handler(int signum);
void launchUserApp(const char* pathToApp);
void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  clprintf (SAFplus::LOG_SEV_INFO, "***********************************************************");
  clprintf (SAFplus::LOG_SEV_INFO, "app Event channel[%s] data [%s]",channelName.c_str(), data.c_str());
  clprintf (SAFplus::LOG_SEV_INFO, "***********************************************************");
  //g_isCallBack = true;
}
/* This simple helper function just prints an error and quits */
int errorExit(SaAisErrorT rc)
{
    logCritical("APP","INI", "Component [%.*s] : PID [%d]. Initialization error [0x%x]\n", appName.length, appName.value, SAFplus::pid, rc);
    exit(-1);
    return -1;
}

//pid_t proxied_pid;
namespace SAFplus
  {
  extern Handle           myHandle;  // This handle resolves to THIS process.
  extern SafplusInitializationConfiguration   serviceConfig;
  
};

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
	logInfo("APP","INI", "AMF Initialize");
    initializeAmf(); // Connect to the AMF cluster

    // You can override the logging parameters, but it must be done AFTER
    // calling initializeAmf() because that function loads the logSeverity
    //  from the environment variable CL_LOG_SEVERITY.
    SAFplus::logEchoToFd = 1;  // echo logs to stdout (fd 1) for debugging
    //SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;

    /* Do any application specific initialization here. */
#if 0
   clAmfClientNotificationInitialize(myHandle,&eventCallback);
#endif
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
       logInfo("APP","INI", "AMF dispatchLoop");
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
             clprintf(SAFplus::LOG_SEV_INFO,"safAssignWork Basic HA app: ACTIVE state call saAmfResponse");
            saAmfResponse(amfHandle, invocation, SA_AIS_OK);
            clprintf(SAFplus::LOG_SEV_INFO,"safAssignWork Basic HA app: ACTIVE state AFTER call saAmfResponse");
#if 1
            //char* aspDir = getenv("ASP_DIR");
            char pathToApp[256];
            if (SAFplus::ASP_BINDIR[0]==0) // Was not set
			{
			  char spbinary[CL_MAX_NAME_LENGTH];
			  int len = readlink("/proc/self/exe", spbinary, CL_MAX_NAME_LENGTH);  // will only work on linux
			  spbinary[len] = 0;
			  boost::filesystem::path canonicalPath = boost::filesystem::canonical(spbinary).parent_path();
			  assert(strlen(canonicalPath.c_str())<CL_MAX_NAME_LENGTH);
			  strncpy(SAFplus::ASP_BINDIR, canonicalPath.c_str(),CL_MAX_NAME_LENGTH);
			}
            if (SAFplus::ASP_BINDIR[0] != 0)
            {
                //signal(SIGCHLD, catch_child);
                clprintf(SAFplus::LOG_SEV_INFO,"safAssignWork SAFplus::ASP_BINDIR[0] != 0");
                snprintf(pathToApp, 255, "%s/%s", SAFplus::ASP_BINDIR, userAppProcessName);
                launchUserApp(pathToApp);
            }
            else
            {
				clprintf(SAFplus::LOG_SEV_INFO,"safAssignWork SAFplus::ASP_BINDIR[0] == 0");
			}
#endif

            
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
            assert(0);
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

/*
 * safProxiedCompInstantiate 
 */
void safProxiedCompInstantiate(SaInvocationT invocation, const SaNameT *proxiedCompName)
{
	SaAisErrorT         rc = SA_AIS_OK;
	if ( (rc = saAmfComponentNameGet(amfHandle, &appName)) != SA_AIS_OK)
    {
    if (rc == SA_AIS_ERR_UNAVAILABLE)  // This component was not run by the AMF.  You must tell us what the component name is.
      {
      logInfo("APP","MAIN","Application NOT started by AMF");
      SAFplus::saNameSet(&appName, DEFAULT_APP_NAME);
      }
    }

  if ( (rc = saAmfComponentRegister(amfHandle, proxiedCompName, &appName)) != SA_AIS_OK) 
    errorExit(rc);
  saAmfResponse(amfHandle, invocation, SA_AIS_OK);
}

void safProxiedCompCleanup(SaInvocationT invocation, const SaNameT *proxiedCompName)
{
	logInfo("APP","MAIN","safProxiedCompCleanup : proxied compName [%s]", proxiedCompName->value);
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
#define PROXIED_SUPPORT
#ifdef PROXIED_SUPPORT 
  callbacks.saAmfProxiedComponentInstantiateCallback = safProxiedCompInstantiate;
  callbacks.saAmfProxiedComponentCleanupCallback = safProxiedCompCleanup;
#endif
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
		clprintf(SAFplus::LOG_SEV_INFO,"before saAmfDispatch");
        saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
        clprintf(SAFplus::LOG_SEV_INFO,"after saAmfDispatch");
        }
      else 
        {
        clprintf(SAFplus::LOG_SEV_INFO,"Select returned but nothing to dispatch");
        }
      /* if (FD_ISSET(ckpt_dispatch_fd,&read_fds)) saCkptDispatch(ckptLibraryHandle, SA_DISPATCH_ALL); */
 
      if (running==1) clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Active.  Hello World!123"); // show_progress());
      else if (running==2) clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: Standby."); // show_progress());
      else 
        {
        clprintf(SAFplus::LOG_SEV_INFO,"Basic HA app: idle");
        SAFplus::name.set(SAFplus::ASP_COMPNAME,SAFplus::myHandle,SAFplus::NameRegistrar::MODE_NO_CHANGE);

        }
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
  
  
  void launchUserApp(const char* pathToApp)
{
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigchild_handler;
        sigaction(SIGCHLD, &sa, NULL);
        pid_t pid = fork();
        if (pid == 0) // child process
        {
                clprintf (SAFplus::LOG_SEV_INFO, "This is child process, launching user app[%s]\n", pathToApp);
                //prctl(PR_SET_PDEATHSIG, SIGKILL);
                
                execl(pathToApp, pathToApp, (char*)0);
                //printf("from child: errno [%d]; errmsg [%s]\n", errno, strerror(errno));
                clprintf (SAFplus::LOG_SEV_INFO, "Fail to launch user app. ErrNo=%d", errno);
        }
        else if (pid > 0)
        {
                clprintf (SAFplus::LOG_SEV_INFO, "This is parent process, do nothing\n");
                userAppPid = pid;
                //SAFplus::proxied_pid = pid;
                //proxied_pid = pid;
               // clprintf (SAFplus::LOG_SEV_INFO, "Child pid [%lu] SAFplus::proxied_pid [%lu]\n", userAppPid,proxied_pid);
               clprintf (SAFplus::LOG_SEV_INFO, "Child pid [%lu] \n", userAppPid);
        }
        else
        {
                clprintf (SAFplus::LOG_SEV_ERROR, "Fork fail");
        }
}
void sigchild_handler(int signum)
{
        pid_t p;
        int status;
        while((p = waitpid(-1, &status, WNOHANG)) != -1)
        {
                if (p == userAppPid)
                {
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WIFEXITED(status)=%d", WIFEXITED(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WIFSIGNALED(status)=%d", WIFSIGNALED(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WTERMSIG(status)=%d", WTERMSIG(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WCOREDUMP(status)=%d", WCOREDUMP(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WIFSTOPPED(status)=%d", WIFSTOPPED(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WSTOPSIG(status)=%d", WSTOPSIG(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WIFCONTINUED(status)=%d", WIFCONTINUED(status));
                        clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): WEXITSTATUS(status)=%d", WEXITSTATUS(status));

                        if (signum == SIGCHLD)
                        {
                                clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): received SIGCHLD");
                                clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): user app terminated. The main app is AUTOMATICALLY terminated, too");
                        }
                        else
                        {
                                clprintf(SAFplus::LOG_SEV_INFO, "sigchild_handler(): signum [%d]", signum);
                        }
                        break;
                }
        }
}

