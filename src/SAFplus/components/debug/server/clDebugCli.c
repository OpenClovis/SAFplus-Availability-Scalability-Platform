/*
 * Copyright (C) 2002-2013 OpenClovis Solutions Inc.  All Rights Reserved.
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
/****************************************************************************
 * ModuleName  : debug
 * File        : clDebugCli.c
 ****************************************************************************/

/*****************************************************************************
 * Description :
 * This file contains debugServer implementation routines 
 ****************************************************************************/
#include <saAmf.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#ifndef VXWORKS_BUILD
#include <termios.h>
#endif
#include <unistd.h>
#include <clCommon.h>
#include <clLogApi.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clVersionApi.h>
#include <clCpmApi.h>
#include <clEoApi.h>
#include <clIocApi.h>
#include <clXdrApi.h>
#include <clBufferApi.h>
#include <clIocApiExt.h>
#include <clCpmIpi.h>
#include <clHandleApi.h>
#include <clDebug.h>
#include <clCpmExtApi.h>
#include "clDebugRmd.h"
#include <clLogUtilApi.h>
static  ClRcT  clDbgCliErrNo = CL_OK;

#define CL_DBG_DEFAULT_COMMAND_TIMEOUT   50000   /* 50 secs */
/* DEFINES */
static ClVersionT clVersionSupported[]={
    {'B',0x01 , 0x01}
};
/*Version Database*/
static ClVersionDatabaseT versionDatabase={
    sizeof(clVersionSupported)/sizeof(ClVersionT),
    clVersionSupported
}; 
typedef struct ClDebugInfoT
{
    ClHandleDatabaseHandleT databaseHdl;
    ClBoolT                 isResponse;
    ClOsalCondIdT           condVar;
    ClOsalMutexIdT          mutexVar;
}ClDebugInfoT;

ClDebugInfoT gDbgInfo;
ClCharT      *gStrDb[] = {
    "\r\nerror in making rmd call",                     /* 0 */
    "\r\nexecution of the command timed out",           /* 1 */
    "\r\nexecution of the command has been terminated", /* 2 */
    "\r\nerror in execution of the command",            /* 3 */
    "\r\ncould not execute the command"                 /* 4 */
};
#define CL_DEBUG_SERVER_LIB     "debug_server"

#define	COMMAND_TOTAL	         100
/* Maximun length of a command */
#define	CLI_CMD_SZ	         (MAX_ARGS*MAX_ARG_BUF_LEN)
/* Command Buffer size */
#define	CLI_CMD_LEN	         CLI_CMD_SZ

#ifndef VXWORKS_BUILD

#define CL_DEBUG_INPUT_SETTING  (~INPCK & ~IGNPAR & ~PARMRK &           \
                                 ~IGNPAR & ~ISTRIP & ~IGNBRK & ~BRKINT & ~IGNCR & \
                                 ~INLCR & ~ICRNL & ~IXON & ~IXOFF & ~IXANY & ~IMAXBEL)
#ifdef SOLARIS_BUILD
  #define CL_DEBUG_LOCAL_SETTING  (~ECHO)
#else
  #define CL_DEBUG_LOCAL_SETTING  (~ECHO & ~ICANON)
#endif

#endif

#define GET_ARRAY_SIZE(x)       sizeof(x)/sizeof(x[0])

#define DEBUG_CALL_RMD_SYNC(_nodeAddr, _commPort, _fcnId, _inMsg,       \
                            _outMsg, _rc)                               \
    do                                                                  \
    {                                                                   \
        ClRmdOptionsT    _rmdOptions;                                   \
        ClIocAddressT _iocAddr;                                         \
        _iocAddr.iocPhyAddress.nodeAddress = (_nodeAddr);               \
        _iocAddr.iocPhyAddress.portId = (_commPort);                    \
        _rmdOptions.timeout = CL_DBG_DEFAULT_COMMAND_TIMEOUT;           \
        _rmdOptions.retries = 0;                                        \
        _rmdOptions.priority = 0;                                       \
        if (0 == (_outMsg))                                             \
        {                                                               \
            (_rc) = clBufferCreate(&(_outMsg));                  \
        }                                                               \
        if (CL_OK == (_rc))                                             \
        {                                                               \
            (_rc) = clRmdWithMsg( _iocAddr, (ClUint32T)(_fcnId),        \
                                  (_inMsg), (_outMsg),                  \
                                  (CL_RMD_CALL_NEED_REPLY |             \
                                   CL_RMD_CALL_NON_PERSISTENT),         \
                                  &_rmdOptions, NULL);                  \
        }                                                               \
    }                                                                   \
    while (0);

/*Getting the details of all component*/
extern ClRcT clCpmComponentListDebugAll( ClIocNodeAddressT iocAddress,
                                         ClCharT           **retStr);
typedef enum ClCmds {
    SLEEP = 0,
    TIMEOUTSET,
    TIMEOUTGET,
    SETC,
    INTRO,
    HELP,
    LOGLEVELSET,
    ERRNO,
    DBGSTATUS,
    NO_OF_CMDS,
}ClCmdsT;

ClCharT *gHelpStrings[NO_OF_CMDS] = {
    /* sleep */
    "\nUsage: sleep <sleep time>\n"
    "\t<sleep time> [INT/DEC] - session sleeps for specified number of "
    "seconds.\n",

    /* timeoutset */
    "\nUsage: timeoutset <timeout>\n"
    "\t<timeout> [INT/DEC] - timeout for the debug session.\n",

    /* timeoutget */
    "\nUsage: timeoutget\n",

    /* setc */
    "\nUsage: setc <arg>\n"
    "\t<arg> [STRING/DEC] - in ASP context, this is a number which identifies"
    " the slot id or \"master\" to denote the leader\nand in node context"\
    " it is a string which identifies the "
    "component to setc context to.\n",

    /* intro */
    "\nThe debug CLI is a command line interface to debug the various "
    "components in the system that provide a debug interface.\n The debug "
    "CLI recognizes 3 levels of operation, called 'context's. It is possible "
    "to set context to and out of these levels. These levels are:\n"
    "\t1. ASP level - in this level, the CLI has visibility throughout "
    "the ASP and it can change the context to the next level\n"
    "\t2. Node level - in this level, the CLI has visibility throughout "
    "the selected node and it can change the context to the next level\n"
    "\t3. Component level - in this level, the CLI has visibility for the "
    "selected component and it can access the debug commands provided by "
    "the system.\n"
    "The various commands supported at any level and a single line "
    "description of these commands is provided by typing 'help' on the debug "
    "CLI.\n",

    /* help */
    "\nUsage: help [<arg>]\n"
    "\t<arg> [STRING] - if this command is specified, the usage for this "
    "command is displayed,\n if nothing is specified, the help for all "
    "available commands is displayed.\n",

    /* loglevelset */
    "\nUsage: loglevelset <loglevel>\n"
    "\t<loglevel> [STRING] - this is a string corresponding to the different "
    "log level names.\n",

    /* return the errorcode of last command */
    "\nUsage: errno\n"
    "\terrno - this is a command to find the return code of the last command\n"
    /* return the errorcode of last command */
    "\nUsage: status\n"
    "\tstatus - this is a command to find the status of the last call," 
    "0 indicates success, any non zero value means failure\n"
};

ClCharT gHelpLogSevString[] = 
{
    "\n\tLog level can be any of the following values :\n\n"
    /*Severity Strings*/
    "\tCL_LOG_SEV_EMERGENCY\n"
    "\tCL_LOG_SEV_ALERT\n"
    "\tCL_LOG_SEV_CRITICAL\n"
    "\tCL_LOG_SEV_ERROR\n"
    "\tCL_LOG_SEV_WARNING\n"
    "\tCL_LOG_SEV_NOTICE\n"
    "\tCL_LOG_SEV_INFO\n"
    "\tCL_LOG_SEV_DEBUG\n"
};


typedef struct ClDebugCondInfoT
{
    ClRcT          rc ;
} ClDebugCondInfoT;

/*This structure describes the context of a component*/
typedef struct ClDebugCompContext {
    ClUint32T  numList;
    ClCharT ** list;
    ClCharT *  buf;
} ClDebugCompContextT;

/*This structure describes the context of a debug*/
typedef struct ClDebugContext
{
    ClUint32T            isNodeAddressValid;
    ClIocNodeAddressT    nodeAddress;
    ClUint32T            isCommPortValid;
    ClIocPortT           commPort;
    ClUint32T            slotId;
    ClCharT              prompt[CL_DEBUG_CLI_PROMPT_LEN];
} ClDebugContextT;

/*This structure describes the debugCli*/
typedef struct ClDebugCli
{
    ClCharT*         argv[MAX_ARGS];
    ClCharT          argvBuf[MAX_ARGS*MAX_ARG_BUF_LEN];
    ClUint32T        argc;
    ClCharT          dbgCliBuf[CLI_CMD_LEN];
    ClCharT          dbgCommands[COMMAND_TOTAL][CLI_CMD_LEN];
    ClUint32T        dbgToBeExecutedCommand;
    ClUint32T        dbgTotalCommand;
    ClUint32T        dbgFirstCommand;
    ClUint32T        dbgLastCommand;
    ClUint32T        dbgInputSetting;
    ClUint32T        dbgLocalSetting;
    ClUint32T        timeout;
    ClCharT          prompt[CL_DEBUG_CLI_USER_PROMPT_LEN];
    ClDebugContextT  context;
} ClDebugCliT;


/*This structure discribes about commands*/ 
typedef struct cmdEntry {
    ClCharT cmdName[80];
    ClRcT  (*fp)(ClUint32T, ClCharT**);
    ClCharT helpString[80];

} ClCmdEntryT;

/*This structure discribes about modules Entires*/
typedef struct modEntry {
    ClCharT       modName[80];
    ClCharT       modPrompt[20];
    ClCmdEntryT*  cmdList;
    ClCharT       help[80];
} ClModEntryT;


/* To set the terminal for our input */
static void  dbgSetSettings (ClUint32T* pDbgInputSetting,
                             ClUint32T* pDbgLocalSetting);
/* To restore the old stage*/
static void  dbgRestoreSettings (ClUint32T* pDbgInputSetting,
                                 ClUint32T* pDbgLocalSetting);
/*To get the command from the debugcli command line*/
static void  dbgCliCommandGets (ClDebugCliT* pDebugObj, ClUint32T idx,
                                ClCharT *ptrPrompt);
/*Printing the prompt according to the level*/
static void  printPrompt(ClCharT *prompt);
/*Setting up the argument*/
static void  setupArgv(ClCharT *buf, ClUint32T *argc, ClCharT** argv);
/*Clearing the argument*/
static void  clearArgBuf(ClUint32T *argc, ClCharT **argv);
/*To check the command completion */
static ClRcT cmdCompletion(ClDebugCliT* pDebugObj, ClCharT *ptrPrompt);
/*Intialize the debugCli*/
static ClRcT debugCliInitialize(ClDebugCliT** ppDebugObj, ClCharT* name);
/*To bring the debugCli as shell*/
static ClUint32T   debugCliShell(ClDebugCliT* pDebugObj);
static ClRcT debugCliFinalize(ClDebugCliT** ppDebugObj);
static ClRcT appInitialize(ClUint32T argc, ClCharT* argv[]);

static ClRcT appStateChange(ClEoStateT eoState);
static ClRcT appHealthCheck(ClEoSchedFeedBackT* schFeedback);
static ClRcT cmdListInit(ClDebugCliT *pDebugObj);
static ClRcT argCompletion(ClDebugCliT* pDebugObj, ClCharT *ptrPrompt,
                           ClCharT *cmd, ClInt32T argLen,
                           ClInt32T cmdLen, ClUint32T *len);

