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
/*******************************************************************************
* ModuleName  : debug
* File        : clDebugApi.h
*******************************************************************************/

/*******************************************************************************
* Description :
*   This This module contains common DEBUG definitions
******************************************************************************/

/**
 *  \file
 *  \brief Header file of Debug Service Related APIs
 *  \ingroup debug_apis
 */

/**
 ************************************
 *  \addtogroup debug_apis
 *  \{
 */

#ifndef _CL_DEBUG_API_H_
#define _CL_DEBUG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clDebugErrors.h>
#include <clHandleApi.h>
#include <clLogApi.h>
#include <clRmdIpi.h>
#ifndef __KERNEL__
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#endif

#ifdef SOLARIS_BUILD
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>
int  backtrace(void **buffer, int count);
char **backtrace_symbols(void *const *array, int size);
void backtrace_symbols_fd(void *const *array, int size, int fd);
#endif /* SOLARIS_BUILD */

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/** These defines are aligned with the logging defined in clLogApi.h
    and will ultimately be deprecated as logs are converted over to 
    the new format
*/

/**
 * This debug level in \c CL_DEBUG_PRINT is used to report a critical error.
 */
#define CL_DEBUG_CRITICAL 3

/**
 * This debug level in \c CL_DEBUG_PRINT is used to report a non-critical error.
 */
#define CL_DEBUG_ERROR    4

/**
 * This debug level in \c CL_DEBUG_PRINT is used to report a warning.
 */
#define CL_DEBUG_WARN     5

/**
 * This debug level in \c CL_DEBUG_PRINT is used to provide information.
 */
#define CL_DEBUG_INFO     7

/**
 * This debug level in \c CL_DEBUG_PRINT is used to add trace information.
 */
#define CL_DEBUG_TRACE    0xc

/**
 * The value of CL_DEBUG_LEVEL_THRESHOLD can be changed (either here,
 * or by some file that includes this file) to make the debug output
 * either more or less voluminous
 */
#if !defined(CL_DEBUG_LEVEL_THRESHOLD)
#define CL_DEBUG_LEVEL_THRESHOLD CL_DEBUG_ERROR
#endif

/**
 * The macro \c CL_DEBUG_PRINT is used to print messages to the user.
   It logs other useful information about the print such as file,
   function number and line number from where the message originated.
   It is also used to log messages in kernel mode.
 */
#ifndef CL_DEBUG
#define CL_DEBUG
#endif

#ifdef CL_DEBUG
#ifndef __KERNEL__
#include <clEoApi.h>
#include <clDbg.h>

#define CL_DEBUG_SP(...) __VA_ARGS__

#define clCompStatLog(...) clLog(CL_LOG_SEV_DEBUG,"COMP","STAT", __VA_ARGS__)

    

enum
  {
    clDebugTimeStrMaxLen = 128
  };

#define CL_DEBUG_PRINT(x,y) \
    do \
    { \
        char __str[256]; \
        if(x <= CL_DEBUG_LEVEL_THRESHOLD) \
        {\
            snprintf(__str,256,CL_DEBUG_SP y); \
            clLog((ClLogSeverityT)x, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
                  __str);\
        }\
           if (1) \
          { \
            snprintf(__str,256,CL_DEBUG_SP y); \
            clDbgMsg((int)getpid(),__FILE__, __LINE__, __FUNCTION__,x,__str); \
          } \
    }while (0)

#define CL_DEBUG_PRINT_CONSOLE(x,y)                                     \
    do                                                                  \
    {                                                                   \
        char __str[256];                                                \
        if(x <= CL_DEBUG_LEVEL_THRESHOLD)                               \
        {                                                               \
            snprintf(__str,256,CL_DEBUG_SP y);                          \
            clLogConsole((ClLogSeverityT)x, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, \
                  __str);                                               \
        }                                                               \
        if (1)                                                          \
        {                                                               \
            snprintf(__str,256,CL_DEBUG_SP y);                          \
            clDbgMsg((int)getpid(),__FILE__, __LINE__, __FUNCTION__,x,__str); \
        }                                                               \
    }while (0)

#else /* __KERNEL__ */

#define CL_DEBUG_PRINT(x,y) \
    do \
    {  \
        if(x <= CL_DEBUG_LEVEL_THRESHOLD) \
        {\
            printk("file:%s,func:%s,line:%d: ", __FILE__, __FUNCTION__, __LINE__);\
            printk y; \
        }\
    }while (0)

#define CL_DEBUG_PRINT_CONSOLE CL_DEBUG_PRINT

#endif

#else /* CL_DEBUG */
#define CL_DEBUG_PRINT(x,y)
#define CL_DEBUG_PRINT_CONSOLE(x, y)
#endif

