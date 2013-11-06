/*
 * Copyright (C) 2002-2008 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <string.h>

#include "clApp.hxx"

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
                                  __VA_ARGS__)
#define CompErr(x) x

namespace SAFplus
{
void clCompAppAMFPrintCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState);

App *theApp=NULL;

// SAF callbacks
void clCompAppTerminate(SaInvocationT       invocation, const SaNameT       *compName)
{
    ClRcT rc = CL_OK;

    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Terminating\n", compName->length, compName->value, theApp->pid);
    theApp->Stop();
    
    /*
     * Unregister with AMF and send back a response
     */

    if ( (rc = saAmfComponentUnregister(theApp->amfHandle, compName, NULL)) != SA_AIS_OK)
        goto errorexit;

    saAmfResponse(theApp->amfHandle, invocation, SA_AIS_OK);
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Terminated\n", compName->length, compName->value, theApp->pid);

errorexit:
    clprintf (CL_LOG_SEV_ERROR, "Component [%.*s] : PID [%d]. Termination error [0x%x]\n",compName->length, compName->value, theApp->pid, rc);
}


void clCompAppAMFCSISet(SaInvocationT       invocation,const SaNameT       *compName,SaAmfHAStateT       haState,SaAmfCSIDescriptorT csiDescriptor)
{
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. CSI Set Received\n", compName->length, compName->value, theApp->pid);
    clCompAppAMFPrintCSI(csiDescriptor, haState);

    /* If component hang detection is off, then reply with an OK first, so that this call won't time out causing the controller to kill me. */
    if (clDbgNoKillComponents)
      {
        if (haState != SA_AMF_HA_QUIESCING)
          saAmfResponse(theApp->amfHandle, invocation, SA_AIS_OK);
      }

    /* DO NOT SET BREAKPOINTS ABOVE THIS LINE (IN THIS FUNCTION)

       (the controller will think that your process is dead)

       To set a breakpoint in this routine:

       1. First set "clDbgNoKillComponents" to 1 (best to do it in the code at
          components/utils/client/clDbg.c, but you can also do it from gdb)
       2. Set your breakpoint below this comment
       3. Continue
    */

    /* Take appropriate action based on state */
    switch ( haState )
    {
        case SA_AMF_HA_ACTIVE:
        {
            /* AMF has requested application to take the active HA state for the CSI. */
            theApp->ActivateWorkAssignment(csiDescriptor);
            break;
        }

        case SA_AMF_HA_STANDBY:
        {
            /* AMF has requested application to take the standby HA state for this CSI. */
            theApp->StandbyWorkAssignment(csiDescriptor);
            break;
        }

        case SA_AMF_HA_QUIESCED:
        {
            /* AMF has requested application to quiesce the CSI currently
             * assigned the active or quiescing HA state. The application 
             * must stop work associated with the CSI immediately.
             */

            theApp->AbortWorkAssignment(csiDescriptor);
            break;
        }

        case SA_AMF_HA_QUIESCING:
        {
            /* AMF has requested application to quiesce the CSI currently
             * assigned the active HA state. The application must stop work
             * associated with the CSI gracefully and not accept any new
             * workloads while the work is being terminated.
             */

            theApp->QuiesceWorkAssignment  (csiDescriptor);

            clCpmCSIQuiescingComplete(theApp->amfHandle, invocation, CL_OK);
            break;
        }

        default:
        {
            /*
             * Should never happen. Ignore.
             */
        }
    }


    /* If I'm am killing components, then reply with an OK after execution, to ensure that the handling did not hang */
    if (!clDbgNoKillComponents)
      {
        if (haState != SA_AMF_HA_QUIESCING)
          saAmfResponse(theApp->amfHandle, invocation, SA_AIS_OK);
      }
}

/*
 * clCompAppAMFCSIRemove
 * ---------------------
 * This function is invoked when a CSI assignment is to be removed.
 */


