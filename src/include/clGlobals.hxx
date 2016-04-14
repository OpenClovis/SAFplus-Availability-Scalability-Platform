#pragma once
#include <clLogApi.hxx>
#include <clHandleApi.hxx>
#include <clBufferApi6.h>
#include <clCustomization.hxx>

#ifndef CL_OK
#define CL_OK 0x00
#endif

namespace SAFplus
  {
  extern pid_t pid;  //? This process's ID
  extern const char* logCompName;  //? Override this component name for display in the logs.  If this variable is not changed the name will be the SAF component name.
  extern bool  logCodeLocationEnable; //? Set this to a file descriptor to echo all logs that pass the severity limit to it this fd on the client side.  For example, use 1 to send to stdout.  -1 to turn off (default)
  extern int logEchoToFd;  //? Echo all logs to a particular file descriptor (typically used to echo to stdout (or fd 1)
  extern SAFplus::LogSeverity logSeverity;  //? The log severity cutoff.  Logs less severe than this will not be issued

  extern uint_t iocPort;  //? The default communications port number for this component

  extern uint64_t beat; //? Objects can save their last change time by setting a local variable to this "beat" and then incrementing beat.  Other entities can then tell if the object was changed since the last time the other entity checked.

    //? Name of the node.  Loaded from the same-named environment variable.
  extern char ASP_NODENAME[CL_MAX_NAME_LENGTH];
    //? Name of the component.  Loaded from the same-named environment variable.
  extern char ASP_COMPNAME[CL_MAX_NAME_LENGTH];
    //? Address of the node. This is the same as the slot number in slot-based system.  Loaded from the same-named environment variable.  On a slot-based system, it is the application programmer's job to access the hardware and set this environment variable properly (and remove it from asp.conf).  Otherwise a unique number should be provided in the asp.conf file.
  extern int ASP_NODEADDR;

    //? Working dir where programs are run. Loaded from the same-named environment variable.
  extern char ASP_RUNDIR[CL_MAX_NAME_LENGTH];
    //? Dir where logs are stored by default.  Loaded from the same-named environment variable.
    extern char ASP_LOGDIR[CL_MAX_NAME_LENGTH];
    //? Dir where ASP binaries are located. Loaded from the same-named environment variable.
  extern char ASP_BINDIR[CL_MAX_NAME_LENGTH];
    //? Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].  Deprecated, use ASP_APP_BINDIR
  extern char CL_APP_BINDIR[CL_MAX_NAME_LENGTH];
    //? Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].
  extern char ASP_APP_BINDIR[CL_MAX_NAME_LENGTH];

    //? Dir where xml config are located. Loaded from the same-named environment variable.
  extern char ASP_CONFIG[CL_MAX_NAME_LENGTH];
    //? Dir where persistent db files are to be stored. Loaded from the same-named environment variable.
  extern char ASP_DBDIR[CL_MAX_NAME_LENGTH];


  
    //? Variable to check if the current node is a system controller node.  Loaded from the same-named environment variable.
  extern bool SYSTEM_CONTROLLER; 
    //? Variable to check if the current node is a SC capable node.  Loaded from the same-named environment variable.
  extern bool ASP_SC_PROMOTE;
  
    // AMF did not start this component. ASP_WITHOUT_CPM environment variable.  DEPRECATED (component can discover this itself in main())
    // extern bool clWithoutAmf;

/* SAFplus initialization */

//? <class> This class defines initialization parameters for the SAFplus services.  It can be passed to safplusInitialize to customize service creation.
class SafplusInitializationConfiguration
  {
  public:
  uint_t iocPort;      //? What messaging port should this component's SAFplus service layer (safplusMsgServer object) listen to?
  uint_t msgQueueLen;  //? How many messages can be queued up in the SAFplus service layer?
  uint_t msgThreads;   //? What's the maximum number of message processing threads that can be created?

  SafplusInitializationConfiguration()
    {
    iocPort = 0;  // means allocate a random unique-to-the-node one
    msgQueueLen = SAFplus::MsgAppQueueLen;
    msgThreads  = SAFplus::MsgAppMaxThreads;
    }
}; //? </class>

class LibSet
  {
  public:
   /* Library sets */
  enum
  {
    RPC=1, /* deferred */
    CKPT=2,
    IOC=4,
    UTILS=8,
    OSAL=0x10,
    LOG=0x20,
    GRP=0x40,
    HEAP=0x80,
    BUFFER=0x100,
    TIMER=0x200,
    DBAL=0x400,
    MSG=0x800,
    OBJMSG=0x1000,
    NAME=0x2000,
    FAULT=0x4000,
    MGT_ACCESS=0x8000,
  };
  };


    //? <class> This class is a namespace wrapper around bitfields describing components and their dependencies
    class LibDep
    {
    public:
      /* Library dependencies */
      enum Bits
        {
          LOG = LibSet::LOG | LibSet::OSAL | LibSet::UTILS,  //? Log service and dependencies
          OSAL = LibSet::OSAL | LibSet::LOG | LibSet::UTILS, //? Operating System Adaption Layer and dependencies
          UTILS = LibSet::UTILS | LibSet::OSAL,              //? Utility library and dependencies (initializing this loads global vars from the environment)
          IOC = LibSet::IOC | LibSet::LOG |  LibSet::UTILS | LibSet::OSAL | LibSet::HEAP | LibSet::TIMER | LibSet::BUFFER,
          FAULT = LibSet::FAULT | LibDep::LOG,               //? Fault determination library and dependencies
          MSG = LibSet::MSG | LibDep::FAULT | LibDep::IOC,   //? Messaging library and dependencies
          OBJMSG = LibSet::OBJMSG | LibDep::MSG,             //? Object Messaging library and dependencies
          GRP = LibSet::GRP | LibDep::OBJMSG | LibDep::MSG,  //? Group membership library and dependencies
          CKPT = LibSet::CKPT | LibSet::DBAL | LibDep::GRP | LibDep::MSG | LibDep::UTILS, //? Checkpoint library and dependencies
          NAME = LibSet::NAME | LibDep::CKPT, //? Name service and dependencies
          HEAP = LibSet::HEAP,
          BUFFER = LibSet::BUFFER,
          TIMER = LibSet::TIMER, //? Timer library and dependencies
          DBAL = LibSet::DBAL | LibSet::OSAL | LibSet::HEAP | LibSet::TIMER | LibSet::BUFFER, //? Database adaption layer and dependencies
          MGT_ACCESS = LibSet::MGT_ACCESS | LibDep::MSG | LibDep::CKPT, //? Management client access library and dependencies

        };
    }; //? </class>

  extern void utilsInitialize() __attribute__((weak));
  extern Logger* logInitialize(void) __attribute__((weak));
  extern void objectMessagerInitialize() __attribute__((weak));
  extern void nameInitialize() __attribute__((weak));
  extern void msgServerInitialize(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads)  __attribute__((weak));
  extern void msgServerFinalize() __attribute__((weak));
  extern void clMsgInitialize(void) __attribute__((weak)); 
  extern void mgtAccessInitialize(void) __attribute__((weak));

#ifdef __cplusplus
extern "C" {
#endif 
  extern ClRcT clOsalInitialize(const ClPtrT pConfig) __attribute__((weak));  
  extern ClRcT clHeapInit(void) __attribute__((weak));
  extern ClRcT clBufferInitialize(const ClBufferPoolConfigT *pConfig) __attribute__((weak));
  extern ClRcT clTimerInitialize (ClPtrT pConfig) __attribute__((weak));
  extern ClRcT clDbalLibInitialize (void) __attribute__((weak));
#ifdef __cplusplus
}
#endif

  extern void groupInitialize(void) __attribute__((weak));
  extern void groupFinalize(void) __attribute__((weak));
  extern void faultInitialize(void) __attribute__((weak));

    //? Initialize specific SAFplus components and dependencies.  This API is automatically called by <ref>saAmfInitialize()</ref> so it is unnecessary to call it for SA-Aware components.
    //<arg name="svc">A bit field describing the services to initialize.  Use constants defined in the LibDep class.</arg>
    //<arg name="cfg">Optional extended configuration parameters.  See <ref>SafplusInitializationConfiguration</ref> for information about each parameter.</arg>
  inline void safplusInitialize(unsigned int svc,const SafplusInitializationConfiguration& cfg=*((SafplusInitializationConfiguration*)NULL))
  {
    SafplusInitializationConfiguration defaults;
    const SafplusInitializationConfiguration* sic;
    if (&cfg) sic = &cfg;
    else sic = &defaults;
    ClRcT rc;
    if ((svc&LibSet::LOG)&& logInitialize) logInitialize();
    if ((svc&LibSet::UTILS)&& utilsInitialize) utilsInitialize();
    if ((svc&LibSet::OSAL)&& clOsalInitialize)
      {
      rc = clOsalInitialize(NULL);
      assert(rc == CL_OK);
      }
    if((svc&LibSet::HEAP)&&clHeapInit)
      {
      rc = clHeapInit();
      assert(rc == CL_OK);
      }
    if((svc&LibSet::BUFFER)&&clBufferInitialize) 
      {
      rc = clBufferInitialize(NULL);
      assert(rc == CL_OK);
      }
    if((svc&LibSet::TIMER)&&clTimerInitialize)
      {
      rc = clTimerInitialize(NULL);
      assert(rc == CL_OK);
      }

    if((svc&LibSet::IOC)&&clMsgInitialize) 
      { 
      clMsgInitialize();
      }

    if((svc&LibSet::DBAL)&&clDbalLibInitialize)
      {
      rc = clDbalLibInitialize();
      assert(rc == CL_OK);
      }

    if((svc&LibSet::GRP)&&SAFplus::groupInitialize) 
      { 
      SAFplus::groupInitialize();
      }

    if((svc&LibSet::FAULT)&&faultInitialize) 
      { 
      faultInitialize();
      }

    if ((svc&LibSet::MSG)&& msgServerInitialize)
      {
      msgServerInitialize(sic->iocPort,sic->msgQueueLen,sic->msgThreads);
      }

    if ((svc&LibSet::OBJMSG)&&SAFplus::objectMessagerInitialize)
      {
      objectMessagerInitialize();
      }

    if((svc&LibSet::NAME)&&SAFplus::nameInitialize) 
      { 
      SAFplus::nameInitialize();
      }

    if ((svc&LibSet::MGT_ACCESS)&&SAFplus::mgtAccessInitialize)
      {
        SAFplus::mgtAccessInitialize();
      }
  }

    //? Finalize (stop) SAFplus services.  Call this function if you want a clean shutdown
    inline void safplusFinalize(void)
    {
      SAFplus::groupFinalize();
      msgServerFinalize();
    }
  };