/**
 * This macro is called while entering the function.
 */
#define CL_FUNC_ENTER()


/**
 * This macro is called while exiting the function.
 */
#define CL_FUNC_EXIT()


/**
 * This macro is used to assert upon a critical failure.
 */
#ifndef __KERNEL__
#include <assert.h>
#define CL_ASSERT(expr) do { if (clDbgPauseOnCodeError&&(!(expr))) clDbgCodeError(0,("Assertion failed")); else assert((expr)); } while(0)   
#endif /* __KERNEL__ */

struct clEoExecutionObj;

/**
 * The maximum length of the prompt for the component to be displayed in the debug CLI.
 */
#define CL_DEBUG_COMP_PROMPT_LEN   15


/**
 * The maximum length of the name of a component in the debug CLI.
 */
#define CL_DEBUG_COMP_NAME_LEN   128


/**
 * The maximum length of a command of the component in the debug CLI.
 */
#define CL_DEBUG_FUNC_NAME_LEN    41

/**
 * The maximum length of the help for a command of the component in the debug CLI.
 */
#define CL_DEBUG_FUNC_HELP_LEN    201

/**
 * This is the signature of the callback invoked by the debug CLI in response to a
 * command entered. A function of this signature has to be registered by the
 * component with the debug CLI for every command.
 */
typedef ClRcT (*ClDebugCallbackT) (

/**
 * Number of arguments passed with the command.
 */
    ClUint32T argc,

/**
 * An array of the arguments passed with the command.
 */
    ClCharT** argv,

/**
 * Placeholder for a return string to be allocated by the function and displayed on the CLI.
 */
    ClCharT** ret);

/**
 * The structure contains the entry for the debug CLI information that needs the component has to provide on a per command basis.
 */
typedef struct ClDebugFuncEntryT
{
/**
 * Function to be invoked upon a command entered on the CLI.
 */
    ClDebugCallbackT  fpCallback;

/**
 * Name of the command used while invoking the command.
 */
    ClCharT           funcName[CL_DEBUG_FUNC_NAME_LEN];

/**
 * One line help for the command.
 */
    ClCharT           funcHelp[CL_DEBUG_FUNC_HELP_LEN];
} ClDebugFuncEntryT;

/**
 * This structure is used to register the module with the CLI library.
 */
typedef struct ClDebugModEntryT {

/**
 * Module name.
*/
    char modName[80];

/**
 * Module prompt.
 */
    char modPrompt[20];

/**
 * List of commands and their functions.
 */
    ClDebugFuncEntryT *cmdList;
    char help[80];
} ClDebugModEntryT;

/**
 * The type of the handle clDebugPrint APIs.
 */
typedef ClPtrT ClDebugPrintHandleT;

/*****************************************************************************
 *  Debug APIs
 *****************************************************************************/

/**
 ************************************
 *  \brief Invokes the library based local debug CLI.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param nprompt String which is incorporated into the debug CLI prompt.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to invoke the library based local debug CLI. It will continue to
 *  execute till exited from the console. This function must be called from a separate thread.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugLibInitialize(), clDebugLibFinalize()
 */

ClRcT clDebugCli(
   CL_IN ClCharT *nprompt /*this string is incorporated into the debug CLI prompt*/);


/**
 ************************************
 *  \brief Initializes the Debug CLI library.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \par Parameters:
 *  None.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On failure to allocate memory.
 *
 *  \par Description:
 *  This function is used as the library initialization routine for debug CLI.
 *  It must be called before executing any other debug functions.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugLibFinalize()
 */

ClRcT clDebugLibInitialize(void);



/**
 ************************************
 *  \brief Finalizes the Debug CLI library.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \par Parameters:
 *  None.
 *
 *  \retval CL_OK The API executed successfully. And return values
 *   passed by EO APIs on an error.
 *
 *  \par Description:
 *  This function is used as the library finalization routine for debug CLI. It is
 *  the last function called in order to do the necessary cleanup.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugLibInitialize()
 */

ClRcT clDebugLibFinalize(void);

/**
 ************************************
 *  \brief Sets the name and prompt of the component.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param pCompPrompt Prompt for the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NULL_PARAMETER On passing NULL as parameter.
 *
 *  \par Description:
 *  This function is used to set the prompt for the Component. If the user
 *  doesn't set any prompt, the default prompt will be displayed. The default
 *  prompt is "DEFAULT".
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \par Related Function(s):
 *          None
 *
 */
 ClRcT clDebugPromptSet(CL_IN  const ClCharT  *pCompPrompt);