void clCompAppAMFCSIRemove(SaInvocationT       invocation, const SaNameT       *compName, const SaNameT       *csiName, SaAmfCSIFlagsT      csiFlags)
{
    /*
     * Print information about the CSI Remove
     */

    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. CSI Remove Received\n", compName->length, compName->value, theApp->pid);
    clprintf (CL_LOG_SEV_INFO, "   CSI                     : %.*s\n", csiName->length, csiName->value);
    clprintf (CL_LOG_SEV_INFO, "   CSI Flags               : 0x%d\n", csiFlags);

    theApp->RemoveWorkAssignment(csiName, (csiFlags == CL_AMS_CSI_FLAG_TARGET_ALL) ? true : false);
    saAmfResponse(theApp->amfHandle, invocation, SA_AIS_OK);
}


App::App()
{
  if ((theApp)&&(theApp!=this))
    throw new clAppException("Only one App class can be created per process", 0);  
  unblockNow = false;
  theApp = this;
}

App::~App()
{

  theApp = NULL;
}

void App::Start()
{
  // Not implementing this is ok.
}

  void App::init(char releaseCode,int majorVersion, int minorVersion)
  {
    SaAisErrorT rc;
    pid = getpid();  // Get the pid for the process and store in member variable.

   /*
     * Initialize and register with CPM. 'version' specifies the
     * version of AMF with which this application would like to
     * interface. 'callbacks' is used to register the callbacks this
     * component expects to receive.
     */
    SaVersionT          version;
    version.releaseCode  = releaseCode;
    version.majorVersion = majorVersion;
    version.minorVersion = minorVersion;
    
    SaAmfCallbacksT     callbacks;
    callbacks.saAmfHealthcheckCallback          = NULL;
    callbacks.saAmfComponentTerminateCallback   = clCompAppTerminate;
    callbacks.saAmfCSISetCallback               = clCompAppAMFCSISet;
    callbacks.saAmfCSIRemoveCallback            = clCompAppAMFCSIRemove;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
        
    // Initialize AMF client library.
    if ( (rc = saAmfInitialize(&amfHandle, &callbacks, &version)) != SA_AIS_OK) 
      throw new clAppException("Cannot connect to SAF Availability Management Framework (AMF)", rc);
    
    // Get the AMF dispatch FD for the callbacks
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &dispatch_fd)) != SA_AIS_OK)
      throw new clAppException("Cannot get SAF Availability Management Framework (AMF) change flag", rc);

    // Now register the component with AMF. At this point it is ready to provide service, i.e. take work assignments.

    if ( (rc = saAmfComponentNameGet(amfHandle, &appName)) != SA_AIS_OK) 
      throw new clAppException("Cannot get component name", rc);
    if ( (rc = saAmfComponentRegister(amfHandle, &appName, NULL)) != SA_AIS_OK) 
      throw new clAppException("Cannot register component", rc);

    clEoMyEoIocPortGet(&msgPort);
    // Log that I am alive    
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Initializing\n", appName.length, appName.value, theApp->pid);
    clprintf (CL_LOG_SEV_INFO, "   IOC Address             : 0x%x\n", clIocLocalAddressGet());
    clprintf (CL_LOG_SEV_INFO, "   IOC Port                : 0x%x\n", msgPort);

  }

  void App::dispatchForever(void)
  {
    fd_set read_fds;

    // Block on AMF dispatch file descriptor for callbacks
    do
    {
        FD_ZERO(&read_fds);
        FD_SET(dispatch_fd, &read_fds);
        if( select(dispatch_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }
		    clprintf (CL_LOG_SEV_ERROR, "Error in select()");
			perror("");
            break;
        }
        saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
    }while(!unblockNow);      
  }

void App::dispatch()
  {
    assert(0);  // TBD
  }

void App::finalize(void)
  {
    SaAisErrorT rc;
    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
	{
        clprintf (CL_LOG_SEV_ERROR, "AMF finalization error[0x%X]", rc);
	}
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Left SAF AMF framework\n", appName.length, appName.value, pid);
  }


void App::Stop()
{
  unblockNow = true;
}

void App::Suspend()
{
}

void App::Resume()
{
}

ClRcT App::HealthCheck()
{
  return CL_OK;
}


void App::ActivateWorkAssignment (const SaAmfCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}


