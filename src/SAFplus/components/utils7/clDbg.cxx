#include <stdio.h>
#include <semaphore.h>

#include <clDbg.hxx>
#include <clLogApi.hxx>
#include <clCommon.hxx>

namespace SAFplus
  {
/* Debug mode control variables.  Change the values to change behavior. */
  int    clDbgPauseOn             = 0; /* 0/1; */  /* Set to 0 to make clDbgPause a nop */
  int    clDbgPauseOnCodeError    = 0; /* 0/1; */  /* Set to 1 to execute clDbgPause in the clDbgCodeError() macro */
  int    clDbgNoKillComponents    = 0; /* 0/1; */  /* Set to 1 to stop Components from being killed */
  int    clDbgCompTimeoutOverride = 0000; /* 10000; */  /* Set to > 0 to cause all component timeouts to be this many seconds */
  int    clDbgLogLevel            = 0; /* CL_DEBUG_WARN is a good choice or 0 to turn off */
  SAFplus::LogSeverity    clDbgResourceLogLevel    = LOG_SEV_TRACE; /* CL_DEBUG_TRACE to turn off, CL_DEBUG_INFO a good choice.  Level to log resource allocation/free */

/* Variables that can shake out bugs */

/** Set to > 0 to cause the ASP to introduce different timing into library calls (delays by that many ms).  This will help you find thread race conditions */
  int    clDbgReverseTiming       = 0;  

/* Private variables, do not change */
  sem_t  clDbgSem;
  int    clDbgSemInited           = 0;

  FILE*  clDbgLogFp=NULL;

  void clDbgInitialize(void)
    {
    clDbgPauseOn = parseEnvBoolean("CL_DEBUG_PAUSE") == CL_TRUE ? 1 : clDbgPauseOn;
    clDbgPauseOnCodeError = parseEnvBoolean("CL_DEBUG_PAUSE_CODE_ERROR") == CL_TRUE ? 1 : clDbgPauseOnCodeError;
    clDbgNoKillComponents = parseEnvBoolean("CL_DEBUG_COMP_NO_KILL") == CL_TRUE ? 1 : clDbgNoKillComponents;
    clDbgCompTimeoutOverride = getenv("CL_DEBUG_COMP_TIMEOUT_OVERRIDE") ? atoi(getenv("CL_DEBUG_COMP_TIMEOUT_OVERRIDE")) : clDbgCompTimeoutOverride;
    clDbgLogLevel = getenv("CL_DEBUG_LOG_LEVEL") ? atoi(getenv("CL_DEBUG_LOG_LEVEL")) : clDbgLogLevel;
    char* pEnvVar;
    if((pEnvVar = getenv("CL_LOG_SEVERITY"))!=NULL)
      {
      clDbgResourceLogLevel = logSeverityGet(pEnvVar);
      }

    clDbgReverseTiming = getenv("CL_DEBUG_REVERSE_TIMING") ? atoi(getenv("CL_DEBUG_REVERSE_TIMING")) : clDbgReverseTiming;
    }

/* Call this from the code to pause your program. -- NOT thread safe! */
  void clDbgPauseFn(const char* file, int line)
    { 
    if (clDbgPauseOn != 0)
      {
      if (!clDbgSemInited)
        {
        int rc;
        clDbgSemInited = 1;
        rc = sem_init(&clDbgSem,0,0);
        if (rc == 0)
          {
          logCritical("DBG","PAUSE","dbgPause function called from %s:%d.  Thread is paused.  Open the debugger and execute dbgResume() to continue.",file,line);
          do {
            rc = sem_wait(&clDbgSem);
            } while ((rc == -1)&&(errno == EINTR));
          }
        else 
          {
          int err = errno;          
          logCritical("DBG","PAUSE","Can't create debug semaphore, error: %s, errno %d.", strerror(err), err);
          }
        }
      }
    }


  };

/* Call this from gdb to continue your program. */
extern "C"  void clDbgResume()
    {
    sem_post(&SAFplus::clDbgSem);
    }
