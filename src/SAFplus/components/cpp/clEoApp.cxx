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

namespace clAsp
{

EoApp::EoApp()
{}

EoApp::~EoApp()
{}


void EoApp::Start()
{
  // Not implementing this is ok.
}


void EoApp::Stop()
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void EoApp::Suspend()
{
}

void EoApp::Resume()
{
}

ClRcT EoApp::HealthCheck()
{
  return CL_OK;
}


void EoApp::ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}


void EoApp::RemoveWorkAssignment (const ClNameT* workName, int all)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void EoApp::StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void EoApp::QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void EoApp::AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor)
{
  clDbgNotImplemented(("EO %s function not overridden", __PRETTY_FUNCTION__));
}

void EoAppThreadForWork::DoWork(WorkStatus* ws)
{
  clDbgNotImplemented(("EO %s function must be overridden", __PRETTY_FUNCTION__));
}

EoAppThreadForWork::~EoAppThreadForWork()
{
  clDbgNotImplemented(("destructor"));
}


void EoAppThreadForWork::RemoveWorkAssignment (const ClNameT* workName, int all)
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



void EoAppThreadForWork::ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor)
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


void EoAppThreadForWork::StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
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

void EoAppThreadForWork::QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor)
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

void EoAppThreadForWork::AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor)
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


void EoAppThreadForWork::AbortWorkAssignment    (WorkStatus* ws)
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

WorkStatus* EoAppThreadForWork::GetWorkStatus(unsigned int handle)
{
  clMutexLocker lock(exclusion);

  clDbgCheck((handle>=HandleBase)&&(handle<HandleBase+MaxWork), return NULL, ("Invalid work handle %d", handle));
  WorkStatus* ws = &work[handle-HandleBase];

  clDbgCheck (ws->handle == handle, return NULL, ("Work handle %d already freed", handle));

  return &work[handle-HandleBase];
}


WorkStatus* EoAppThreadForWork::GetWorkStatus(const ClNameT& workName)
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

WorkStatus* EoAppThreadForWork::GetNewHandle()
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

void EoAppThreadForWork::StartWorkThread(WorkStatus* ws)
{
  snprintf(ws->taskName,80,"work assignment %s", ws->descriptor.csiName.value);
  ClRcT rc = clOsalTaskCreateAttached(ws->taskName, threadSched, threadPriority, 0, WorkAssignmentThreadEntry, ws, &ws->thread);
  if (rc != CL_OK)
    {
      clDbgCodeError(rc, ("Cannot start a task, error 0x%x", rc));
    }
  CL_ASSERT(ws->thread != 0);

}

void EoAppThreadForWork::Stop()
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
