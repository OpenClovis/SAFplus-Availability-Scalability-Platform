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

#ifndef CL_SAF_APP_H
#define CL_SAF_APP_H

#include <clCommon.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clDbg.h>
#include <saAmf.h>

#include <clCpmApi.h>
//#include <clEoApi.h>
//#include <clOsalApi.h>

namespace SAFplus
{

  // All application level errors are raised through this exception.
  class clAppException
  {
  public:
    clAppException(const char *s,int err) { errcode = err; }
    int errcode;
  };

class clMutexLocker
{
 protected:
  ClOsalMutexT& mutex;

 public:
  clMutexLocker(ClOsalMutexT& mut): mutex(mut)
    {
      clOsalMutexLock(&mutex);
    }

  ~clMutexLocker()
    {
      clOsalMutexUnlock(&mutex);
    }
  
};

class App
{
 public:
  SaAmfHandleT                  amfHandle;
  SaNameT                       appName;
  unsigned int                  pid;
  ClIocPortT                    msgPort;
  ClIocNodeAddressT             msgAddr;
  SaSelectionObjectT            dispatch_fd;
  fd_set                        read_fds;
  SaVersionT                    version;
  SaAmfCallbacksT               callbacks;
  ClBoolT                       unblockNow;

  App();
  virtual ~App();

  // Call this function to initialize the EO once your setup is completed.
  void init(char releaseCode='B',int majorVersion=1, int minorVersion=1);


  // Handle AMF callbacks until shutdown
  void dispatchForever();

  // Handle all queued AMF callbacks
  void dispatch();

  // All done (called automatically by destructor)
  ClUint32T finalize();

  /// APPLICATION CALLBACKS

  /** Start yourself (and keep this thread until termination)
   */
  virtual void Start();

  /** Stop yourself, all threads you have created, and return from the "Start" function.
   */
  virtual void Stop();

  /** Pause yourself
   */
  virtual void Suspend();

  /** Ok, you can keep going...
   */
  virtual void Resume();

  /** Override (optional) and return CL_OK if your app is ok, otherwise return any error
   */
  virtual ClRcT HealthCheck();

  
  /// WORK ASSIGNMENT CALLBACKS

  /** Override and activate the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor);

  /** Override and set to standby the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor);

  /** Override and cleanly stop the passed work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor);

  /** Override and halt the work (SAForum CSI) assignment
   * @param workDescriptor A structure detailing the work item.
   */
  virtual void AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor);


  /** Override and remove the passed work (SAForum CSI) assignment from your component.
   *  GAS TO DO: Will CSI be stopped before this is called?
   * @param workName The name of the work item (previously passed in an Activate call)
   * @param all      If true, remove ALL matching work items; otherwise, just remove one.
   */
  virtual void RemoveWorkAssignment (const ClNameT* workName, int all);

};

class AppThreadForWork;

class WorkStatus
{
 public:
  ClCharT               taskName[80];
  unsigned int          handle;
  ClAmsCSIDescriptorT   descriptor;
  ClAmsHAStateT         state;
  ClOsalMutexT          exclusion;
  ClOsalCondT           change;
  ClOsalTaskIdT         thread;

  AppThreadForWork*   app;

  void Init(unsigned int handl, AppThreadForWork* ap)
    {
      taskName[0] = 0;
      handle = handl;
      app    = ap;
      state  = CL_AMS_HA_STATE_NONE;
      clOsalRecursiveMutexInit(&exclusion);
      clOsalCondInit(&change);
      thread = 0;
    }

  void Clear()
    {
      taskName[0] = 0;
      handle      = 0;
      state       = CL_AMS_HA_STATE_NONE;
      clOsalMutexDelete(&exclusion);
      thread = 0;
    }


};



class AppThreadForWork: public App
{
  public:
  bool                  standbyIsThreaded;  /**< Set to true if "Standby" work assignments should still have a running thread. */
  ClOsalThreadPriorityT threadPriority;
  ClOsalSchedulePolicyT threadSched;
  ClOsalMutexT          exclusion;

  enum
  {
    MaxWork     = 256,
    HandleBase  = 10000,
  };

  WorkStatus work[MaxWork];

  AppThreadForWork()
    {
      threadSched    = CL_OSAL_SCHED_OTHER;
      threadPriority = CL_OSAL_THREAD_PRI_NOT_APPLICABLE;
      clOsalRecursiveMutexInit(&exclusion);
    }

  ~AppThreadForWork();

  virtual void ActivateWorkAssignment (const ClAmsCSIDescriptorT& workDescriptor);
  virtual void StandbyWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor);
  virtual void QuiesceWorkAssignment  (const ClAmsCSIDescriptorT& workDescriptor);
  virtual void AbortWorkAssignment    (const ClAmsCSIDescriptorT& workDescriptor);
  virtual void RemoveWorkAssignment   (const ClNameT* workName, int all);

  virtual void Stop();

  WorkStatus* GetWorkStatus     (unsigned int handle);
  WorkStatus* GetWorkStatus     (const ClNameT& workName);

  /** Override to implement your application
   *   After each work item (size to be decided by the implementation) is completed, you may return from
   *   the DoWork function.  The framework will call DoWork again if the work assignment is still active,
   *   or will not call if it has been changed.  If there are more work assignments then possible threads,
   *   the framework will multiplex several assignments onto the same thread.
   *  @param ws A description of the work (SA Forum CSI) and your role in doing it.
   */
  virtual void DoWork(WorkStatus* ws);


  // PRIVATE

  WorkStatus* GetNewHandle      ();
  void StartWorkThread        (WorkStatus* ws);
  void AbortWorkAssignment    (WorkStatus* ws);

};
};
#endif
