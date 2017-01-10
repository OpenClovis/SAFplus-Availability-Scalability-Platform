/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */

/**
 * This file implements CPM debug cli registration.
 */

/*
 * ASP header files 
 */
#include <clEoApi.h>
#include <clDebugApi.h>

/*
 * CPM internal header files 
 */
#include <clCpmCliCommands.h>
#include <clCpmInternal.h>

#include <clAmsDebugCli.h>
#include <clAmsMgmtDebugCli.h>
/* #define AMS_TEST_CLI_COMMANDS */

ClHandleT  gCpmDebugReg = CL_HANDLE_INVALID_VALUE;

#define CPM_DEBUG_CLI_COMMON_FUNC_LIST                                  \
    {                                                                   \
        cliEOListShow,                                                  \
        "cpmEOShow",                                                    \
        "Displays list of available EOs on the local node"              \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmComponentListAll,                                          \
        "compList",                                                     \
        "Displays all the components and EOs on the local Node"         \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmClusterListAll,                                            \
        "clusterList",                                                  \
        "Displays cluster wide available Nodes"                         \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmCompGet,                                                   \
        "compAddressGet",                                               \
        "Get Address of a specified component"                          \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmCompGet,                                                   \
        "compPIDGet",                                                   \
        "Get compPID of a specified component"                          \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmCompGet,                                                   \
        "compIdGet",                                                    \
        "Get compId of a specied component"                             \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmCompGet,                                                   \
        "compTraceGet",                                                 \
        "Get stack trace of the specified component, available only if software exception" \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmComponentReport,                                           \
        "compReport",                                                   \
        "Report an error"                                               \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmComponentReport,                                           \
        "compClear",                                                    \
        "Clear an error"                                                \
    },                                                                  \
                                                                        \
    {                                                                   \
        clCpmNodeNameGet,                                               \
        "nodename",                                                     \
        "Utility command for returning CPM node name"                   \
    },                                                                  \
    {                                                                   \
        clCpmHeartbeat,                                                 \
        "heartbeat",                                                    \
        "Utility command for enabling/disabling the heartbeat "         \
        "for the whole system"                                          \
    },                                                                  \
    {                                                                   \
        clCpmRestart,                                                   \
        "nodeRestart",                                                  \
        "Restart a node. Behavior is affected by the node reset env. variables in asp. conf file" \
    },                                                                  \
    {                                                                   \
        clCpmMiddlewareRestartCommand,                                  \
        "middlewareRestart",                                            \
        "Restart the middleware. Behavior is unaffected by the node reset env. variables in asp conf. file" \
    },                                                                  \
    {                                                                   \
        clCpmUptimeGet,                                                 \
        "uptime",                                                       \
        "Middleware uptime",                                            \
    } 

#if 0
{                                                                       \
        clCpmLogFileRotate,                                             \
        "logrotate",                                                    \
        "Utility command for forcing nodename log file rotations",      \
    },
    
#endif