/**
 ************************************
 *  \brief Registers the component name.
 *
 *  \param funcArray List of commands and their respective functions and help.
 *  \param funcArrayLen Length of the function array.
 *  \param phDebugReg  This is address where handle for registration will be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NO_MEMORY On failure to allocate memory.
 *  \retval CL_ERR_DUPLICATE When you register same command more than once.
 *
 *  \par Description:
 *  This function is used to register the list of all the CLI functions
 *  for the component with the EO. It is invoked before the debug server CLI can be used.
 *  The server debug CLI uses information registered by this function.
 *  If you register the same command again, DUPLICATE error will be returned.
 *
 *  \sa clDebugDeregister()
 *
 */

ClRcT clDebugRegister(CL_IN  ClDebugFuncEntryT  *funcArray,
                      CL_IN  ClUint32T          funcArrayLen, 
                      CL_OUT ClHandleT          *phDebugReg);



/**
 ************************************
 *  \brief De-registers the debug CLI information from the EO.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *
 *  \param  hReg Handle for the commandGroup which has to deregistered.
 *                This handle is returned by clDebugRegister.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Handle.
 *
 *  \par Description:
 *  This function is used to de-register the debug CLI information from a given EO.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugRegister()
 *
 */

ClRcT clDebugDeregister(CL_IN  ClHandleT  hReg);

/**
 ************************************
 *  \brief Retrieve a handle for printing.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param msg Handle for printing.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On failure to allocate memory.
 *   And other errors returned by EO APIs.
 *
 *  \par Description:
 *  This function is used to retrieve a handle for printing.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugPrintFinalize(), clDebugPrint()
 *
 */

ClRcT clDebugPrintInitialize(CL_OUT ClDebugPrintHandleT* msg /*handle for printing*/);


/**
 ************************************
 *  \brief Prints a string into the handle.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param msg Handle for printing.
 *  \param fmtStr Format string for the variadic arguments.
 *  \param vargs Variadic arguments.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to print a string with a maximum of 512 bytes into the handle at any given
 *  point in time time. This function can be invoked 'n' number of times after invoking \e clDebugPrintInitialize
 *  function, before calling either \e clDebugPrintFinalize or \e clDebugPrintDestroy functions.
 *
 *  \par Library File:
 *  libClDebugClient.a
 *
 *  \sa clDebugPrintInitialize(), clDebugPrintFinalize()
 *
 */

ClRcT clDebugPrint(CL_INOUT ClDebugPrintHandleT msg,
                   CL_IN    const char* fmtStr, 
                   ...) CL_PRINTF_FORMAT(2, 3);


/**
 ************************************
 *  \brief Cleans up the print handle.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param msg Handle for printing.
 *  \param buf Buffer containing the print message.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On failure to allocate memory.
 *
 *  \par Description:
 *  This function is used to clean up the print handle. It returns a memory allocated buffer
 *  containing the print messages logged into the handle.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugPrintInitialize()
 *
 */

ClRcT clDebugPrintFinalize(CL_IN ClDebugPrintHandleT* msg,
                           CL_OUT char** buf);


/**
 ************************************
 *  \brief Frees the print handle.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param msg Handle for printing.
 *  \param buf Buffer containing the print message.
 *
 *  \retval CL_OK The API executed successfully. It returns all error codes
 *   returned by clBufferDelete().
 *
 *  \par Description:
 *  This function is used to free the handle without returning any message buffer.
 *
 *  \par Library File:
 *  libClDebugClient.a
 *
 *  \sa clDebugPrintInitialize()
 */

ClRcT clDebugPrintDestroy(CL_INOUT ClDebugPrintHandleT* msg);

/**
 ************************************
 *  \brief Check the given version is supported or not.
 *
 *  \par Header File:
 *  clDebugApi.h
 *
 *  \param pVersion the version.
 *
 *  \retval CL_OK The API executed successfully. It returns all error codes
 *   returned by clVersionVerify().
 *
 *  \par Description:
 *  This function is used to check the given version is compatible or not.
 *  If not, will return the compatibl version with error.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \sa clDebugLibInitialize(), clDebugLibFinalize()
 */

ClRcT clDebugVersionCheck(CL_INOUT ClVersionT *pVersion);

ClRcT clDebugResponseSend(ClRmdResponseContextHandleT responseHandle,
                          ClBufferHandleT *pOutMsgHandle,
                          ClCharT *respBuffer,
                          ClRcT retCode);
ClRcT clDebugResponseDefer(ClRmdResponseContextHandleT *pResponseHandle, ClBufferHandleT *pOutMsgHandle);

ClRcT
clDebugPrintExtended(ClCharT **retstr, ClInt32T *maxBytes, ClInt32T *curBytes, 
                     const ClCharT *format, ...) CL_PRINTF_FORMAT(4, 5);

#ifdef __cplusplus
}
#endif

#endif /* _CL_DEBUG_API_H_ */

/**
 *  \}
 */

