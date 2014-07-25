#pragma once
#include <clIocApi.h>
#include <clLogApi.hxx>
#include <clHandleApi.hxx>

namespace SAFplus
  {
  extern pid_t pid;  // This process's ID 
  extern const char* logCompName;  // Override this component name for display in the logs.  If this variable is not changed the name will be the SAF component name.
  extern bool  logCodeLocationEnable;
  /** Set this to a file descriptor to echo all logs that pass the severity limit to it this fd on the client side.  For example, use 1 to send to stdout.  -1 to turn off (default) */
  extern int logEchoToFd;

  extern SAFplus::LogSeverity logSeverity;
  extern ClIocPortT  iocPort;  //? The default communications port number for this component

/** Name of the node.  Loaded from the same-named environment variable.  */
  extern ClCharT ASP_NODENAME[CL_MAX_NAME_LENGTH];
/** Name of the component.  Loaded from the same-named environment variable.  */
  extern ClCharT ASP_COMPNAME[CL_MAX_NAME_LENGTH];
/** Address of the node. This is the same as the slot number in slot-based system.  Loaded from the same-named environment variable.  On a slot-based system, it is the application programmer's job to access the hardware and set this environment variable properly (and remove it from asp.conf).  Otherwise a unique number should be provided in the asp.conf file. */
  extern ClWordT ASP_NODEADDR;

/** Working dir where programs are run. Loaded from the same-named environment variable.  */
  extern ClCharT ASP_RUNDIR[CL_MAX_NAME_LENGTH];
/** Dir where logs are stored. Loaded from the same-named environment variable.  */
  extern ClCharT ASP_LOGDIR[CL_MAX_NAME_LENGTH];
/** Dir where ASP binaries are located. Loaded from the same-named environment variable.  */
  extern ClCharT ASP_BINDIR[CL_MAX_NAME_LENGTH];
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].  Deprecated. */
  extern ClCharT CL_APP_BINDIR[CL_MAX_NAME_LENGTH];
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0]. */
  extern ClCharT ASP_APP_BINDIR[CL_MAX_NAME_LENGTH];

/** Dir where xml config are located. Loaded from the same-named environment variable.  */
  extern ClCharT ASP_CONFIG[CL_MAX_NAME_LENGTH];
/** Dir where persistent db files are to be stored. Loaded from the same-named environment variable.  */
  extern ClCharT ASP_DBDIR[CL_MAX_NAME_LENGTH];


  
/** Variable to check if the current node is a system controller node.  Loaded from the same-named environment variable.  */
  extern ClBoolT SYSTEM_CONTROLLER; 
/** Variable to check if the current node is a SC capable node.  Loaded from the same-named environment variable.  */
  extern ClBoolT ASP_SC_PROMOTE;

/** The IOC port assigned to this component.  */
  extern ClIocPortT gEOIocPort;
  
/** AMF did not start this component. ASP_WITHOUT_CPM environment variable.  */
  extern bool clWithoutAmf;

/* SAFplus initialization */

   /* Library sets */
  enum class LibSet
  {
    RPC=1, /* defferred */
    CKPT=2,
    IOC=4,
    UTILS=8,
    OSAL=0x10,
    LOG=0x20,
    GRP=0x40,
    HEAP=0x80,
    BUFFER=0x100,
    TIMER=0x200
  };
  /* LibSet operators overload */
  inline constexpr uint32_t operator*(LibSet ls)
  {
    return static_cast<uint32_t>(ls);
  }

  inline constexpr LibSet operator|(LibSet lls, LibSet rls)
  {
    return static_cast<LibSet>((*lls) | (*rls));
  }

  inline bool operator&(LibSet lls, LibSet rls)
  {
    return ((*lls) & (*rls)) != 0;
  }
  /* Library dependencies */
  enum class LibDep
  {
    LOG = LibSet::LOG | LibSet::OSAL | LibSet::UTILS,
    OSAL = LibSet::OSAL | LibSet::LOG | LibSet::UTILS,
    UTILS = LibSet::UTILS | LibSet::OSAL,
    IOC = LibSet::OSAL | LibSet::HEAP | LibSet::TIMER | LibSet::BUFFER,
    #if 0
    CKPT = LibSet::CKPT | LibSet::IOC | LibSet::UTILS | LibSet::GRP,
    GRP = LibSet::GRP | LibSet::CKPT | LibSet::IOC,
    #endif
    // CKPT and GRP itself initialized when its instance is created    
    CKPT = LibSet::IOC | LibSet::UTILS,
    GRP = LibSet::IOC,
    HEAP = LibSet::HEAP,
    BUFFER = LibSet::BUFFER,
    TIMER = LibSet::TIMER
  };
  /* LibDep operators overload */
  inline uint32_t operator*(LibDep ld)
  { 
    return static_cast<uint32_t>(ld);
  }

  inline LibDep operator|(LibDep lld, LibDep rld)
  {
    return static_cast<LibDep>((*lld) | (*rld));
  }

  inline bool operator&(LibDep lld, LibDep rld)
  {
    return ((*lld) & (*rld)) != 0;
  }

  inline bool operator&(LibDep ld, LibSet ls)
  {
    return ((*ld) & (*ls)) != 0;
  }  

  extern void utilsInitialize() __attribute__((weak));
  extern Logger* logInitialize(void) __attribute__((weak));
  extern ClRcT clOsalInitialize(const ClPtrT pConfig) __attribute__((weak));
  extern ClRcT clIocLibInitialize(ClPtrT pConfig) __attribute__((weak));
  extern ClRcT clHeapInit(void) __attribute__((weak));
  extern ClRcT clBufferInitialize(const ClBufferPoolConfigT *pConfig) __attribute__((weak));
  extern ClRcT clTimerInitialize (ClPtrT pConfig) __attribute__((weak));

  inline void safplusInitialize(LibDep svc)
  {
    if(svc&LibSet::LOG)
      if(logInitialize) logInitialize();
    if(svc&LibSet::UTILS)
      if(utilsInitialize) utilsInitialize();
    if(svc&LibSet::OSAL)
      if(clOsalInitialize) assert(clOsalInitialize(NULL) == CL_OK);
    if(svc&LibSet::HEAP)
      if(clHeapInit) assert(clHeapInit() == CL_OK);
    if(svc&LibSet::BUFFER)
      if(clBufferInitialize) assert(clBufferInitialize(NULL) == CL_OK);
    if(svc&LibSet::TIMER)
      if(clTimerInitialize) assert(clTimerInitialize(NULL) == CL_OK);
    if(svc&LibSet::IOC)
      if(clIocLibInitialize) assert(clIocLibInitialize(NULL) == CL_OK);    
  }

  };