static ClUint32T    shouldIUnblock = 0;
static ClDebugCliT  *pGDebugObj = 0;


ClCharT* helpGeneric[] = 

{
    "setc", "set context to node",
    "bye,quit,exit", "quit the cli",
    "help", "lists commands in current mode",
    "?", "lists commands in current mode",
    "list", "list nodes to which setc can be done in current mode",
    "history", "display the last 100 commands executed",
    "timeoutset", "set the timeout for the debug session ",
    "timeoutget", "get the timeout of the debug session",
    "sleep", "sleep for the specified time",
    "errno", "return code of the last executed call",
    "status", "sucess/failure status of the last executed call",
};

ClCharT* helpNodeLevel[] = 
{
    "setc", "set context to component",
    "end", "exit current mode",
    "bye,quit,exit", "quit the cli",
    "help", "lists commands in current mode",
    "?", "lists commands in current mode",
    "list", "list components to which setc can be done in current mode",
    "history", "display the last 100 commands executed",
    "loglevelset", "set the log level of the component",
    "loglevelget", "get the log level of the component",
    "timeoutset", "set the timeout for the debug session ",
    "timeoutget", "get the timeout of the debug session",
    "sleep", "sleep for the specified time",
    "errno", "return code of the last executed call",
    "status", "sucess/failure status of the last executed call",
};

ClCharT* helpCompLevel[] = 
{
    "end", "exit current mode",
    "bye,quit,exit", "quit the cli",
    "help", "lists commands in current mode",
    "?", "lists commands in current mode",
    "history", "display the last 100 commands executed",
    "timeoutset", "set the timeout for the debug session ",
    "timeoutget", "get the timeout of the debug session",
    "sleep", "sleep for the specified time",
    "errno", "return code of the last executed call",
    "status", "sucess/failure status of the last executed call",
};

typedef struct debugCmds
{
    ClCharT** cmds;
    ClUint32T cmdNum;
}ClDebugCmdsT;

ClDebugCmdsT debugCmdList;

void dbgSignalHandler(int arg)
{
    gDbgInfo.isResponse = CL_FALSE;
    clOsalCondSignal(gDbgInfo.condVar);
    return;
}

static ClRcT debugCliInitialize(ClDebugCliT** ppDebugObj, ClCharT* prompt)
{
    ClRcT      rc = CL_OK;
    ClUint32T  idx = 0;
    ClUint32T  jdx = 0;

    if (NULL == ppDebugObj)
    {
        rc = CL_DEBUG_RC(CL_ERR_NULL_POINTER);
        return rc;
    }
    (*ppDebugObj) = (ClDebugCliT*)clHeapAllocate(sizeof(ClDebugCliT));
    if (NULL == *ppDebugObj)
    {
        rc = CL_DEBUG_RC(CL_ERR_NO_MEMORY);
        clLogCritical("DBG","INI","Failed to allocate memory [0x %x]"  
                                         " \n",rc);
        return rc;
    }

    (*ppDebugObj)->dbgTotalCommand = 0;
    (*ppDebugObj)->dbgFirstCommand = 0;
    (*ppDebugObj)->dbgLastCommand  = 0;
    (*ppDebugObj)->dbgInputSetting = 0;
    (*ppDebugObj)->dbgLocalSetting = 0;
    (*ppDebugObj)->timeout         = CL_DBG_DEFAULT_COMMAND_TIMEOUT; 
    (*ppDebugObj)->dbgToBeExecutedCommand = 0;

    if (strlen(prompt) < CL_DEBUG_CLI_USER_PROMPT_LEN)
    {
        strcpy( (*ppDebugObj)->prompt , prompt);
    }
    else
    {
        clHeapFree(*ppDebugObj);
        rc = CL_DEBUG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("DBG","INI","The Passed parameter is invalid "
                                      " [0x %x]\n",rc);
        return rc;
    }

    (*ppDebugObj)->argc = 0;
    for (idx =0; idx < MAX_ARGS; idx++)
    {
        (*ppDebugObj)->argv[idx] =
            &((*ppDebugObj)->argvBuf[idx*MAX_ARG_BUF_LEN]);
        for (jdx = 0; jdx < MAX_ARG_BUF_LEN; jdx++)
        {
            (*ppDebugObj)->argv[idx][jdx] = 0;
        }
    }
    (*ppDebugObj)->context.isNodeAddressValid = 0;
    (*ppDebugObj)->context.isCommPortValid = 0;
    rc = clHandleDatabaseCreate(NULL,&gDbgInfo.databaseHdl);
    if( CL_OK != rc)
    {
        clHeapFree(*ppDebugObj);
        clLogError("DBG","INI","Error in handle database creation "
                                      " [0x %x]\n",rc);
        return rc;

    }
    rc = clOsalCondCreate(&gDbgInfo.condVar);
    if( CL_OK != rc)
    {
        clHandleDatabaseDestroy(gDbgInfo.databaseHdl);
        clHeapFree(*ppDebugObj);
        clLogError("DBG","INI","Error in Condtional variable creation " 
                                       " [0x %x]\n",rc);
        return rc;
    }
    rc = clOsalMutexCreate(&gDbgInfo.mutexVar);
    if( CL_OK != rc)
    {
        clOsalCondDelete(gDbgInfo.condVar);
        clHandleDatabaseDestroy(gDbgInfo.databaseHdl);
        clHeapFree(*ppDebugObj);
        clLogError("DBG","INI",
                       "Error in mutex creation [0x %x]\n",rc);
    }

    return rc;
}

static ClRcT debugCliFinalize(ClDebugCliT** ppDebugObj)
{

    dbgRestoreSettings ( &(pGDebugObj->dbgInputSetting), 
                         &(pGDebugObj->dbgLocalSetting));
    clOsalMutexDelete(gDbgInfo.mutexVar);                         
    clOsalCondDelete(gDbgInfo.condVar);                         
    clHandleDatabaseDestroy(gDbgInfo.databaseHdl);
    clHeapFree(*ppDebugObj);
    ppDebugObj = NULL;
    return CL_OK;
}

/*************************************************************************
 * This function sets the termial to raw and no echo mode.
 *
 * @return	None
 *
 *************************************************************************/
#ifdef VXWORKS_BUILD

static void
dbgSetSettings (ClUint32T* pDbgInputSetting, ClUint32T* pDbgLocalSetting)
{
    ClInt32T inputSettings = 0;
    if( (inputSettings = ioctl(0, FIOGETOPTIONS, 0)) == ERROR)
        return;
    *pDbgInputSetting = inputSettings;
    *pDbgLocalSetting = 0;

}

static void
dbgRestoreSettings (ClUint32T* pDbgInputSetting, ClUint32T* pDbgLocalSetting)
{
    ioctl(0, FIOSETOPTIONS, *pDbgInputSetting);
}

#else

/*****************************************************************************
 * This function restores the termial back to normal mode.
 *
 * @return	None
 *
 ****************************************************************************/

static void
dbgSetSettings (ClUint32T* pDbgInputSetting, ClUint32T* pDbgLocalSetting)
{
    struct termios  settings;
    ClUint32T       result = 0;

    memset ((void *) &settings, 0, sizeof (struct termios));

    result = tcgetattr (STDIN_FILENO, &settings);
    if (result != 0)
    {
        return;
    }
    *pDbgInputSetting = settings.c_iflag;
    *pDbgLocalSetting = settings.c_lflag;

#ifdef SOLARIS_BUILD
    settings.c_lflag &= CL_DEBUG_LOCAL_SETTING;
#else
    settings.c_lflag &= CL_DEBUG_LOCAL_SETTING;
    settings.c_iflag &= (CL_DEBUG_INPUT_SETTING);
#endif
    result = tcsetattr (STDIN_FILENO, TCSANOW, &settings);
    if (result != 0)
    {
        return;
    }

    /*  system ("stty raw");
        system ("stty -echo");*/
}

static void
dbgRestoreSettings (ClUint32T* pDbgInputSetting, ClUint32T* pDbgLocalSetting)
{

    struct termios  settings;
    ClUint32T       result = 0;

    memset ((void *) &settings, 0, sizeof (struct termios));

    result = tcgetattr (STDIN_FILENO, &settings);
    if (result != 0)
    {
        return;
    }
    settings.c_iflag |= *pDbgInputSetting;
    settings.c_lflag |= *pDbgLocalSetting;

    result = tcsetattr (STDIN_FILENO, TCSANOW, &settings);
    if (result != 0)
    {
        return;
    }
    /*
      system ("stty -raw");
      system ("stty echo");
    */
}

#endif

/****************************************************************************
 * This function gets a line of command from cli prompt.
 *
 * @return	None
 *
 ****************************************************************************/
