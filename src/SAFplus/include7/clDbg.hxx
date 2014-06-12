#pragma once
#ifndef CL_DBG_HXX
#define CL_DBG_HXX

namespace SAFplus
{
/* See clDbg.c for documentation on these global variables */
extern int clDbgPauseOn;
extern int clDbgPauseOnCodeError;
extern int clDbgNoKillComponents;
extern int clDbgCompTimeoutOverride;
extern int clDbgLogLevel;
extern int clDbgResourceLogLevel;
extern int clDbgReverseTiming;


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

#undef clDbgRootCodeError
#define clDbgCodeError(clErr, ...) do { (void)clErr;  logCritical("---","---", __VA_ARGS__); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

  /* A clDbgCodeError is also a root cause error, so you don't have to call both functions */
#undef clDbgRootCauseError
#define clDbgRootCauseError(clErr, ...) do { (void)clErr;  logCritical("---","---", __VA_ARGS__); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

#ifdef clDbgNotImplemented
#undef clDbgNotImplemented
#endif
#define clDbgNotImplemented(...) do { logCritical("---","---", "Not Implemented:" __VA_ARGS__); if (clDbgPauseOnCodeError) clDbgPause(); } while(0)

#ifdef clDbgCheck  // resolve warning with including SAFplus6 clDbg.h
#undef clDbgCheck
#endif
#define clDbgCheck(predicate, todo, ...) do { int result = predicate; if (!result) { logCritical("---","---", __VA_ARGS__); if (clDbgPauseOnCodeError) clDbgPause(); } if (!result) { todo; } } while(0)
    
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

#define CL_LOG_SP(...) __VA_ARGS__

#define clDbgResourceNotify(resourceType, operation, resourceGroup, resourceId, ...) \
do { \
      clLog(clDbgResourceLogLevel,"---","---",__VA_ARGS__); \
} while(0)

#define clDbgResourceLimitExceeded(resourceType, resourceGroup, ...) \
do { \
     clLog(clDbgResourceLogLevel,"---","---", __VA_ARGS__); \
     clDbgRootCauseError(CL_ERR_NO_RESOURCE,__VA_ARGS__); \
} while(0)

};


/**
 ************************************
 *  \page clDbgResume
 *
 *  \par Synopsis:
 *  DEBUGGING: Continue a paused thread -- do not call in code!
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
 *  This function is outside that SAFplus namespace for convenience in gdb.
 *
 *  \par Library File:
 *   libClDebugClient.a
 *
 *  \par Related Function(s):
 *   \ref "clDebugResume"
 */
extern "C" void clDbgResume();


#endif