void App::RemoveWorkAssignment (const SaNameT* workName, int all)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::StandbyWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::QuiesceWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::AbortWorkAssignment    (const SaAmfCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void AppThreadForWork::DoWork(WorkStatus* ws)
{
  clDbgNotImplemented(("EO %s function must be overridden", __PRETTY_FUNCTION__));
}

AppThreadForWork::~AppThreadForWork()
{
  clDbgNotImplemented(("destructor"));
}


void AppThreadForWork::RemoveWorkAssignment (const SaNameT* workName, int all)
{
  WorkStatus* ws;
  clMutexLocker lock(exclusion);

  if (!(ws=GetWorkStatus(*workName)))
    {
      clDbgCodeError(CompErr(CL_ERR_NOT_EXIST), ("Work assignment (CSI) %s does not exist", workName->value));
      return;
    }

  if (ws->state != SA_AMF_HA_QUIESCED)
    AbortWorkAssignment(ws);

  clOsalTaskJoin(ws->thread); 
  ws->Clear();
}



void AppThreadForWork::ActivateWorkAssignment (const SaAmfCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      ws = GetNewHandle();
      ws->descriptor = workDescriptor;  // GAS TODO: verify that the workdescriptor sticks around.  Otherwise I have to do a deep copy
    }

  clDbgCheck (ws->state != SA_AMF_HA_ACTIVE, return, ("Double activation of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  
  ws->state = SA_AMF_HA_ACTIVE;
  clOsalCondBroadcast(&ws->change);  
  if (!ws->thread) StartWorkThread(ws);

}


void AppThreadForWork::StandbyWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      ws = GetNewHandle();
      ws->descriptor = workDescriptor;  // GAS TODO: verify that the workdescriptor sticks around.  Otherwise I have to do a deep copy
    }

  clDbgCheck (ws->state != SA_AMF_HA_STANDBY, return, ("Double activation of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  
  ws->state = SA_AMF_HA_STANDBY;
  clOsalCondBroadcast(&ws->change);  

  if ((!ws->thread)&&standbyIsThreaded) StartWorkThread(ws);

}

void AppThreadForWork::QuiesceWorkAssignment  (const SaAmfCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      clDbgCodeError(CompErr(CL_ERR_NOT_EXIST), ("Work assignment (CSI) %s does not exist", workDescriptor.csiName.value));
      return;
    }

  clDbgCheck (ws->state != SA_AMF_HA_QUIESCING, return, ("Double quiesce of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  ws->state = SA_AMF_HA_QUIESCING;
  clOsalCondBroadcast(&ws->change);  
}

void AppThreadForWork::AbortWorkAssignment    (const SaAmfCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      clDbgCodeError(CompErr(CL_ERR_NOT_EXIST), ("Work assignment (CSI) %s does not exist", workDescriptor.csiName.value));
      return;
    }

  AbortWorkAssignment(ws);
}


void AppThreadForWork::AbortWorkAssignment    (WorkStatus* ws)
{
  // Not abnormal to have a double, because the RemoveWorkAssignment() function calls this one...
  //clDbgCheck (ws->state != SA_AMF_HA_QUIESCED, return, ("Double abort of work assignment %s, ignoring", ws->descriptor.csiName.value ));
  ws->state = SA_AMF_HA_QUIESCED;
  clOsalCondBroadcast(&ws->change);  
}


void* WorkAssignmentThreadEntry(void* w)
{
  WorkStatus* ws = (WorkStatus*) w;
  bool standbyIsThreaded = ws->app->standbyIsThreaded;

  while ((ws->state == SA_AMF_HA_ACTIVE) || (standbyIsThreaded && (ws->state == SA_AMF_HA_STANDBY)))
    {
      ws->app->DoWork(ws);
    }
  return 0;
}

WorkStatus* AppThreadForWork::GetWorkStatus(unsigned int handle)
{
  clMutexLocker lock(exclusion);

  clDbgCheck((handle>=HandleBase)&&(handle<HandleBase+MaxWork), return NULL, ("Invalid work handle %d", handle));
  WorkStatus* ws = &work[handle-HandleBase];

  clDbgCheck (ws->handle == handle, return NULL, ("Work handle %d already freed", handle));

  return &work[handle-HandleBase];
}


WorkStatus* AppThreadForWork::GetWorkStatus(const SaNameT& workName)
{
  clMutexLocker lock(exclusion);

  for(unsigned int i=0; i<MaxWork;i++)
    {
      if ((work[i].handle)&&(strncmp((char*)work[i].descriptor.csiName.value, (const char*)workName.value, workName.length)==0))
        {
          return &work[i];
        }
    }
  return NULL;
}

WorkStatus* AppThreadForWork::GetNewHandle()
{
  clMutexLocker lock(exclusion);

  for(unsigned int i=0; i<MaxWork;i++)
    {
      if (!work[i].handle)
        {
          work[i].Init(i+HandleBase, this);
          return &work[i];
        }
    }
  clDbgResourceLimitExceeded(clDbgWorkHandleResource, 0, ("Out of work (CSI) handles"));
  return NULL;
}

void AppThreadForWork::StartWorkThread(WorkStatus* ws)
{
  snprintf(ws->taskName,80,"work assignment %s", ws->descriptor.csiName.value);
  ClRcT rc = clOsalTaskCreateAttached(ws->taskName, threadSched, threadPriority, 0, WorkAssignmentThreadEntry, ws, &ws->thread);
  if (rc != CL_OK)
    {
      clDbgCodeError(rc, ("Cannot start a task, error 0x%x", rc));
    }
  CL_ASSERT(ws->thread != 0);

}

void AppThreadForWork::Stop()
{
  clMutexLocker lock(exclusion);
  /* Take all the threads down hard.  Probably "stop" should be nicer */
  for(unsigned int i=0; i<MaxWork;i++)
    {
      if (work[i].handle)
        {
          clOsalTaskDelete(work[i].thread);
        }
    }

}


#define STRING_HA_STATE(S)                                            \
(   ((S) == SA_AMF_HA_ACTIVE)             ? "Active" :                \
    ((S) == SA_AMF_HA_STANDBY)            ? "Standby" :               \
    ((S) == SA_AMF_HA_QUIESCED)           ? "Quiesced" :              \
    ((S) == SA_AMF_HA_QUIESCING)          ? "Quiescing" :             \
                                            "Unknown" )

#define STRING_CSI_FLAGS(S)                                           \
(   ((S) & SA_AMF_CSI_ADD_ONE)            ? "Add One" :               \
    ((S) & SA_AMF_CSI_TARGET_ONE)         ? "Target One" :            \
    ((S) & SA_AMF_CSI_TARGET_ALL)         ? "Target All" :            \
                                            "Unknown" )

void clCompAppAMFPrintCSI(SaAmfCSIDescriptorT csiDescriptor, SaAmfHAStateT haState)
{
  clprintf (CL_LOG_SEV_INFO, "CSI Flags : [%s]", STRING_CSI_FLAGS(csiDescriptor.csiFlags));

    if (SA_AMF_CSI_TARGET_ALL != csiDescriptor.csiFlags)
    {
        clprintf (CL_LOG_SEV_INFO, "CSI Name : [%s]",  csiDescriptor.csiName.value);
    }

    if (SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags)
    {
        ClUint32T i = 0;
        
        clprintf (CL_LOG_SEV_INFO, "Name value pairs :");
        for (i = 0; i < csiDescriptor.csiAttr.number; i++)
        {
            clprintf (CL_LOG_SEV_INFO, "Name : [%s]", csiDescriptor.csiAttr.attr[i].attrName);
            clprintf (CL_LOG_SEV_INFO, "Value : [%s]",csiDescriptor.csiAttr.attr[i].attrValue);
        }
    }
    
    clprintf (CL_LOG_SEV_INFO, "HA state : [%s]",STRING_HA_STATE(haState));

    if (SA_AMF_HA_ACTIVE == haState)
    {
        clprintf (CL_LOG_SEV_INFO, "Active Descriptor :");
        clprintf (CL_LOG_SEV_INFO,"Transition Descriptor : [%d]",csiDescriptor.csiStateDescriptor.activeDescriptor.transitionDescriptor);
        clprintf (CL_LOG_SEV_INFO,"Active Component : [%s]",csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.value);
    }
    else if (SA_AMF_HA_STANDBY == haState)
    {
        clprintf (CL_LOG_SEV_INFO, "Standby Descriptor :");
        clprintf (CL_LOG_SEV_INFO,"Standby Rank : [%d]",csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank);
        clprintf (CL_LOG_SEV_INFO, "Active Component : [%s]",csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.value);
    }
}


}