static void
dbgCliCommandGets (ClDebugCliT* pDebugObj, ClUint32T idx, ClCharT *ptrPrompt)
{
    ClInt32T   his;
    ClInt32T  c;
    ClUint32T  commandDone = 0;
    ClUint32T  i, index = 0, j, jj, toBeCopied, l, maxLenth = 0;
    ClRcT      ret = CL_OK;

    for (j = 0; j < CLI_CMD_LEN - 1; j++)
    {
        pDebugObj->dbgCliBuf[j] = ' ';
    }
    pDebugObj->dbgCliBuf[CLI_CMD_LEN - 1] = '\0';

    index = 0;
    maxLenth = 0;
    fflush (stdin);

    dbgSetSettings (&(pDebugObj->dbgInputSetting),
                    &(pDebugObj->dbgLocalSetting));

    while (commandDone == 0)
    {
        c = getchar ();
        switch (c)
        {

        case EOF : /* end of file */ 
            commandDone = 1;
            shouldIUnblock = 1;
            break;

        case '\t':		/* tab */
            ret = cmdCompletion (pDebugObj, ptrPrompt);
            for (i = 0; i < (CLI_CMD_SZ - 1); i++)
            {
                if (((pDebugObj->dbgCliBuf[i] == 0x00) ||
                     (pDebugObj->dbgCliBuf[i] == 0x20)) &&
                    ((pDebugObj->dbgCliBuf[i + 1] == 0x00) ||
                     (pDebugObj->dbgCliBuf[i + 1] == 0x20)))
                {
                    maxLenth = index = i;
                    break;
                }
            }
            if (ret == 0)
                break;
            else
                continue;

        case 13  :		/* new line */
        case '\n':		/* new line */
            dbgRestoreSettings ( &(pDebugObj->dbgInputSetting),
                                 &(pDebugObj->dbgLocalSetting));
            printf ("\n");
            fflush (stdout);
            commandDone = 1;
            pDebugObj->dbgCliBuf[maxLenth] = '\0';
            if (strlen (pDebugObj->dbgCliBuf) > 0)
            {
                if (pDebugObj->dbgTotalCommand < COMMAND_TOTAL)
                {
                    pDebugObj->dbgTotalCommand++;
                }
                else
                {
                    pDebugObj->dbgFirstCommand =
                        ( pDebugObj->dbgFirstCommand + 1) % COMMAND_TOTAL;
                }
                if (pDebugObj->dbgCliBuf[0] != '#')
                {
                    strncpy ( pDebugObj->dbgCommands[
                                 pDebugObj->dbgLastCommand],
                             pDebugObj->dbgCliBuf,CLI_CMD_LEN-1);
                }
                else
                {
                    strncpy ( pDebugObj->dbgCommands[
                                 pDebugObj->dbgLastCommand],
                             &(pDebugObj->dbgCliBuf[1]), CLI_CMD_LEN-1);
                }
                pDebugObj->dbgLastCommand =
                    ( pDebugObj->dbgLastCommand + 1) % COMMAND_TOTAL;
                pDebugObj->dbgToBeExecutedCommand =
                    pDebugObj->dbgLastCommand;
            }
            pDebugObj->dbgCliBuf[maxLenth] = '\n';
            pDebugObj->dbgCliBuf[maxLenth + 1] = '\0';
            break;
        case 0x7f:
            c = 0x8;    /* intentional fallthrough */
        case 0x8 :		/* back space */
            if (index > 0)
            {
                printf ("%c", c);
                fflush (stdout);
                index--;
                for (j = index; j < maxLenth; j++)
                { 
                    pDebugObj->dbgCliBuf[j] = pDebugObj->dbgCliBuf[j + 1];
                }

                toBeCopied = maxLenth - index;
                for (l = index; l < maxLenth; l++)
                { 
                    if (pDebugObj->dbgCliBuf[l] == '\0')
                    {
                        printf ("%c", ' ');
                        fflush (stdout);
                    }
                    else
                    {
                        printf ("%c", pDebugObj->dbgCliBuf[l]);
                        fflush (stdout);
                    }
                }
                for (l = 0; l < toBeCopied; l++)
                {
                    printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                    fflush (stdout);
                }
                maxLenth--;
            }
            break;
        case 0x1b: /*ESC */
            c = getchar ();
            if (c == 0x5b) /*[ */
            {
                c = getchar ();
                if (c == 0x44)	/* left */
                {
                    if (index > 0)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                }
                else if (c == 0x43)	/* right */
                {
                    if (index < maxLenth)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x43);
                        fflush (stdout);
                        index++;
                    }
                }
                else if (c == 0x41)	/* up */
                {
                    while (index > 0)
                    { 
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                    while (index < maxLenth)
                    {
                        printf (" ");
                        fflush (stdout);
                        index++;
                    }
                    j = 0;
                    do
                    {
                        pDebugObj->dbgToBeExecutedCommand =
                            (pDebugObj->dbgToBeExecutedCommand + 
                             COMMAND_TOTAL - 1) % COMMAND_TOTAL;
                        j++;
                    }while (!strlen (pDebugObj->dbgCommands[pDebugObj->
                                                            dbgToBeExecutedCommand]) &&
                            (j < COMMAND_TOTAL));
                    strncpy (pDebugObj->dbgCliBuf,
                            pDebugObj->dbgCommands[
                                pDebugObj->dbgToBeExecutedCommand], CLI_CMD_LEN-1);
                    while (index > 0)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                    printf ("%s", pDebugObj->dbgCliBuf);
                    fflush (stdout);
                    maxLenth = index = strlen (pDebugObj->dbgCliBuf);
                }
                else if (c == 0x42)	/* down */
                { 
                    while (index > 0)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                    while (index < maxLenth)
                    {
                        printf (" ");
                        fflush (stdout);
                        index++;
                    }
                    j = 0;
                    do
                    {
                        pDebugObj->dbgToBeExecutedCommand =
                            (pDebugObj->dbgToBeExecutedCommand
                             + COMMAND_TOTAL + 1) % COMMAND_TOTAL;
                        j++;
                    }
                    while (!strlen (pDebugObj->dbgCommands[pDebugObj->
                                                           dbgToBeExecutedCommand])
                           && (j < COMMAND_TOTAL));
                    strncpy ( pDebugObj->dbgCliBuf,
                             pDebugObj->dbgCommands[
                                 pDebugObj->dbgToBeExecutedCommand], CLI_CMD_LEN-1);
                    while (index > 0)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                    printf ("%s", pDebugObj->dbgCliBuf);
                    fflush (stdout);
                    maxLenth = index = strlen (pDebugObj->dbgCliBuf);
                }
                else if(c == 0x31)
                {
                    c = getchar();
                    for(l = index ; l > 0 ;l--)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                        index--;
                    }
                }
                else if(c == 0x34)
                {
                    c = getchar();
                    while(index < maxLenth)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x43);
                        fflush (stdout);
                        index++;
                    }     
                }    
                else if(c == 0x33)       
                {
                    c = getchar();
                    for (j = index; j < maxLenth; j++)
                    { 
                        pDebugObj->dbgCliBuf[j] =
                            pDebugObj->dbgCliBuf[j + 1];
                    }
                    toBeCopied = maxLenth - index;
                    for (l = index; l < maxLenth; l++)
                    { 
                        if (pDebugObj->dbgCliBuf[l] == '\0')
                        {
                            printf ("%c", ' ');
                            fflush (stdout);
                        }
                        else
                        {
                            printf ("%c", pDebugObj->dbgCliBuf[l]);
                            fflush (stdout);
                        }  
                    }
                    for (l = 0; l < toBeCopied; l++)
                    {
                        printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                        fflush (stdout);
                    }
                    if(index < maxLenth)
                        maxLenth--;
                }
            }
            else
            {
                /* break for now */
                break;
            }
            break;
        default:    
            if (index >=(CLI_CMD_LEN -2))
            {
                printf ("%c", 0x7);
                fflush (stdout);
                break;
            }
            if (index >= maxLenth)
            {
                printf ("%c", c);
                fflush (stdout);
                pDebugObj->dbgCliBuf[index++] = c;
                maxLenth = index;
            }
            else
            {
                /* Move one over to the right for all chars on the right
                   hand side of index (include index)   
                */
                toBeCopied = maxLenth - index;
                for (j = maxLenth; j > index; j--)
                {
                    pDebugObj->dbgCliBuf[j] = pDebugObj->dbgCliBuf[j - 1];
                }
                maxLenth++;
                pDebugObj->dbgCliBuf[index++] = c;

                for (l = index - 1; l < maxLenth; l++)
                {
                    printf ("%c", pDebugObj->dbgCliBuf[l]);
                    fflush (stdout);
                }
                for (l = 0; l < toBeCopied; l++)
                {
                    printf ("%c%c%c", 0x1b, 0x5b, 0x44);
                    fflush (stdout);
                }
            }
            break;
        }
    }
    commandDone = 0;
    if (pDebugObj->dbgCliBuf[0] == '!')
    {
        sscanf (pDebugObj->dbgCliBuf + 1, "%d", &his);
        if (his != 0)
        {
            if (pDebugObj->dbgTotalCommand <= COMMAND_TOTAL)
            {
                j = 1;
            }
            else
            {
                j = pDebugObj->dbgTotalCommand - COMMAND_TOTAL;
            }
            for ( i = pDebugObj->dbgFirstCommand;
                  j <= pDebugObj->dbgTotalCommand;
                  i = (i + 1) % COMMAND_TOTAL, j++)
            {
                if (j == his)
                {
                    strncpy (pDebugObj->dbgCliBuf, pDebugObj->dbgCommands[i], CLI_CMD_LEN-1);
                    printf ("          %s\n", pDebugObj->dbgCommands[i]);
                    jj = strlen (pDebugObj->dbgCliBuf);
                    pDebugObj->dbgCliBuf[jj] = '\n';
                    pDebugObj->dbgCliBuf[jj + 1] = '\0';
                    strncpy(pDebugObj->dbgCommands[
                               pDebugObj->dbgToBeExecutedCommand- 1],
                           pDebugObj->dbgCliBuf, CLI_CMD_LEN-1);
                    pDebugObj->dbgCommands[
                        pDebugObj->dbgToBeExecutedCommand - 1][jj] = '\0';
                    break;
                }
            }
        }
    }
    if (!strcmp (pDebugObj->dbgCliBuf, "history\n"))
    {
        if (pDebugObj->dbgTotalCommand <= COMMAND_TOTAL)
        {
            j = 1;
        }
        else
        {
            j = pDebugObj->dbgTotalCommand - COMMAND_TOTAL;
        }
        for ( i = pDebugObj->dbgFirstCommand; j <= pDebugObj->dbgTotalCommand;
              i = (i + 1) % COMMAND_TOTAL, j++)
        {
            printf ("%8d  %s\n", j, pDebugObj->dbgCommands[i]);
        }
    }

    dbgRestoreSettings ( &(pDebugObj->dbgInputSetting),
                         &(pDebugObj->dbgLocalSetting));
}


/**************************************************************************
 **
 ** NAME: printPrompt
 **
 ** DESCRIPTION:
 **
 ** ARGUMENTS:
 **
 ** RETURNS:
 **
 **************************************************************************/
static void
printPrompt(ClCharT *prompt)
{
    printf("\r\n%s", prompt);
}


/**************************************************************************
 **
 ** NAME: setupArgv
 **
 ** DESCRIPTION:
 **
 ** ARGUMENTS:
 **
 ** RETURNS:
 **
 **************************************************************************/
static void
setupArgv(ClCharT *buf, ClUint32T *argc, ClCharT** argv)
{
    ClUint32T  i = 0; 
    ClUint32T  j = 0;
    ClCharT    *ptr = NULL;

    *argc = 0;
    while(buf[i]==' ')
    {
        i++;
    }
    while(buf[i] && (*argc < MAX_ARGS))
    {
        ptr = argv[*argc];
        if (buf[i] == ' ')
        {
            j = 0;
            (*argc)++;
            while (buf[i]==' ')
            {
                i++;
            }
            continue;

        }

        if ((buf[i] == '"') && (i == 0 || buf[i-1] != '\\'))
        {
            i++;
            while (buf[i] != '"')
            {
                if (buf[i] == '\\' && buf[i+1] == '"')
                {
                    i++;
                }
                ptr[j] = buf[i];
                i++;
                j++;
                if (buf[i] == '\n'||buf[i] == '\0')
                {
                    printf("\nMissing \"\n");
                    break;
                }
            }
            buf[j] = 0;
            i++;
        }

        else if ((buf[i] == '\0') || (buf[i] == '\n') 
                 || (buf[i] == '\t'))
        {
            if( 0 != j )
            {
                (*argc)++;
            }
            break;
        }
        else 
        {
            ptr[j] = buf[i];
            i++;
            j++;
            ptr[j] = 0;
        }
    }
    if(ptr == NULL)
    {
        printf("\nUnrecognized Command\n"
               "Commands append or prepend with some unknown characters\n");
    }
}


/**************************************************************************
 **
 ** NAME: clearArgBuf
 **
 ** DESCRIPTION:
 **
 ** ARGUMENTS:
 **
 ** RETURNS:
 **
 **************************************************************************/
static void
clearArgBuf(ClUint32T *argc, ClCharT **argv)
{
    ClUint32T idx;

    for (idx = 0; idx < *argc; idx++)
        *(argv[idx]) = 0;
    *argc =0;
}

void dbgCommandReplyCallback( ClRcT retCode, ClPtrT pData,
                              ClBufferHandleT inMsg,
                              ClBufferHandleT  outMsg)
{
    ClHandleT  invokeHdl        = (ClHandleT)(ClWordT)pData;
    void       *pHdl            = NULL;
    ClDebugCondInfoT *pCondInfo = NULL;
    ClRcT             rc        = CL_OK;
    rc = clHandleCheckout(gDbgInfo.databaseHdl,invokeHdl,(void **)&pHdl);
    if( CL_OK != rc)
    {
        /*its invalid handle .ignore it*/
        clBufferDelete(&outMsg);
        return;
    }

    pCondInfo = pHdl;
    pCondInfo->rc = retCode;
    rc = clHandleCheckin(gDbgInfo.databaseHdl,invokeHdl);
    clOsalMutexLock(gDbgInfo.mutexVar);
    gDbgInfo.isResponse = CL_TRUE;
    clOsalCondSignal(gDbgInfo.condVar);
    clOsalMutexUnlock(gDbgInfo.mutexVar);
    return; 
}

