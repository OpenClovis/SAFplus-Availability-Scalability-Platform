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
* File        : clDbg.h
*******************************************************************************/
 /*******************************************************************************
* Description :
*   This module contains functions that the app or infrastructure writer can call 
* to help debug his code.
 *******************************************************************************/

#ifndef _CL_DBG_H_
#define _CL_DBG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* See clDbg.c for documentation on these global variables */
extern int clDbgPauseOn;
extern int clDbgPauseOnCodeError;
extern int clDbgNoKillComponents;
extern int clDbgCompTimeoutOverride;
extern int clDbgLogLevel;             
extern int clDbgResourceLogLevel;
extern int clDbgReverseTiming;  


#define clDbgIfNullReturn(ptr,comp) if ( ptr == NULL) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL passed to function [%s] in parameter [" #ptr "].",__FUNCTION__)); \
        clDbgCodeError(CL_RC(comp,CL_ERR_NULL_POINTER),("NULL passed to function [%s] in parameter [" #ptr "].",__FUNCTION__)); \
        return CL_RC(comp,CL_ERR_NULL_POINTER); \
    }

#define clDbgCheckNull(ptr,comp) if ( ptr == NULL) \
    { \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL passed to function [%s] in parameter [" #ptr "].",__FUNCTION__)); \
        clDbgCodeError(CL_RC(comp,CL_ERR_NULL_POINTER),("NULL passed to function [%s] in parameter [" #ptr "].",__FUNCTION__)); \
    }

void clDbgInitialize(void);

/**
 ************************************
 *  \page clDbgPause
 *
 *  \par Synopsis:
 *  Stop the thread so the user can attach to it via a debugger.
 *
 *  \par Header File:
 *  clDbg.h
 *
 *  \par Syntax:
 *  \code 	 void clDebugPause();                          
 *  \endcode
 *
 *  \par Description:
 *  Call this function to pause this thread.  Then attach via a debugger 
 *  and execute clDebugResume (in gdb "call clDebugResume()") to start the
 *  thread up again.  Useful when you don't have a chance to set a breakpoint.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \par Related Function(s):
 *   \ref "clDebugResume"
 */
#define clDbgPause() clDbgPauseFn(__FILE__,__LINE__)
void clDbgPauseFn(const char* file, int line);

/**
 ************************************
 *  \page clDbgResume
 *
 *  \par Synopsis:
 *  Continue a paused thread -- do not call in code!
 *
 *  \par Header File:
 *  clDbg.h
 *
 *  \par Syntax:
 *  \code 	 void clDebugPause();                          
 *  \endcode
 *
 *  \par Description:
 *  Call this function from the debugger to resume a paused thread this thread. 
 *  Useful when you don't have a chance to set a breakpoint.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \par Related Function(s):
 *   \ref "clDebugResume"
 */
void clDbgResume();

void clDbgMsg(int pid, const char* file, int line, const char* fn, int level, const char* str);

/**
 ************************************
 *  \page clDbgCodeError
 *
 *  \par Synopsis:
 *  Call when a logic or otherwise unhandleable error is detected in the code
 *
 *  \par Header File:
 *  clDbg.h
 *
 *  \par Syntax:
 *  \code 	 clDbgCodeError(clovis_error, ("printf style string %s", and_args) )                          
 *  \endcode
 *
 *  \par Description:
 *  Call this function when there a logic or otherwise unhandleable error is detected in the code.
 *  For example, if a passed parameter is invalid or if a function call is called before its 'library' is initialized.
 *
 *  The first parameter is the clovis error of type ClRcT.  Pass CL_OK if the code can gracefully handle the problem.  
 *  For example, calling an "initialize" function twice could be handled gracefully -- the second call will just be skipped.
 *
 *  The second parameter is a printf style parameter list to tell the user what's going on, and WHY the problem occurred.  
 *
 *  \par Library File:
 *   libClUtils.a
 *
 *  \par Related Function(s):
 *
 */
#define CL_DEBUG_CODE_ERROR clDbgCodeError

#define clDbgCodeError(clErr, printfParams) do { CL_DEBUG_PRINT_CONSOLE(CL_DEBUG_CRITICAL, printfParams); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

  /* A clDbgCodeError is also a root cause error, so you don't have to call both functions */
#define clDbgRootCauseError(clErr, printfParams) do { CL_DEBUG_PRINT_CONSOLE(CL_DEBUG_CRITICAL, printfParams); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

#define clDbgNotImplemented(printfParams) do { CL_DEBUG_PRINT_CONSOLE(CL_DEBUG_CRITICAL, printfParams); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

#define clDbgCheck(predicate, todo, printfParams) do { int result = predicate; if (!result) { CL_DEBUG_PRINT_CONSOLE(CL_DEBUG_CRITICAL, printfParams); if (clDbgPauseOnCodeError) clDbgPause(); } if (!result) { todo; } } while(0)


/**
 ************************************
 *  \page clDbgResourceNotify
 *
 *  \par Synopsis:
 *  Call to indiciate the allocation or release of a resource, for debugging purposes
 *
 *  \par Header File:
 *  clDbg.h
 *
 *  \par Syntax:
 *  \code 	 clDbgResourceNotify(resourceType, operation, resource, printfParams)                          
 *  \endcode
 *
 *  \par Description:
 *  Call this function when you are allocating/deallocating a resource or initializing/deinitializing a service.  The debugging infrastructure will
 *  track this resource to make sure it is not leaked, inited/destructed twice, etc.
 *  The resourceType is one of the "Resource" enums defined below, go ahead and add others when necessary
 *  The resource operation must be to allocate or release (enum defined below)
 *  The resourceid is a integer that uniquely identifies the resource within the resourceType.  You get to choose what exactly this is for each type if you
 *  add a new type.
 *  
 *  For Handles, this is the handle id.  For Memory a pointer to the memory.  For Mutexes, a pointer to the mutex.  
 *  For Services use the ClCompIdT (defined in clCommon.h).
 *
 *  \par Library File:
 *   libClUtils.a
 *
 *  \par Related Function(s):
 *
 */

enum
  {
    clDbgHandleResource         = 1,
    clDbgHandleGroupResource    = 2,  /* A major subsystem like checkpointing */
    clDbgWorkHandleResource     = 3,  /* A major subsystem like checkpointing */
    clDbgMemoryResource         = 4,
    clDbgMutexResource          = 5,
    clDbgComponentResource      = 6,  /* A major subsystem like checkpointing, set resourceId to the component ID in clCommon.h. */
    clDbgCheckpointResource     = 7,

    clDbgAllocate = 1000,
    clDbgRelease  = 1001
  };

#define clDbgResourceNotify(resourceType, operation, resourceGroup, resourceId, printfParams) do { CL_DEBUG_PRINT(clDbgResourceLogLevel, printfParams); } while(0)

#define clDbgResourceLimitExceeded(resourceType, resourceGroup, printfParams) do { CL_DEBUG_PRINT(clDbgResourceLogLevel, printfParams); clDbgRootCauseError(CL_ERR_NO_RESOURCE,printfParams); } while(0)


#ifdef __cplusplus
}
#endif

#endif
