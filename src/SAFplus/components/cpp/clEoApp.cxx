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

#include "clEoApp.hxx"

namespace SAFplus
{

App::App()
{}

App::~App()
{}


void App::Start()
{
  // Not implementing this is ok.
}

  void App::init(char releaseCode,int majorVersion, int minorVersion)
  {
    pid = getpid();  // Get the pid for the process and store in member variable.

   /*
     * Initialize and register with CPM. 'version' specifies the
     * version of AMF with which this application would like to
     * interface. 'callbacks' is used to register the callbacks this
     * component expects to receive.
     */

    version.releaseCode  = releaseCode;
    version.majorVersion = majorVersion;
    version.minorVersion = minorVersion;
    
    callbacks.saAmfHealthcheckCallback          = NULL;
    callbacks.saAmfComponentTerminateCallback   = clCompAppTerminate;
    callbacks.saAmfCSISetCallback               = clCompAppAMFCSISet;
    callbacks.saAmfCSIRemoveCallback            = clCompAppAMFCSIRemove;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
        
    // Initialize AMF client library.
    if ( (rc = saAmfInitialize(&amfHandle, &callbacks, &version)) != SA_AIS_OK) 
      throw new clAppException("Cannot connect to SAF Availability Management Framework (AMF)", rc);
    
    // Get the AMF dispatch FD for the callbacks
    SaSelectionObjectT dispatch_fd;
    FD_ZERO(&read_fds);
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &dispatch_fd)) != SA_AIS_OK)
        goto errorexit;   
    FD_SET(dispatch_fd, &read_fds);

    // Now register the component with AMF. At this point it is ready to provide service, i.e. take work assignments.

    if ( (rc = saAmfComponentNameGet(amfHandle, &appName)) != SA_AIS_OK) 
        goto errorexit;
    if ( (rc = saAmfComponentRegister(amfHandle, &appName, NULL)) != SA_AIS_OK) 
        goto errorexit;

    clEoMyEoIocPortGet(&msgPort);
    // Log that I am alive    
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Initializing\n", appName.length, appName.value, mypid);
    clprintf (CL_LOG_SEV_INFO, "   IOC Address             : 0x%x\n", clIocLocalAddressGet());
    clprintf (CL_LOG_SEV_INFO, "   IOC Port                : 0x%x\n", msgPort);

  }

  void App::dispatchForever(void)
  {
    // Block on AMF dispatch file descriptor for callbacks
    do
    {
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
    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
	{
        clprintf (CL_LOG_SEV_ERROR, "AMF finalization error[0x%X]", rc);
	}
    clprintf (CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Left SAF AMF framework\n", appName.length, appName.value, mypid);
  }


void App::Stop()
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
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


void App::ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}


void App::RemoveWorkAssignment (const ClNameT* workName, int all)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void App::AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor)
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


void AppThreadForWork::RemoveWorkAssignment (const ClNameT* workName, int all)
{
  WorkStatus* ws;
  clMutexLocker lock(exclusion);

  if (!(ws=GetWorkStatus(*workName)))
    {
      clDbgCodeError(CompErr(CL_ERR_NOT_EXIST), ("Work assignment (CSI) %s does not exist", workName->value));
      return;
    }

  if (ws->state != CL_AMS_HA_STATE_QUIESCED)
    AbortWorkAssignment(ws);

  clOsalTaskJoin(ws->thread); 
  ws->Clear();
}



void AppThreadForWork::ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      ws = GetNewHandle();
      ws->descriptor = workDescriptor;  // GAS TODO: verify that the workdescriptor sticks around.  Otherwise I have to do a deep copy
    }

  clDbgCheck (ws->state != CL_AMS_HA_STATE_ACTIVE, return, ("Double activation of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  
  ws->state = CL_AMS_HA_STATE_ACTIVE;
  clOsalCondBroadcast(&ws->change);  
  if (!ws->thread) StartWorkThread(ws);

}


void AppThreadForWork::StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      ws = GetNewHandle();
      ws->descriptor = workDescriptor;  // GAS TODO: verify that the workdescriptor sticks around.  Otherwise I have to do a deep copy
    }

  clDbgCheck (ws->state != CL_AMS_HA_STATE_STANDBY, return, ("Double activation of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  
  ws->state = CL_AMS_HA_STATE_STANDBY;
  clOsalCondBroadcast(&ws->change);  

  if ((!ws->thread)&&standbyIsThreaded) StartWorkThread(ws);

}

void AppThreadForWork::QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clMutexLocker lock(exclusion);

  WorkStatus* ws;
  if (!(ws=GetWorkStatus(workDescriptor.csiName)))
    {
      clDbgCodeError(CompErr(CL_ERR_NOT_EXIST), ("Work assignment (CSI) %s does not exist", workDescriptor.csiName.value));
      return;
    }

  clDbgCheck (ws->state != CL_AMS_HA_STATE_QUIESCING, return, ("Double quiesce of work assignment %s, ignoring", workDescriptor.csiName.value ) );
  ws->state = CL_AMS_HA_STATE_QUIESCING;
  clOsalCondBroadcast(&ws->change);  
}

void AppThreadForWork::AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor)
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
  //clDbgCheck (ws->state != CL_AMS_HA_STATE_QUIESCED, return, ("Double abort of work assignment %s, ignoring", ws->descriptor.csiName.value ));
  ws->state = CL_AMS_HA_STATE_QUIESCED;
  clOsalCondBroadcast(&ws->change);  
}


void* WorkAssignmentThreadEntry(void* w)
{
  WorkStatus* ws = (WorkStatus*) w;
  bool standbyIsThreaded = ws->app->standbyIsThreaded;

  while ((ws->state == CL_AMS_HA_STATE_ACTIVE) || (standbyIsThreaded && (ws->state == CL_AMS_HA_STATE_STANDBY)))
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


WorkStatus* AppThreadForWork::GetWorkStatus(const ClNameT& workName)
{
  clMutexLocker lock(exclusion);

  for(unsigned int i=0; i<MaxWork;i++)
    {
      if ((work[i].handle)&&(strncmp(work[i].descriptor.csiName.value, workName.value, workName.length)==0))
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

}