static ClRcT
invoke( ClDebugCliT* pDebugObj,ClUint32T argc, ClCharT** argv, 
        ClCharT** retStr)
{
    ClUint32T              rmdFlags   = 0; 
    ClBufferHandleT        inMsgHdl   = 0;
    ClBufferHandleT        outMsgHdl  = 0;
    ClVersionT             version    = {0};
    ClUint32T              i          = 0;
    ClRcT                  rc         = CL_OK;
    ClDebugCondInfoT       *pCondInfo = NULL;
    void                   *pHdl      = NULL;
    ClRmdOptionsT          rmdOptions;
    ClIocAddressT          iocAddr;
    ClRmdAsyncOptionsT     asyncOptions;
    ClTimerTimeOutT        timeout;
    ClHandleT              invokeHdl  = CL_HANDLE_INVALID_VALUE;
    
    rc = clBufferCreate(&inMsgHdl);
    if (CL_OK != rc)
    {
        goto L2;
    }
    rc = clXdrMarshallClVersionT(versionDatabase.versionsSupported,inMsgHdl, 0);

    if(CL_OK != rc)
    {
        goto L3;
    }
    rc = clXdrMarshallClUint32T(&argc,inMsgHdl,0);
    if(CL_OK != rc)
    {
        goto L3;
    }

    for (i = 0; i < argc; i++)
    {
        ClUint32T stringLength;

        stringLength = strlen(argv[i]);

        rc = clXdrMarshallClUint32T(&stringLength,inMsgHdl,0);
        if (CL_OK != rc)
        {
            goto L3;
        }

        rc = clXdrMarshallArrayClCharT(argv[i],stringLength,inMsgHdl,0);
        if (CL_OK != rc)
        {
            goto L3;
        }
    }
    rc = clHandleCreate(gDbgInfo.databaseHdl, sizeof(ClDebugCondInfoT),
                        &invokeHdl);
    if( CL_OK != rc)
    {
        goto L3;
    }
    timeout.tsSec = pDebugObj->timeout / 1000;
    timeout.tsMilliSec = pDebugObj->timeout % 1000;

    iocAddr.iocPhyAddress.nodeAddress = pDebugObj->context.nodeAddress;
    iocAddr.iocPhyAddress.portId = pDebugObj->context.commPort;
    rmdOptions.timeout = pDebugObj->timeout;
    rmdOptions.retries = 0;
    rmdOptions.priority = 0; 
    rmdOptions.transportHandle = 0; 
    rc = clBufferCreate(&outMsgHdl);
    if (CL_OK != rc)
    {
        goto L4;
    }
    rmdFlags = CL_RMD_CALL_ASYNC  | CL_RMD_CALL_NEED_REPLY
        | CL_RMD_CALL_NON_PERSISTENT;
    asyncOptions.pCookie = (ClPtrT)(ClWordT)invokeHdl;
    asyncOptions.fpCallback = dbgCommandReplyCallback;
    /*
     * take a lock to ensure the callback will not signal,
     * before this guy wait on the signal
     */
    clOsalMutexLock(gDbgInfo.mutexVar);
    rc = clRmdWithMsg( iocAddr, CL_DEBUG_INVOKE_FUNCTION_FN_ID,
                       inMsgHdl, outMsgHdl,rmdFlags,
                       &rmdOptions,&asyncOptions);
    if( CL_OK != rc)
    {
        *retStr = (ClCharT*)clHeapAllocate(
            strlen(gStrDb[0]) + 1);
        if (NULL != *retStr)
        {
            sprintf(*retStr, gStrDb[0]);
        }
        clOsalMutexUnlock(gDbgInfo.mutexVar);
        goto L1;
    }
    rc = clOsalCondWait(gDbgInfo.condVar, gDbgInfo.mutexVar,timeout);
    /* unlock and go out */
    clOsalMutexUnlock(gDbgInfo.mutexVar);
    
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
    {
        *retStr = (ClCharT*)clHeapAllocate(
            strlen(gStrDb[1]) + 1);
        if (NULL != *retStr)
        {
            sprintf(*retStr, gStrDb[1]);
        }
        goto L1;
    }
    if(gDbgInfo.isResponse != CL_TRUE)
    {
        /*its coming from signal.so no processing */
        *retStr = (ClCharT*)clHeapAllocate(
            strlen(gStrDb[2]) + 1);
        if (NULL != *retStr)
        {
            sprintf(*retStr, gStrDb[2]);
        }
        goto L1;
    }
    rc = clHandleCheckout(gDbgInfo.databaseHdl,invokeHdl,(void **)&pHdl);
    if( CL_OK != rc)
    {
        /*its invalid handle .ignore it*/
        *retStr = (ClCharT*)clHeapAllocate(
            strlen(gStrDb[3]) + 1);
        if (NULL != *retStr)
        {
            sprintf(*retStr, gStrDb[3]);
        }
        clBufferDelete(&outMsgHdl);
        return rc;
    }
    pCondInfo = pHdl;
    rc = pCondInfo->rc;
    clHandleCheckin(gDbgInfo.databaseHdl,invokeHdl);
    if (CL_OK != rc)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            rc = clXdrUnmarshallClVersionT(outMsgHdl,&version);
            if(rc != CL_OK)
            {
                printf("Version unmarshall function failed. error code [0x%x].",rc);
                goto L1;
            }
            printf("The supporeted Version is %c %d %d",version.releaseCode,
                   version.majorVersion,
                   version.minorVersion);
        }
        else if (CL_RMD_TIMEOUT_UNREACHABLE_CHECK(rc))
        {
            *retStr = (ClCharT*)clHeapAllocate(strlen(gStrDb[1]) + 1);
            if (NULL != *retStr)
            {
                sprintf(*retStr, gStrDb[1]);
            }
            goto L1;
        }

        *retStr = (ClCharT*)clHeapAllocate(strlen(gStrDb[4]) + 1);
        if (NULL != *retStr)
        {
            sprintf(*retStr, gStrDb[4]);
        }
        goto L1;
    }

    rc = clXdrUnmarshallClUint32T(outMsgHdl,&i);
    if (CL_OK != rc)
    {
        goto L1;
    }

    *retStr = (ClCharT*)clHeapAllocate(i + 1);
    if (NULL != *retStr)
    {
        rc = clXdrUnmarshallArrayClCharT(outMsgHdl,*retStr,i);
        if (CL_OK != rc)
        {
            goto L1;
        }
        (*retStr)[i] = '\0';
    }
    else
    {
        clLogError("DBG",CL_LOG_CONTEXT_UNSPECIFIED,"Failed to allocate the Memory");
    }
    ClRcT errCode = CL_OK;
    rc = clXdrUnmarshallClUint32T(outMsgHdl, &errCode);
    if( CL_OK != rc )
    {
        /* error code should be returned */
        goto L1;
    }
    rc = errCode;

L1:
    clBufferDelete(&outMsgHdl);
    clHandleDestroy(gDbgInfo.databaseHdl,invokeHdl);
    return rc;
L4: 
    clHandleDestroy(gDbgInfo.databaseHdl,invokeHdl);
L3: 
    clBufferDelete(&inMsgHdl);
L2:
    *retStr = 0;
    return rc;
}


static ClRcT initCompContext( ClDebugCompContextT* context, 
                              ClUint32T numList,
                              ClUint32T listSize)
{
    ClRcT  rc = CL_OK;
    if (context == NULL)
    {
        return CL_DEBUG_RC(CL_ERR_NULL_POINTER);
    }

    context->list = (ClCharT**)clHeapAllocate(numList * sizeof(ClCharT*));
    if (NULL == context->list)
    {
        rc = CL_DEBUG_RC(CL_ERR_NO_MEMORY);
        clLogCritical("CTX","INI","Failed to allocate memory "
                                         " [0x %x]\n",rc);
        return rc;
    }

    context->buf = (ClCharT*)clHeapAllocate(listSize * sizeof(ClCharT*));
    if (NULL == context->buf)
    {
        rc = CL_DEBUG_RC(CL_ERR_NO_MEMORY);
        clLogCritical("CTX","INI","Failed to allocate memory "
                                         " [0x %x]\n",rc);
        clHeapFree(context->list);
        return rc;
    }

    context->numList = numList;

    return CL_OK;
}


static ClRcT finCompContext(ClDebugCompContextT* context)
{
    if(context == NULL)
    {
        return CL_DEBUG_RC(CL_ERR_NULL_POINTER);
    }

    if(NULL != context->buf)
    {
        clHeapFree(context->buf);
    }
    if(NULL != context->list)
    {
        clHeapFree(context->list);
    }
    
    return CL_OK;
}


static ClRcT getCompContext( ClIocNodeAddressT nodeAddress,
                             ClIocPortT commPort,
                             ClDebugCompContextT* context)
{
    ClBufferHandleT inMsgHdl  = 0;
    ClBufferHandleT outMsgHdl = 0;
    ClVersionT             version   ={0};
    ClCharT compName[CL_DEBUG_COMP_NAME_LEN];
    ClCharT compPrompt[CL_DEBUG_COMP_PROMPT_LEN];
    ClCharT funcName[CL_DEBUG_FUNC_NAME_LEN];
    ClCharT funcHelp[CL_DEBUG_FUNC_HELP_LEN];
    ClUint32T i;
    ClUint32T len;
    ClRcT rc;
    ClCharT* ptr;
    rc = clBufferCreate(&inMsgHdl);
    if(CL_OK != rc)
    {
        goto L0;
    }
    rc = clXdrMarshallClVersionT(versionDatabase.versionsSupported,inMsgHdl,
                                 0);
    if(CL_OK != rc)
    {
        clBufferDelete(&inMsgHdl);
        goto L0;
    }
    DEBUG_CALL_RMD_SYNC(nodeAddress,
                        commPort,
                        CL_DEBUG_GET_DEBUGINFO_FN_ID,
                        inMsgHdl,
                        outMsgHdl,
                        rc);
    if (CL_OK != rc)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
        {
            if(CL_OK == clXdrUnmarshallClVersionT(outMsgHdl,&version))
            {
                printf("The supporeted Version is %c %d %d",version.releaseCode,
                        version.majorVersion,
                        version.minorVersion);
            }
            else
            {
                printf("Version mismatch");
            }
        }
        printf("\r\nCould not get information for the component");
        goto L1;
    }

    rc = clXdrUnmarshallArrayClCharT( outMsgHdl,compName,
                                      CL_DEBUG_COMP_NAME_LEN);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrUnmarshallArrayClCharT(outMsgHdl,compPrompt,
                                     CL_DEBUG_COMP_PROMPT_LEN);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = clXdrUnmarshallClUint32T(outMsgHdl,&len);
    if (CL_OK != rc)
    {
        goto L1;
    }

    rc = initCompContext(context,
                         ((len + 1) * 2),
                         (CL_DEBUG_COMP_NAME_LEN +
                          CL_DEBUG_COMP_PROMPT_LEN +
                          len * (CL_DEBUG_FUNC_NAME_LEN +
                                 CL_DEBUG_FUNC_HELP_LEN)));
    if (CL_OK != rc)
    {
        goto L1;
    }

    ptr = context->buf;

    memcpy(ptr, compName, CL_DEBUG_COMP_NAME_LEN);
    context->list[0] = ptr;
    ptr += CL_DEBUG_COMP_NAME_LEN;

    memcpy(ptr, compPrompt, CL_DEBUG_COMP_PROMPT_LEN);
    context->list[1] = ptr;
    ptr += CL_DEBUG_COMP_PROMPT_LEN;

    for (i = 0; i < len; i++)
    {
        rc = clXdrUnmarshallArrayClCharT(outMsgHdl,funcName,
                                         CL_DEBUG_FUNC_NAME_LEN);
        if (CL_OK != rc)
        {
            goto L2;
        }

        memcpy(ptr, funcName, CL_DEBUG_FUNC_NAME_LEN);
        context->list[2 * (i + 1)] = ptr;
        ptr += CL_DEBUG_FUNC_NAME_LEN;

        rc = clXdrUnmarshallArrayClCharT(outMsgHdl,funcHelp,
                                         CL_DEBUG_FUNC_HELP_LEN);
        if (CL_OK != rc)
        {
            goto L2;
        }

        memcpy(ptr, funcHelp, CL_DEBUG_FUNC_HELP_LEN);
        context->list[2 * i + 3] = ptr;
        ptr += CL_DEBUG_FUNC_HELP_LEN;
    }

    goto L1;

