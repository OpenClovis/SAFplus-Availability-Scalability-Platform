#include <clIocApi.h>

namespace SAFplus
{
  extern pid_t pid;  // This process's ID 
  extern char* logCompName;  // Override this component name for display in the logs.  If this variable is not changed the name will be the SAF component name.
  extern bool  logCodeLocationEnable;
  extern ClIocPortT  iocPort;  // The default communications port number for this component

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

};
