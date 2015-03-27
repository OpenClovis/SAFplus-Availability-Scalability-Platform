//? <section name="Debugging">

#pragma once
#ifndef CL_DBG_HXX
#define CL_DBG_HXX

// Include clLogIpi before including clDbg.hxx if you are an internal SAFplus component so the logs APIs default to the SAFplus log stream
#include <clLogApi.hxx>

namespace SAFplus
{

  extern int clDbgPauseOn; //? Default is 0.  Set to 1 to make the clDbgPause() function block the current thread.  This will allow an engineer to attach the the process via a debugger and then resume the thread.
  extern int clDbgPauseOnCodeError;  //? Default is 0. Set to 1 to execute clDbgPause in the clDbgCodeError() macro.
  extern int clDbgNoKillComponents;  // [TODO] Default is 0. Set to 1 to stop components from being killed (set this to 1 inside the AMF for example to stop the AMF from killing processes).
  extern int clDbgCompTimeoutOverride;  // [TODO] Detault is 0.   Set to > 0 to cause all component timeouts to be this many seconds.  This is very useful because it allows an engineer to debug components without the AMF thinking that the component is unresponsive/dead.
  extern int clDbgLogLevel; //? Default is 0 (off). The severity to use when these debugging functions issue any logs.
  extern SAFplus::LogSeverity clDbgResourceLogLevel;  //? Default is LOG_SEV_TRACE. The level to use when logging resource allocation debugging information
  extern int clDbgReverseTiming; //? Set to > 0 to cause the ASP to introduce different timing into certain library calls (delays by that many ms).  This will help you find thread race conditions by changing race timings.


  //? Stop the thread so the user can attach to it via a debugger.  Call this function to pause this thread.  Then attach via a debugger and execute clDebugResume() (in gdb "call clDebugResume()") to start the thread up again.  Useful when you don't have a chance to set a breakpoint.
#define clDbgPause() ::SAFplus::clDbgPauseFn(__FILE__,__LINE__)
void clDbgPauseFn(const char* file, int line);

#define CL_DEBUG_CODE_ERROR clDbgCodeError

  //? Call when a coding error (bug) is detected.  These errors will never be handled by the code so it does not make sense to return an error code, especially for a critical operation.  For example, if a passed parameter is invalid or if a function call is called before its 'library' is initialized.  
  // <arg name="clErr">The error code (if one exists).  Pass 0 (CL_OK) if the code can gracefully handle the problem.  For example, calling an "initialize" function twice could be handled gracefully -- the second call will just be skipped.</arg>
  // <arg name="...">The second parameter is a printf style parameter list to tell the user/developer what is going on, and WHY the problem occurred.</arg>
#define clDbgCodeError(clErr, ...) do { (void)clErr;  logCritical("---","---", __VA_ARGS__); if (::SAFplus::clDbgPauseOnCodeError) clDbgPause(); } while(0)

  /* [DEPRECATED] A clDbgCodeError is also a root cause error, so you don't have to call both functions */
#undef clDbgRootCauseError
#define clDbgRootCauseError(clErr, ...) do { (void)clErr;  logCritical("---","---", __VA_ARGS__); if (::SAFplus::clDbgPauseOnCodeError) clDbgPause(); } while(0)

#ifdef clDbgNotImplemented
#undef clDbgNotImplemented
#endif
  //? Call when functionality has not been implemented.  A critical log will be issued, regardless of the value of <ref type="variable">clDbgLogLevel</ref>.  And the thread will be paused if <ref type="variable">clDbgPauseOnCodeError</ref> is set.  Also, this makes a convenient search string to look for unimplemented sections of code before release!
#define clDbgNotImplemented(...) do { logCritical("---","---", "Not Implemented:" __VA_ARGS__); if (::SAFplus::clDbgPauseOnCodeError) clDbgPause(); } while(0)

#ifdef clDbgCheck  // resolve warning with including SAFplus6 clDbg.h
#undef clDbgCheck
#endif

  //? Like assert, call to verify that some condition is true.  If it is not true, a log will be issued, and the thread will be paused if <ref type="variable">clDbgPauseOnCodeError</ref> is set.  Next, the statements specified in the todo argument will be executed.
  // <arg name="predicate">The condition clause that should evaluate to true</arg>
  // <arg name="todo">Statements to be executed if the condition evaluates to false, e.g. "throw VeryBadError()"</arg>
#define clDbgCheck(predicate, todo, ...) do { int result = predicate; if (!result) { logWrite(::SAFplus::clDbgLogLevel,"---","---", __VA_ARGS__); if (::SAFplus::clDbgPauseOnCodeError) clDbgPause(); } if (!result) { todo; } } while(0)
    
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
      logWrite(clDbgResourceLogLevel,"---","---",__VA_ARGS__); \
} while(0)

#define clDbgResourceLimitExceeded(resourceType, resourceGroup, ...) \
do { \
     logWrite(clDbgResourceLogLevel,"---","---", __VA_ARGS__); \
     clDbgRootCauseError(CL_ERR_NO_RESOURCE,__VA_ARGS__); \
} while(0)

};


//? DEBUGGER ONLY: Continue a paused thread from the debugger -- do not call in code!  Call this function from the debugger to resume a paused thread this thread.  Useful when you don't have a chance to set a breakpoint.  This function is outside of the SAFplus namespace for convenience in gdb.  In gdb run "call clDbgResume()" to resume all paused threads.
extern "C" void clDbgResume();

#endif

//? </section>