L2: finCompContext(context);
L1: clBufferDelete(&outMsgHdl);
L0: 
    return rc;
}

static ClRcT 
cmdListInit(ClDebugCliT* pDebugObj)
{
    ClDebugCompContextT context = {0};
    ClUint32T i = 0;
    ClUint32T j = 0;
    ClUint32T len = 0;
    ClRcT rc = CL_OK;
    ClCharT **temp;
    
    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            rc = getCompContext( pDebugObj->context.nodeAddress,
                                 pDebugObj->context.commPort, &context);
            if (CL_OK != rc)
            {
                return rc;
            }

            temp = (ClCharT**) clHeapAllocate(sizeof(ClCharT*) *
                                            (GET_ARRAY_SIZE(helpCompLevel) + context.numList));
            if (NULL == temp)
            {
                finCompContext(&context);
                return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
            }
            debugCmdList.cmdNum = GET_ARRAY_SIZE(helpCompLevel) + context.numList
                - 2;

            for (i = 2; i < (context.numList); i+=2, j++)
            {
                len = strlen(context.list[i]);
                temp[j] = (ClCharT *) clHeapAllocate(sizeof(ClCharT) * (len + 1));
                temp[j][len] = '\0';
                if(NULL == temp[j])
                {
                    CL_ASSERT(0);
                }
                strncpy(temp[j], context.list[i],len);
            }

            for (i = 0; i < GET_ARRAY_SIZE(helpCompLevel); i+=2, j++)
            {
                len = strlen(helpCompLevel[i]);
                temp[j] = (ClCharT *) clHeapAllocate(sizeof(ClCharT) * (len+1));
                temp[j][len] = '\0';
                strncpy(temp[j], helpCompLevel[i],len);
            }
            finCompContext(&context);
        }
        else
        {
            temp = (ClCharT**) clHeapAllocate(sizeof(ClCharT*) *
                                            (GET_ARRAY_SIZE(helpNodeLevel)));
            for (i = 0; i < GET_ARRAY_SIZE(helpNodeLevel); i=i+2, j++)
            {
                temp[j] = helpNodeLevel[i];
            }

            debugCmdList.cmdNum = GET_ARRAY_SIZE(helpNodeLevel);
        }
    }
    else
    {
        temp = (ClCharT**) clHeapAllocate(sizeof(ClCharT*) *
                                        (GET_ARRAY_SIZE(helpGeneric)));
        for (i = 0; i < GET_ARRAY_SIZE(helpGeneric); i=i+2, j++)
        {
            temp[j] = helpGeneric[i];
        }
        
        debugCmdList.cmdNum = GET_ARRAY_SIZE(helpGeneric);
    }

    if(NULL != debugCmdList.cmds)
    {
        clHeapFree(debugCmdList.cmds);
    }
        
    debugCmdList.cmds = temp;
    debugCmdList.cmdNum /= 2;

    return rc;
}


static ClRcT setPrompt(ClDebugCliT* pDebugObj)
{
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;
    ClCpmSlotInfoT cpmSlotInfo = {0};

    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            ClDebugCompContextT context;
            rc = getCompContext( pDebugObj->context.nodeAddress,
                    pDebugObj->context.commPort,
                    &context);
            if (CL_OK != rc)
            {
                return rc;
            }

            cpmSlotInfo.slotId = pDebugObj->context.slotId;

            retCode = clCpmSlotGet(CL_CPM_SLOT_ID, &cpmSlotInfo);

            if(CL_OK == retCode)
            {

                sprintf( pDebugObj->context.prompt, "cli[%s:%.*s:%s]-> ",
                        pDebugObj->prompt,
                        cpmSlotInfo.nodeName.length,
                        cpmSlotInfo.nodeName.value,
                        context.list[1]);
            }
            else
            {
                sprintf( pDebugObj->context.prompt, "cli[%s:Slot%d:%s]-> ",
                        pDebugObj->prompt,
                        pDebugObj->context.slotId,
                        context.list[1]);
            }

            finCompContext(&context);
        }
        else
        {
            cpmSlotInfo.slotId = pDebugObj->context.slotId;

            retCode = clCpmSlotGet(CL_CPM_SLOT_ID, &cpmSlotInfo);

            if(CL_OK == retCode)
            {

                sprintf( pDebugObj->context.prompt, "cli[%s:%.*s]-> ",
                        pDebugObj->prompt,
                        cpmSlotInfo.nodeName.length,
                        cpmSlotInfo.nodeName.value);
            }
            else
            {
                sprintf( pDebugObj->context.prompt, "cli[%s:Slot%d]-> ",
                        pDebugObj->prompt,
                        pDebugObj->context.slotId);
            }
        }
    }
    else
    {
        sprintf(pDebugObj->context.prompt, "cli[%s]-> ",pDebugObj->prompt);
    }
    return rc;
}


static ClRcT debugHelp(ClDebugCliT* pDebugObj)
{
    ClCharT**  pHelp      = NULL;
    ClUint32T  numHelp    = 0;
    ClRcT      rc         = CL_OK;
    ClUint32T  i          = 0;

    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            ClDebugCompContextT context = {0};

            rc = getCompContext( pDebugObj->context.nodeAddress,
                                 pDebugObj->context.commPort,
                                 &context);
            if (CL_OK != rc)
            {
                return rc;
            }

            for (i = 1; i < context.numList/2; i++)
            {
                printf("\r\n%s - %s", context.list[2 * i], 
                       context.list[2 * i + 1]);
            }
            finCompContext(&context);
            pHelp = helpCompLevel;
            numHelp = sizeof(helpCompLevel)/sizeof(helpCompLevel[0]);
        }
        else
        {
            pHelp = helpNodeLevel;
            numHelp = sizeof(helpNodeLevel)/sizeof(helpNodeLevel[0]);
        }
    }
    else
    {
        pHelp = helpGeneric;
        numHelp = sizeof(helpGeneric)/sizeof(helpGeneric[0]);
    }

    for (i = 0; i < numHelp/2; i++)
    {
        printf("\r\n%s - %s", pHelp[2 * i], pHelp[2 * i + 1]);
    }

    printf("\r\n");

    return rc;
}


static ClRcT exitContext( ClDebugCliT* pDebugObj)
{
    ClRcT rc = CL_OK;

    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            pDebugObj->context.isCommPortValid = 0;
        }
        else
        {
            pDebugObj->context.isNodeAddressValid = 0;
        }
    }
    else
    {
        printf("\r\nCan not exit context any further\n");
        return CL_DEBUG_RC(CL_DBG_ERR_COMMON_ERROR);
    }

    rc = cmdListInit(pDebugObj);
    if(CL_OK != rc)
    {
        printf("Could not initialize command list.\n");
        return rc;
    }

    rc = setPrompt(pDebugObj);

    return rc;
}


static ClRcT enterContext( ClDebugCliT* pDebugObj, ClCharT* name )
{
    ClRcT                   rc = CL_OK;
    SaNameT                 nameStr;
    ClIocAddressT           iocAddress;

    if (pDebugObj->context.isNodeAddressValid)
    {
        if (!pDebugObj->context.isCommPortValid)
        {
            if (strcmp(name, "cpm"))
            {
                strcpy ((ClCharT *)nameStr.value, name);
                nameStr.length = strlen(name);
                if(nameStr.length == 0)
                {
                    printf("\r\nComponent name is missing\n"
                           "\rUsage: setc <component name>\n");
                    return CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                }
                rc = clCpmComponentAddressGet( pDebugObj->context.nodeAddress,
                                               &nameStr,
                                               &iocAddress);
                if (CL_OK != rc)
                {
                    printf("\r\nThe given component '%s' is invalid\n"
                           "\rType 'list' to see proper component names\n",
                           name);
                    return CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                }
            }
            else
            {
                iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            }
            pDebugObj->context.isCommPortValid = 1;
            pDebugObj->context.commPort = iocAddress.iocPhyAddress.portId;

            rc = setPrompt(pDebugObj);
            if (CL_OK != rc)
            {
                pDebugObj->context.isCommPortValid = 0;
            }
        }
        else
        {
            printf("\r\nThis command is not allowed in this context\n");
            return CL_DEBUG_RC(CL_DBG_ERR_INVALID_CTX);
        }

    }
    else
    {
        ClIocNodeAddressT nodeAddress=0;
        ClUint32T          slotId=0;
        ClCharT*          temp = name;
        ClCharT**         pName;
        ClStatusT nodeStatus = CL_STATUS_DOWN;

        pName = &temp;

        if(!strncasecmp(name,"master",6))
        {
            if( (rc = clCpmMasterAddressGet((ClIocNodeAddressT*)&slotId))
                != CL_OK)
            {
                printf("CPM master address get failed with [rc=0x%x]\n",
                       rc);
                return rc;
            }
            goto get_address;
        }
        slotId = (ClUint32T)strtol(name, pName, 10);
        if((ClInt32T)slotId <= 0)
        {
            printf("\r\nInvalid slot number\n"
                   "\rType 'list' to see valid slots\n");
            return CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);       
        }
        if (temp == name)
        {
            printf("\r\nThe argument should be an integer slot number " \
                   "or \"master\"\rUsage:setc <slot number> | master\n");
            return CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
        }

        get_address:

        CL_CPM_IOC_ADDRESS_GET(0, slotId, nodeAddress);
        rc = clCpmNodeStatusGet(nodeAddress,&nodeStatus);
        if(CL_OK != rc || nodeStatus != CL_STATUS_UP)
        {
            printf("\r\nCPM is not running in the given slot. rc=[0x%x], status [%d], node [%d]\n",rc,nodeStatus,nodeAddress);
            return CL_DEBUG_RC(CL_ERR_INVALID_STATE);
        }

        pDebugObj->context.isNodeAddressValid = 1;
        pDebugObj->context.nodeAddress = nodeAddress;
        pDebugObj->context.slotId = slotId;

        rc = setPrompt(pDebugObj);
        if (CL_OK != rc)
        {
            pDebugObj->context.isNodeAddressValid = 0;
        }
    }

    rc = cmdListInit(pDebugObj);
    if(CL_OK != rc)
    {
        printf("Could not initialize command list.\n");
        return rc;
    }

    return rc;
}