static ClDebugFuncEntryT cpmSCDebugFuncList[] =
{
	
    {
        clAmsDebugCliEntityCommand,
        "start",
        "Starts an entity running and assigns work, regardless of its current state"
    },

    {
        clAmsDebugCliEntityCommand,
        "stop",
        "Stops an entity, regardless of its current state"
    },

    {
        clAmsDebugCliEntityCommand,
        "idle",
        "Puts an entity in the idle state (running but no assigned work), regardless of its current state"
    },

    {
        clAmsDebugCliEntityCommand,
        "repair",
        "Repairs a faulty entity"
    },

    {
        clAmsDebugCliEntityCommand,
        "repair all",
        "Looks through every faulty entity and repairs it"
    },

    {
        clAmsDebugCliAdminAPI,
        "amsLockAssignment",
        "Admin API for lock assignment of an entity"
    },
    
    {
        clAmsDebugCliAdminAPI,
        "amsLockInstantiation",
        "Admin API for lock instantiation of an entity"
    },
    
    {
        clAmsDebugCliAdminAPI,
        "amsUnlock",
        "Admin API for unlocking an entity"
    },
    
    {
        clAmsDebugCliAdminAPI,
        "amsShutdown",
        "Admin API for shutting down an entity"
    },
    
    {
        clAmsDebugCliAdminAPI,
        "amsRestart",
        "Admin API for restarting an entity"
    },
    
    {
        clAmsDebugCliAdminAPI,
        "amsRepaired",
        "Admin API for marking an entity as repaired"
    },

    {
        clAmsDebugCliSISwap,
        "amsSISwap",
        "Admin API for swapping ha states of SIs assigned service units"
    },

    {
        clAmsDebugCliSGAdjust,
        "amsSGAdjust",
        "Admin API for adjusting the SG to use the most preferred assignments based on the SU rank", 
    },

    {
        clAmsDebugCliFaultReport,
        "amsFaultReport",
        "Admin API for reporting a fault on a component/node"
    },

    {
        clAmsDebugCliNodeJoin,
        "amsNodeJoin",
        "Admin API for reporting arrival of a node"
    },

    {
        clAmsDebugCliForceLock,
        "amsForceLock",
        "Admin API for forcefully locking an SU in a fault restart loop",
    },

    {
        clAmsDebugCliForceLockInstantiation,
        "amsForceLockInstantiation",
        "Admin API for forcefully lock instantiating an SU",
    },

#ifdef AMS_TEST_CLI_COMMANDS
    {
        clAmsDebugCliPGTrackAdd,
        "amsPGTrackAdd",
        "Admin API for adding a client in the PG track list of a CSI"
    },
    
    {
        clAmsDebugCliPGTrackStop,
        "amsPGTrackStop",
        "Admin API for deleting a client from the PG track list of a CSI"
    },

#endif

    {
        clAmsDebugCliEntityDebugEnable,
        "amsDebugEnable",
        "API for enabling debug for a particular entity"
    },
    
    {
        clAmsDebugCliEntityDebugDisable,
        "amsDebugDisable",
        "API for disabling debug for a particular entity"
    },
    
    {
        clAmsDebugCliEnableLogToConsole,
        "amsDebugEnableLogToConsole",
        "API for enabling debug messages to be logged on the console"
    },

    {
        clAmsDebugCliDisableLogToConsole,
        "amsDebugDisableLogToConsole",
        "API for disabling debug messages to be logged on the console"
    },

    {
        clAmsDebugCliEntityDebugGet,
        "amsDebugGet",
        "API for getting entity's debug information"
    },
    
    {
        clAmsDebugCliPrintAmsDB,
        "amsDbPrint",
        "Utility API for printing the contents of ams DB"
    },

    {
        clAmsDebugCliPrintAmsDBXML,
        "amsDbXMLPrint",
        "Utility API for printing the contents of ams DB in XML format"
    },
    
    {
        clAmsDebugCliEntityPrint,
        "amsEntityPrint",
        "Utility API for printing the contents of an ams entity"
    },
    
    {
        clAmsDebugCliAssignSU,
        "assignSUtoSI",
        "Assigns specific service units to a service instance (for the custom redundancy model)"
    },
    
    {
        clAmsDebugCliXMLizeDB,
        "amsDbXMLize",
        "Utility API for XMLizing the contents of ams DB"
    },
    
    {
        clAmsDebugCliXMLizeInvocation,
        "amsDbInvocationXMLize",
        "Utility API for XMLizing the contents of ams invocation list"
    },

    {
        clAmsDebugCliEntityAlphaFactor,
        "amsAlpha",
        "Utility API for setting or fetching the alpha factor for the given SG for active SUs"
    },

    {
        clAmsDebugCliEntityBetaFactor,
        "amsBeta",
        "Utility API for setting or fetching the beta factor for the given SG for standby SUs"
    },

    {
        clAmsDebugCliEntityTrigger,
        "amsTrigger",
        "Utility API for setting entity AMS triggers on hitting certain thresholds (like CPU/MEM thresholds)",
    },
#ifdef AMS_TEST_CLI_COMMANDS
    {
        clAmsDebugCliDeXMLizeDB,
        "amsDbDeXMLize",
        "Utility API for DeXMLizing the contents of ams DB"
    },
    
    {
        clAmsDebugCliDeXMLizeInvocation,
        "amsDbInvocationDeXMLize",
        "Utility API for DeXMLizing the contents of ams invocation list"
    },
    
    {
        clAmsDebugCliSCStateChange,
        "amsStateChange",
        "Utility API testing ams state changes ( Active to Standby and vice versa"
    },
    
    {
        clAmsDebugCliEventTest,
        "amsEventTest",
        "Utility command for testing ams event functionality"
    },
#endif

    {
        clAmsDebugCliMgmtApi,
        "amsMgmt",
        "Debug CLI front end for AMS managment functions"
    },
    
    {
        clCpmShutDown,
        "nodeShutdown",
        "shutdown down a node"
    },

    {
        clCpmNodeErrorReport,
        "nodeErrorReport",
        "shutdown down a node"
    },

    {
        clCpmNodeErrorClear,
        "nodeErrorClear",
        "shutdown down a node"
    },
    {
        clCpmCliNodeDelete,
        "nodeTableDelete",
        "Delete node in AMF node table (for fault testing)"
    },
     
    CPM_DEBUG_CLI_COMMON_FUNC_LIST,
    
    {
        NULL,
        "",
        ""
    }
};

ClRcT cpmDebugRegister(void)
{
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("CPM");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPrompSet(): rc[0x %x]", rc));
        return rc;
    }

    return clDebugRegisterConstBuf(cpmSCDebugFuncList,
                           sizeof(cpmSCDebugFuncList) /
                           sizeof(cpmSCDebugFuncList[0]),
                           &gCpmDebugReg);

    //return clDebugRegister(cpmSCDebugFuncList,
     //                      sizeof(cpmSCDebugFuncList) /
      //                     sizeof(cpmSCDebugFuncList[0]), 
       //                    &gCpmDebugReg);
}

ClRcT cpmDebugDeregister()
{
    return clDebugDeregister(gCpmDebugReg);
}