static ClRcT debugList(ClDebugCliT* pDebugObj)
{
    ClUint32T i = 0;
    ClRcT     rc = CL_OK;
    ClRcT     retCode = CL_OK;
    ClCpmSlotInfoT cpmSlotInfo = {0};

    /*this is only valid in generic context so check for that*/
    if (!pDebugObj->context.isNodeAddressValid)
    {
        ClUint32T          numNeighbors = 0;
        ClIocNodeAddressT* pNeighborList = NULL;
        ClStatusT           status        = 0;

        rc = clIocTotalNeighborEntryGet(&numNeighbors);
        pNeighborList = clHeapAllocate(numNeighbors
                                     * sizeof(ClIocNodeAddressT));
        if (NULL == pNeighborList)
        {
            return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
        }
        rc = clIocNeighborListGet(&numNeighbors,pNeighborList );
        if (CL_OK != rc)
        {
            clHeapFree(pNeighborList);
            return rc;
        }

        printf("\r\nSlot\tNode");
        if(0 == numNeighbors)
        {
            printf("\r\nNone\tNone");
        }
        else
        {
            for (i = 0; i < numNeighbors; i++)
            {
                clCpmNodeStatusGet(pNeighborList[i],&status); 
                if(status == CL_STATUS_UP)
                {
                    memset(&cpmSlotInfo, '\0', sizeof(ClCpmSlotInfoT));
                    cpmSlotInfo.slotId = pNeighborList[i];

                    printf("\r\n%d",pNeighborList[i]);

                    retCode = clCpmSlotGet(CL_CPM_SLOT_ID, &cpmSlotInfo);

                    if(CL_OK == retCode)
                    {
                        printf("\t%.*s", cpmSlotInfo.nodeName.length,
                                         cpmSlotInfo.nodeName.value);
                    }
                    else if(CL_ERR_DOESNT_EXIST == CL_GET_ERROR_CODE(retCode))
                    {
                        printf("\tNot Avail");
                    }
                    else
                    {
                        printf("\tError : clCpmSlotGet() failed. rc[0x%x] ", rc);
                    }
                }
            }
        }
        printf("\r\n");
        clHeapFree(pNeighborList);
    }
    else
    {
        if (!pDebugObj->context.isCommPortValid)
        {
            ClCharT *retStr = NULL;
            rc = clCpmComponentListDebugAll( pDebugObj->context.nodeAddress,
                                             &retStr);
            if ((CL_OK == rc) && (NULL != retStr))
            {
                printf("cpm\n");
                printf("%s", retStr);
                clHeapFree(retStr);
            }
            else
            {
                printf("Failed to get component list, error [%#x]", rc);
            }
        }
        else
        {
            printf("\r\nThis command is not allowed in this context\n");
            return CL_DEBUG_RC(CL_DBG_ERR_INVALID_CTX);
        }
    }

    printf("\r\n");

    return rc;
}


static ClUint32T debugCliShell(ClDebugCliT* pDebugObj)
{
    ClCharT       *retStr = NULL;
    ClCharT       buf[MAX_ARGS * MAX_ARG_BUF_LEN];
    ClCharT       tmpCh;
    ClUint32T     j;
    ClRcT         rc = CL_OK;
    ClIocAddressT iocAddress;
    ClInt32T      timeout = 0;
    ClInt32T      sleep_time = 0;
    ClCmdsT       cmd;
    ClUint32T     index = 0;
    ClUint32T     matches = 0;
    ClUint32T     prevMatch = 0;
    ClRcT         retCode   = CL_OK;

    if (NULL == pDebugObj)
    {
        rc = CL_DEBUG_RC(CL_ERR_NULL_POINTER);
        clLogError("CLI","SHL","The Passed value is invalid " 
                                      "  [0x %x]\n",rc);
        return rc ;
    }

    rc = cmdListInit(pDebugObj);
    if(CL_OK != rc)
    {
        printf("Could not initialize command list.\n");
        return rc;
    }
    
    setPrompt(pDebugObj);
    printf("\nTo get started, type 'help intro'\n");
    while (!shouldIUnblock)
    {
        printPrompt(pDebugObj->context.prompt);
        memset(buf, 0, sizeof(buf));

        dbgCliCommandGets(pDebugObj, 0, pDebugObj->context.prompt);

        if(shouldIUnblock)
        {
            break;
        }

        strcpy (buf,pDebugObj->dbgCliBuf);

        /* Commands starting with ! are passed to the shell */
        if (buf[0] == '!')
        {
            retCode = system(&pDebugObj->dbgCliBuf[1]);
            if(retCode) retCode = (ClRcT)errno;
        }
        
        /* Check if user has entered just the new line
           don't do anything if that is the case */

        else if ((buf[0] != '\n') && (buf[0] != '#'))
        {
            setupArgv( buf, &(pDebugObj->argc), pDebugObj->argv);
            
            if(strlen(pDebugObj->argv[0]))
            {
                memcpy ( &tmpCh, pDebugObj->argv[0], 1);
                j = strlen(pDebugObj->argv[0]);
            
            
                if((!strncasecmp(pDebugObj->argv[0], "bye", j))
                   ||(!strncasecmp(pDebugObj->argv[0], "quit", j))
                   ||(!strncasecmp(pDebugObj->argv[0], "exit", j))
                   )
                {
                    printf("\nGoodbye!!\n");
                    retCode = CL_OK;
                    exit(0);
                    return 0;
                }
                else if ( (!strncasecmp(pDebugObj->argv[0], "help", j)) ||
                          (!strcasecmp(pDebugObj->argv[0], "?")))
                {
                    ClUint32T k;
                    if (pDebugObj->argv[1][0] == '\0')
                    {
                        debugHelp(pDebugObj);
                        retCode = CL_OK;
                        goto L1;
                    }
                    k = strlen(pDebugObj->argv[1]);
                    if (!strncasecmp(pDebugObj->argv[1], "bye", k) ||
                        !strncasecmp(pDebugObj->argv[1], "quit", k) ||
                        !strncasecmp(pDebugObj->argv[1], "exit", k) ||
                        !strncasecmp(pDebugObj->argv[1], "end", k) ||
                        !strncasecmp(pDebugObj->argv[1], "list", k) ||
                        !strncasecmp(pDebugObj->argv[1], "loglevelget", k) ||
                        !strncasecmp(pDebugObj->argv[1], "history", k))
                    {
                        printf("\nUsage: %s\n", pDebugObj->argv[1]);
                        retCode = CL_OK;
                        goto L1;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "timeoutset", k))
                    {
                        cmd = TIMEOUTSET;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "timeoutget", k))
                    {
                        cmd = TIMEOUTGET;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "loglevelset", k))
                    {
                        cmd = LOGLEVELSET;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "setc", k))
                    {
                        cmd = SETC;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "sleep", k))
                    {
                        cmd = SLEEP;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "errno", k))
                    {
                        cmd = ERRNO;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "status", k))
                    {
                        cmd = DBGSTATUS;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "intro", k))
                    {
                        cmd = INTRO;
                    }
                    else if (!strncasecmp(pDebugObj->argv[1], "help", k))
                    {
                        cmd = HELP;
                    }
                    else
                    {
                        printf("Unrecognized argument. No help available.\n");
                        retCode = CL_DEBUG_RC(CL_DBG_ERR_UNRECOGNIZED_CMD);
                        goto L1;
                    }
                    printf("%s",gHelpStrings[cmd]);
                    retCode = CL_OK;
                    goto L1;
                }
                else if ((!strncasecmp(pDebugObj->argv[0], "end", j)))
                {
                    retCode = exitContext(pDebugObj);
                    goto L1;
                }
                else if (ispunct (tmpCh) != 0)
                {
                    printf("\r\n Command ignored due to leading punctuation mark!"
                           "\r\n");
                    retCode = CL_DEBUG_RC(CL_DBG_ERR_COMMON_ERROR);
                    goto L1;
                }
                

                matches = 0;
                for(index = 0; index < debugCmdList.cmdNum; index++)
                {
                    if(!strncasecmp(pDebugObj->argv[0], debugCmdList.cmds[index], j))
                    {
                        if(j == strlen(debugCmdList.cmds[index]))
                        {
                            matches = 1;
                            prevMatch = 0;
                            break;
                        }
                        else
                        {
                            prevMatch = index;
                            matches++;
                        }
                        
                    }
                }

                if(1 == matches)
                {
                    if(0 != prevMatch)
                    {
                        strncpy(pDebugObj->argv[0], debugCmdList.cmds[prevMatch], MAX_ARG_BUF_LEN - 1);
                        pDebugObj->argv[0][MAX_ARG_BUF_LEN] = '\0';
                        
                        /* The Length used to match the commands is the minimum
                         * of the length of the command entred and the MAX_ARG_BUF_LEN*/
                        j = strlen(debugCmdList.cmds[prevMatch]) < MAX_ARG_BUF_LEN ?
                            strlen(debugCmdList.cmds[prevMatch]) : MAX_ARG_BUF_LEN;
                    }
                    
                    if (!strncasecmp(pDebugObj->argv[0], "timeoutset", j))
                    {
                        if( pDebugObj->argc != 2)
                        {
                            cmd = TIMEOUTSET;
                            printf("%s",gHelpStrings[cmd]);
                        }
                        else if(pDebugObj->argv[1][1] == 'x' || pDebugObj->argv[1][1] == 'X')
                        {
                            timeout = (ClInt32T)strtol (pDebugObj->argv[1], NULL, 16);
                        }
                        else
                        {
                            timeout = (ClInt32T)strtol (pDebugObj->argv[1], NULL, 10);
                        }

                        if( timeout < 0)
                        {
                            printf("\n\rPlease enter the proper timeout value\r\n");
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                        }
                        else
                        {
                            pDebugObj->timeout = (ClUint32T)timeout;
                            printf("\r\nTimeout has been set successfully\r\n");
                            retCode = CL_OK;
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "timeoutget", j))
                    {
                        if( pDebugObj->argc != 1)
                        {
                            cmd = TIMEOUTGET;
                            printf("%s",gHelpStrings[cmd]);
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                        }
                        else
                        {
                            printf("\r\nTimeout of the debug session is %d milliseconds\r\n",
                                   pDebugObj->timeout);
                            retCode = CL_OK;
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "setc", j))
                    {
                        retCode = enterContext(pDebugObj, pDebugObj->argv[1]);
                    }
                    else if (!strncasecmp(pDebugObj->argv[0],"sleep",j))
                    {
                        if (( pDebugObj->argc != 2 ) || ((sleep_time = atoi(pDebugObj->argv[1])) < 0))
                        {
                            printf("\n%s\n", gHelpStrings[SLEEP]);
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                        }
                        else
                        {
                            sleep(sleep_time);
                            retCode = CL_OK;
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "errno", j))
                    {
                        if( pDebugObj->argc != 1 )
                        {
                            printf("\n%s\n", gHelpStrings[ERRNO]);
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                        }
                        else
                        {
                            printf("%x", clDbgCliErrNo);
                            retCode = CL_OK; 
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "status", j))
                    {
                        if( pDebugObj->argc != 1 )
                        {
                            printf("\n%s\n", gHelpStrings[DBGSTATUS]);
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                        }
                        else
                        {
                            printf("%d", clDbgCliErrNo == CL_OK ? CL_OK: !CL_OK);
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "history", j))
                    {
                        ;           /* history is already handled in dbgCliCommandGets */
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "list", j))
                    {
                        retCode = debugList(pDebugObj);
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "loglevelset", j))
                    {
                        ClLogSeverityT logLevel = CL_LOG_SEV_ERROR;
                        ClBufferHandleT msg = 0;

                        if ((!pDebugObj->context.isNodeAddressValid) ||
                            (pDebugObj->context.isCommPortValid))
                        {
                            printf("\nInvalid context\n"
                                   "Type 'help' or '?'\n");
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_CTX);
                            goto L1;                            
                        }
                        else if(pDebugObj->argc != 2)
                        {
                            cmd = LOGLEVELSET;
                            printf("%s", gHelpStrings[cmd]);
                            printf("%s", gHelpLogSevString);
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                            goto L1;                                                        
                        }
                        else if (!strcasecmp("CL_LOG_SEV_EMERGENCY", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_EMERGENCY;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_ALERT", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_ALERT;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_CRITICAL", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_CRITICAL;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_ERROR", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_ERROR;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_WARNING", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_WARNING;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_NOTICE", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_NOTICE;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_INFO", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_INFO;
                        }
                        else if (!strcasecmp("CL_LOG_SEV_DEBUG", pDebugObj->argv[1]))
                        {
                            logLevel = CL_LOG_SEV_DEBUG;
                        }
                        else
                        {
                            printf("\n\rThe given loglevel is incorrect\n");
                            printf("%s", gHelpLogSevString);                            
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                            goto L1;
                        }

                        rc = clBufferCreate(&msg);
                        rc = clBufferNBytesWrite( msg, (ClUint8T*)&logLevel,
                                                         sizeof(ClLogSeverityT));
                        iocAddress.iocPhyAddress.nodeAddress =
                            pDebugObj->context.nodeAddress;
                        iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

                        rc = clRmdWithMsg( iocAddress,
                                           CL_EO_LOG_LEVEL_SET,
                                           msg, 0,
                                           CL_RMD_CALL_NON_PERSISTENT,
                                           NULL, NULL);
                        retCode = rc;
                        if (CL_OK != rc)
                        {
                            rc = CL_OK;
                            printf("\r\nCould not change log level\n");
                        }
                        else
                        {
                            printf("\r\nloglevel set successfully\n");
                        }
                    }
                    else if (!strncasecmp(pDebugObj->argv[0], "loglevelget", j))
                    {
                        ClLogSeverityT         logLevel = CL_LOG_SEV_ERROR;
                        ClBufferHandleT msg = 0;
                        ClUint32T              i = sizeof(ClLogSeverityT);

                        if ((!pDebugObj->context.isNodeAddressValid) ||
                            (pDebugObj->context.isCommPortValid))
                        {
                            printf("\r\nInvalid context\n"
                                   "Type 'help' or '?' to see valid commands\n");
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_CTX);
                            goto L1;                            
                        }
                        else if(pDebugObj->argc != 1)
                        {
                            printf("\r\nUsage: loglevelget\n");
                            retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                            goto L1;
                        }

                        rc = clBufferCreate(&msg);
                        iocAddress.iocPhyAddress.nodeAddress =
                            pDebugObj->context.nodeAddress;
                        iocAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

                        retCode = clRmdWithMsg( iocAddress,
                                           CL_EO_LOG_LEVEL_GET,
                                           0, msg,
                                           CL_RMD_CALL_NEED_REPLY, NULL, NULL);
                        
                        if (CL_OK != retCode)
                        {
                            rc = CL_OK;
                            printf("\r\nCould not retrieve log level\n");
                        }
                        else
                        {
                            rc = clBufferNBytesRead(msg, (ClUint8T*)&logLevel, &i);

                            clBufferDelete(&msg);

                            switch (logLevel)
                            {
                            case CL_LOG_SEV_EMERGENCY:
                                printf("\nlog level is CL_LOG_SEV_EMERGENCY\n");
                                break;
                            
                            case CL_LOG_SEV_ALERT:
                                printf("\nlog level is CL_LOG_SEV_ALERT\n");
                                break;
                            
                            case CL_LOG_SEV_CRITICAL:
                                printf("\nlog level is CL_LOG_SEV_CRITICAL\n");
                                break;
                            
                            case CL_LOG_SEV_ERROR:
                                printf("\nlog level is CL_LOG_SEV_ERROR\n");
                                break;
                            
                            case CL_LOG_SEV_WARNING:
                                printf("\nlog level is CL_LOG_SEV_WARNING\n");
                                break;
                            
                            case CL_LOG_SEV_NOTICE:
                                printf("\nlog level is CL_LOG_SEV_NOTICE\n");
                                break;
                            
                            case CL_LOG_SEV_INFO:
                                printf("\nlog level is CL_LOG_SEV_INFO\n");
                                break;
                            
                            case CL_LOG_SEV_DEBUG:
                                printf("\nlog level is CL_LOG_SEV_DEBUG\n");
                                break;
                            
                            default:
                                printf("\nlog level is unrecognized\n");
                                retCode = CL_DEBUG_RC(CL_DBG_ERR_INVALID_PARAM);
                                break;
                            };
                        }
                    }
                    else if ((pDebugObj->context.isNodeAddressValid) &&
                             (pDebugObj->context.isCommPortValid))
                    {
                       ClInt32T tries = 0;
                       ClTimerTimeOutT delay = {.tsSec = 2, .tsMilliSec = 0};
                       do
                       {
                           retCode = invoke(pDebugObj, pDebugObj->argc, pDebugObj->argv, &retStr);
                           if(CL_GET_ERROR_CODE(retCode) == CL_ERR_TRY_AGAIN)
                           {
                               if(retStr) 
                               {
                                   clHeapFree(retStr);
                                   retStr = NULL;
                               }
                           }
                           else break;
                       } while(++tries < 10 && clOsalTaskDelay(delay) == CL_OK);

                        if (NULL != retStr)
                        {
                            if(0 != strcmp("", retStr))
                            {
                                if('\n' != retStr[0])
                                {
                                    printf("\n");
                                }

                                printf("%s", retStr);

                                if('\n' != retStr[strlen(retStr)-1])
                                {
                                    printf("\n");
                                }
                            }
                            clHeapFree(retStr);
                            retStr = NULL;
                        }
                    }
                }
                else if(matches > 1)
                {
                    printf("Ambiguous command. Matching commands found.\n");
                    /* Uses the same index as the outer loop. So when this ends
                     * the outer loop automatically ends. */
                    for(index = 0; index < debugCmdList.cmdNum; index++)
                    {
                        if(!strncasecmp(pDebugObj->argv[0], debugCmdList.cmds[index], j))
                        {
                            printf("%s\n", debugCmdList.cmds[index]);
                        }
                    }
                }
                else
                {
                    printf("\r\n Command unrecognized\n"
                           "Type 'help' or '?' to get commands\n");
                    retCode = CL_DEBUG_RC(CL_DBG_ERR_UNRECOGNIZED_CMD);
                }
            }
        }
        
        
    L1: buf[0] = 0;
        clearArgBuf(&(pDebugObj->argc), pDebugObj->argv);
        clDbgCliErrNo = retCode;
    }
    
    printf("\nGoodbye!!\n");
    return 0;
}


static ClRcT argCompletion(ClDebugCliT* pDebugObj,
                           ClCharT *ptrPrompt,
                           ClCharT *cmd,
                           ClInt32T argLen,
                           ClInt32T cmdLen,
                           ClUint32T *len)
{
    ClRcT     rc = CL_OK;
    ClCharT   *arg = NULL;
    ClCharT   **argList = NULL;
    ClCharT   *retStr = NULL;
    ClCharT   *temp = NULL;
    ClUint32T num = 0;
    ClUint32T i = 0;
    ClUint32T match = 0;
    ClUint32T matches = 0;
    ClUint32T ind = 0;
    ClUint32T index = 0;
    ClCharT   *cpm = "cpm";

    if(argLen <= 0)
    {
        CL_ASSERT(0);
    }

    arg = clHeapAllocate(argLen + 1);
    if(NULL == arg)
    {
        return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
    }
    arg[argLen] = '\0';
    strncpy(arg, &pDebugObj->dbgCliBuf[cmdLen + 1], argLen);

    if ( !strncmp(cmd, "setc", cmdLen)
            && pDebugObj->context.isNodeAddressValid
            && !pDebugObj->context.isCommPortValid)
    {
        rc = clCpmComponentListDebugAll( pDebugObj->context.nodeAddress,
                &retStr);
        if (NULL != retStr)
        {
            temp = retStr;
            i = 0;
            num = 0;
            while (retStr[i] != '\0')
            {
                if(retStr[i] == '\n')
                {
                    num++;
                }
                i++;
            }
            argList = clHeapAllocate(sizeof(ClCharT *) *(num + 1));
            i = strlen (cpm);
            argList[ind] = clHeapAllocate(i + 1);
            strncpy(argList[ind], cpm, i);
            argList[ind][i] = '\0';
            ind++;
            i = 0;
            while (retStr[i] != '\0')
            {
                while (retStr[i] != '\n')
                {
                    i++;
                }
                argList[ind] = clHeapAllocate(i + 1);
                strncpy(argList[ind], retStr, i);
                argList[ind][i] = '\0';
                retStr = retStr + i + 1;
                i = 0;
                ind++;
            }
        }
        clHeapFree(temp);
    }
    else if (!strncmp(cmd, "help", cmdLen))
    {
        ind = debugCmdList.cmdNum;
        argList = clHeapAllocate(sizeof(ClCharT*) * debugCmdList.cmdNum);
        for(i = 0; i < debugCmdList.cmdNum; i++)
        {
            argList[i] = clHeapAllocate(strlen(debugCmdList.cmds[i]) + 1);
            strncpy(argList[i], debugCmdList.cmds[i], strlen(debugCmdList.cmds[i]));
        }
    }
    else
    {
        strncpy(&pDebugObj->dbgCliBuf[0], cmd, strlen(cmd));
        *len = 4;
        pDebugObj->dbgCliBuf[*len] = ' ';
        *len = *len + 1;
        strncpy(&pDebugObj->dbgCliBuf[*len], arg, argLen);
        *len = *len + argLen;
        pDebugObj->dbgCliBuf[*len] = '\0';
        clHeapFree(arg);
        return rc;
    }

    for(index = 0; index < ind; index++)
    {
        if(!strncasecmp(arg, argList[index], argLen))
        {
            if(!matches)
            {
                matches++;
                match = index;
            }
            else
            {
                printf("\nMultiple completions found.\n");
                matches++;

                /* Uses the same index as the outer loop. So when this ends
                 * the outer loop automatically ends. */
                for(index = 0; index < ind; index++)
                {
                    if(!strncasecmp(arg, argList[index], argLen))
                    {
                        printf("%s\n", argList[index]);

                    }
                }
            }
        }
    }

    if (matches == 1)
    {
        strncpy ( &pDebugObj->dbgCliBuf[0], cmd, strlen (cmd));
        pDebugObj->dbgCliBuf[strlen(cmd)] = ' ';
        strncpy ( &pDebugObj->dbgCliBuf[strlen(cmd) + 1], 
                argList[match],
                strlen(argList[match]));
        *len = strlen(cmd) + strlen(argList[match]) + 1;
    }
    else if (matches > 1)
    {
        strcpy(&pDebugObj->dbgCliBuf[0], cmd);
        *len = strlen(cmd);
        pDebugObj->dbgCliBuf[*len] = ' ';
        (*len)++;
        strncpy(&pDebugObj->dbgCliBuf[*len], arg, argLen);
        *len += argLen;
    }

    for(i = 0; i < ind; i++)
    {
        clHeapFree(argList[i]);
    }
    clHeapFree(argList);

    clHeapFree(arg);

    return rc;

}
    

static ClRcT cmdCompletion(ClDebugCliT* pDebugObj, ClCharT *ptrPrompt)
{
    ClUint32T   i            = 0;
    ClUint32T   j            = 0;
    ClUint32T   len          = 0;
    ClUint32T   size         = 0;
    ClUint32T   tmp          = 0;
    ClUint32T   rcvLen       = 0;
    ClInt32T    argLen       = 0;
    ClUint32T   partialMatch = 0;
    ClCharT   **helpList     = NULL;
    ClUint32T   helpListSize = 0;
    ClUint32T   foundIdx     = 0;
    ClInt32T    cmdLen       = 0;
    ClCharT    *arg          = NULL;

    ClDebugCompContextT context  = {0};
    ClRcT               cmdFound = CL_DEBUG_RC(CL_ERR_NOT_EXIST);
    ClUint32T           lastIdx  = 0;
    ClRcT               rc       = CL_OK;

    /*create the matchable command list*/
    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            ClUint32T j = 0;

            rc = getCompContext( pDebugObj->context.nodeAddress,
                                 pDebugObj->context.commPort, &context);
            if (CL_OK != rc)
            {
                return rc;
            }

            helpList = (ClCharT**) clHeapAllocate(sizeof(ClCharT*) *
                                                (GET_ARRAY_SIZE(helpCompLevel)
                                                 + context.numList - 2));
            if (NULL == helpList)
            {
                finCompContext(&context);
                return CL_DEBUG_RC(CL_ERR_NO_MEMORY);
            }
            helpListSize = GET_ARRAY_SIZE(helpCompLevel) + context.numList
                - 2;

            for (i = 2; i < (context.numList); i++, j++)
            {
                size = strlen(context.list[i]);
                helpList[j] = (ClCharT *) clHeapAllocate(sizeof(ClCharT) * (size + 1));
                helpList[j][size] = '\0';
                if(NULL == helpList[j])
                {
                    CL_ASSERT(0);
                }
                strncpy(helpList[j], context.list[i],size);
            }

            for (i = 0; i < GET_ARRAY_SIZE(helpCompLevel); i++, j++)
            {
                size = strlen(helpCompLevel[i]);
                helpList[j] = (ClCharT *) clHeapAllocate(sizeof(ClCharT) * (size + 1));
                helpList[j][size] = '\0';
                strncpy(helpList[j], helpCompLevel[i],size);
            }
            finCompContext(&context);
        }
        else
        {
            helpList     = helpNodeLevel;
            helpListSize = GET_ARRAY_SIZE(helpNodeLevel);
        }
    }
    else
    {
        helpList     = helpGeneric;
        helpListSize = GET_ARRAY_SIZE(helpGeneric);
    }

    /*get the length upto, after which came '\t'*/
    for (i = 0; i < CLI_CMD_SZ; i++)
    {
        if ( ((pDebugObj->dbgCliBuf[i] == 0x00) || 
              (pDebugObj->dbgCliBuf[i] == 0x20)))
        {
            cmdLen = i;
            break;
        }
    }
    /* cmdLen is the length of the command.
     * rcvLen is the length of the recieved input. */
    for (i = 0; i < (CLI_CMD_SZ - 1); i++)
    {
        if ( ((pDebugObj->dbgCliBuf[i] == 0x00) || 
              (pDebugObj->dbgCliBuf[i] == 0x20)) &&
             ((pDebugObj->dbgCliBuf[i+1] == 0x00) || 
              (pDebugObj->dbgCliBuf[i+1] == 0x20)))
        {
            rcvLen = i;
            break;
        }
    }

    argLen = rcvLen - cmdLen - 1;
    if (argLen > 0)
    {
        arg = clHeapAllocate(argLen + 1);
        if(NULL == arg)
        {
            rc = CL_DEBUG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("CMD",CL_LOG_CONTEXT_UNSPECIFIED,
                    "Failed to allocate memory [0x %x]", rc);
            return rc;
        }
        arg[argLen] = '\0';
        strncpy(arg, &pDebugObj->dbgCliBuf[cmdLen + 1], argLen);
    }
        
    /*command completion!!*/
    for (i = 0; i < helpListSize; i += 2)
    {
        if (i >= helpListSize)
        {
            i = helpListSize - 1;
        }
        if (strncasecmp(pDebugObj->dbgCliBuf, helpList[i], cmdLen) == 0)
        {
            if (foundIdx != 0)
            {
                if (foundIdx == 1)
                {
                    printf("\nAmbiguous command! Matching commands found:");
                    fflush(stdout);
                }
                printf("\n%s", helpList[lastIdx]);
                fflush(stdout);
                len = strlen(helpList[lastIdx]);
                tmp = strncasecmp(helpList[lastIdx],
                                  helpList[i],
                                  partialMatch);
                if (tmp != 0)
                {
                    for (j = cmdLen; j < len; j++)
                    {
                        if (helpList[lastIdx][j] !=
                            helpList[i][j])
                        {
                            partialMatch = j;
                            break;
                        }
                    }
                }
                lastIdx = i;
                foundIdx++;                  
            }
            else
            {
                partialMatch = strlen(helpList[i]);
                lastIdx = i;
                foundIdx++; 
            }
        }
    }
    if ((foundIdx != 1) && (foundIdx != 0))
    {
        printf("\n\r%s\n\r", helpList[lastIdx]);
        fflush(stdout);
        printPrompt(ptrPrompt);
        fflush(stdout);
        printf (" ");
        fflush(stdout);
        strncpy ( &pDebugObj->dbgCliBuf[0], 
                  helpList[lastIdx],
                  partialMatch);
        pDebugObj->dbgCliBuf[len] = '\0';
        for (i = 0; i < partialMatch; i++)
        {
            printf("%c",pDebugObj->dbgCliBuf[i]);
            fflush(stdout);
        }
        pDebugObj->dbgCliBuf[partialMatch] = ' ';
        pDebugObj->dbgCliBuf[partialMatch + 1] = ' ';
    }
    else
    {
        if (foundIdx == 1)
        {
            cmdFound = CL_OK;
            len = strlen(helpList[lastIdx]);

            if(!strncasecmp(helpList[lastIdx], "setc", len) && argLen > 0)
            {
                argCompletion(pDebugObj, ptrPrompt, "setc", argLen, cmdLen, &len);
            }
            else if(!strncasecmp(helpList[lastIdx], "help", len) && argLen > 0)
            {
                argCompletion(pDebugObj, ptrPrompt, "help", argLen, cmdLen, &len);
            }
            else
            {
                strncpy ( &pDebugObj->dbgCliBuf[0], 
                          helpList[lastIdx],
                          len);
                if(argLen > 0)
                {
                    pDebugObj->dbgCliBuf[len] = ' ';
                    len++;
                    strncpy ( &pDebugObj->dbgCliBuf[len], 
                              arg,
                              argLen);
                    len += argLen;
                }
            }
            pDebugObj->dbgCliBuf[len] = '\0';
            printf("\r");
            fflush(stdout); 
            printPrompt(ptrPrompt);
            fflush(stdout);
            printf(" ");
            fflush(stdout);
            for (i = 0; i < len; i++) 
            {                             
                printf("%c", pDebugObj->dbgCliBuf[i]);
                fflush(stdout);
            }
        } 
    }

    /*delete matchable command list - if necessary*/
    if (pDebugObj->context.isNodeAddressValid)
    {
        if (pDebugObj->context.isCommPortValid)
        {
            for(i = 0; i < helpListSize - 1; i++)
            {
                if(NULL != helpList[i])
                {
                    clHeapFree(helpList[i]);
                }
            }
            if(NULL != helpList[i])
            {
                clHeapFree(helpList);
            }
        }
    }

    if (argLen > 0)
    {
        clHeapFree(arg);
    }

    return cmdFound;
}

SaAmfHandleT  amfHandle;
ClCpmHandleT  cpmHandle;
void appTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    

    saAmfComponentUnregister(amfHandle, compName, NULL);
    saAmfFinalize(amfHandle);
    shouldIUnblock = 1;
    
}



static ClRcT appInitialize( ClUint32T argc, ClCharT* argv[])
{
    ClRcT               rc = CL_OK;
    ClDebugCliT         *debugObj;
    SaNameT	    	appName;
    ClIocPortT  	iocPort;
    SaAmfCallbacksT     callbacks;
    SaVersionT          version;
    struct sigaction    sigNewVar;

    shouldIUnblock = 0;

    version.releaseCode  = 'B';
    version.majorVersion = 01;
    version.minorVersion = 01;
    
    callbacks.saAmfHealthcheckCallback          = NULL; 
    callbacks.saAmfComponentTerminateCallback   = appTerminate;
    callbacks.saAmfCSISetCallback               = NULL;
    callbacks.saAmfCSIRemoveCallback            = NULL;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
    callbacks.saAmfProxiedComponentInstantiateCallback = NULL;
    callbacks.saAmfProxiedComponentCleanupCallback = NULL;

    clEoMyEoIocPortGet(&iocPort);
    saAmfInitialize(&amfHandle, &callbacks, &version);


    memset(&gDbgInfo,'\0',sizeof(gDbgInfo));

    rc = debugCliInitialize(&debugObj, "Test");
    if (CL_OK != rc)
    {
        return rc;
    }

    pGDebugObj = debugObj;

    
    saAmfComponentNameGet(amfHandle, &appName);
    saAmfComponentRegister(amfHandle, &appName, NULL);
    /* register signal to handle the Ctrl-C*/
    memset(&sigNewVar,'\0',sizeof(sigNewVar));
    sigNewVar.sa_handler = dbgSignalHandler;
    sigaction(SIGINT,&sigNewVar,NULL);
    
   /* This is a blocking function and will exit only when user type 
       BYE or CPM calls terminate*/


    debugCliShell(debugObj);

     /* This shall execute only in case of when the bye is executed */
    
    if(shouldIUnblock != 1)
    {
        SaInvocationT invocation = 0;

        rc = debugCliFinalize(&debugObj);
        saAmfComponentNameGet(0, &appName);

        /*
         * Cleanup EO resources.
         */
       appTerminate(invocation, &appName);
    }
    
    return (CL_OK);
}



static ClRcT   appStateChange(ClEoStateT eoState)
{
    return CL_OK;
}


static ClRcT   appHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    schFeedback->freq   = CL_EO_DEFAULT_POLL;
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;
}

void dispatchLoop(void);

int errorExit(SaAisErrorT rc);

ClEoConfigT clEoConfig = {
    1,              /* Thread Priority */
    1,              /* 1 listener thread */
    0,              /* Assign port dynamically*/
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_APP, /* use thread for Cli; */
    NULL,
    NULL,
    appStateChange,
    appHealthCheck,
    NULL
};

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,			/* osal */
    CL_TRUE,			/* timer */
    CL_TRUE,			/* buffer */
    CL_TRUE,			/* ioc */
    CL_TRUE,			/* rmd */
    CL_TRUE,			/* eo */
    CL_FALSE,			/* om */
    CL_FALSE,			/* hal */
    CL_FALSE,			/* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_FALSE,			/* cor */
    CL_FALSE,			/* cm */
    CL_FALSE,			/* name */
    CL_FALSE,			/* log */
    CL_FALSE,			/* trace */
    CL_FALSE,			/* diag */
    CL_FALSE,			/* txn */
    CL_FALSE,			/* hpi */
    CL_FALSE,			/* cli */
    CL_FALSE,			/* alarm */
    CL_TRUE,			/* debug */
    CL_FALSE,			/* gms */
    CL_FALSE,           /* pm */
};

ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

    clLogCompName = "CLI"; /* Override generated eo name with a short name for our server */
    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);
    
    rc = appInitialize(argc, argv);
    
    if(rc != CL_OK)
    {
       
           exit(0);
    }
 
    dispatchLoop();
  
    return 0;
}

void dispatchLoop(void)
{        
    SaAisErrorT         rc = SA_AIS_OK;
    SaSelectionObjectT amf_dispatch_fd;
    int maxFd;
    fd_set read_fds;

    /*
     * Get the AMF dispatch FD for the callbacks
    */
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
       errorExit(rc);
     
    maxFd = amf_dispatch_fd;  
    do
    {
        FD_ZERO(&read_fds);
        FD_SET(amf_dispatch_fd, &read_fds);
            
        if( select(maxFd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
           char errorStr[80];
           int err = errno;
           if (EINTR == err) continue;

           errorStr[0] = 0; /* just in case strerror does not fill it in */
           strerror_r(err, errorStr, 79);
          
           break;
        }
        if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
     
    }while(!shouldIUnblock);      
}

int errorExit(SaAisErrorT rc)
{        
    
    exit(-1);
    return -1;
}

