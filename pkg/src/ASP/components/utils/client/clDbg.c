/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include <clDebugApi.h>

/* Debug mode control variables.  Change the values to change behavior. */
int    clDbgPauseOn             = 0; /* 0/1; */  /* Set to 0 to make clDbgPause a nop */
int    clDbgPauseOnCodeError    = 0; /* 0/1; */  /* Set to 1 to execute clDbgPause in the clDbgCodeError() macro */
int    clDbgNoKillComponents    = 0; /* 0/1; */  /* Set to 1 to stop Components from being killed */
int    clDbgCompTimeoutOverride = 0000; /* 10000; */  /* Set to > 0 to cause all component timeouts to be this many seconds */
int    clDbgLogLevel            = 0; /* CL_DEBUG_WARN is a good choice or 0 to turn off */
int    clDbgResourceLogLevel    = CL_DEBUG_TRACE; /* CL_DEBUG_TRACE to turn off, CL_DEBUG_INFO a good choice.  Level to log resource allocation/free */

/* Variables that can shake out bugs */

/** Set to > 0 to cause the ASP to introduce different timing into library calls (delays by that many ms).  This will help you find thread race conditions */
int    clDbgReverseTiming       = 0;  

/* Private variables, do not change */
sem_t  clDbgSem;
int    clDbgSemInited           = 0;

FILE*  clDbgLogFp=NULL;

void clDbgInitialize(void)
{
    clDbgPauseOn = clParseEnvBoolean("CL_DEBUG_PAUSE") == CL_TRUE ? 1 : clDbgPauseOn;
    clDbgPauseOnCodeError = clParseEnvBoolean("CL_DEBUG_PAUSE_CODE_ERROR") == CL_TRUE ? 
        1 : clDbgPauseOnCodeError;
    clDbgNoKillComponents = clParseEnvBoolean("CL_DEBUG_COMP_NO_KILL") == CL_TRUE ? 
        1 : clDbgNoKillComponents;
    clDbgCompTimeoutOverride = getenv("CL_DEBUG_COMP_TIMEOUT_OVERRIDE") ? 
        atoi(getenv("CL_DEBUG_COMP_TIMEOUT_OVERRIDE")) : clDbgCompTimeoutOverride;
    clDbgLogLevel = getenv("CL_DEBUG_LOG_LEVEL") ? 
        atoi(getenv("CL_DEBUG_LOG_LEVEL")) : clDbgLogLevel;
    clDbgResourceLogLevel = getenv("CL_DEBUG_RESOURCE_LOG_LEVEL") ?
        atoi(getenv("CL_DEBUG_RESOURCE_LOG_LEVEL")) : clDbgResourceLogLevel;
    clDbgReverseTiming = getenv("CL_DEBUG_REVERSE_TIMING") ? 
        atoi(getenv("CL_DEBUG_REVERSE_TIMING")) : clDbgReverseTiming;
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
              CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("dbgPause function called from %s:%d.  Thread is paused.  Open the debugger and execute dbgResume() to continue.",file,line));
              do {
                rc = sem_wait(&clDbgSem);
              } while ((rc == -1)&&(errno == EINTR));
            }
          else 
            {
              int err = errno;          
              CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Can't create debug semaphore, error: %s, errno %d.", strerror(err), err));
            }
        }
    }
}

/* Call this from gdb to continue your program. */
void clDbgResume()
{
  sem_post(&clDbgSem);
}


/* Function to collate debugging messages */
void clDbgMsg(int pid, const char* file, int line, const char* fn, int level, const char* str)
{
    char* levelStr="Invalid Level";

    if      (level >= CL_DEBUG_TRACE)    levelStr = "trace";
    else if (level >= CL_DEBUG_INFO)     levelStr = "info";
    else if (level >= CL_DEBUG_WARN)     levelStr = "warn";
    else if (level >= CL_DEBUG_ERROR)    levelStr = "error";
    else if (level >= CL_DEBUG_CRITICAL) levelStr = "critical";


    if (level <= clDbgLogLevel)
    {
        if (!clDbgLogFp)
        {
#ifdef VXWORKS_BUILD
            clDbgLogFp = stdout;
#else
            clDbgLogFp = fopen("/var/log/aspdbg.log","a+");
#endif
        }
        if (clDbgLogFp)
        {
            char todStr[128]="";
            time_t now = time(0);
            struct tm tmStruct;
            localtime_r(&now, &tmStruct);
            strftime(todStr, 128, "[%b %e %T]", &tmStruct);
            pthread_t thread_id = pthread_self();
            fprintf(clDbgLogFp, "%s %s: %s (%d.%d), at %s:%d (%s): %s\n",todStr, levelStr, CL_EO_NAME, pid, (int) thread_id,file, line, fn, str);
            fflush(clDbgLogFp);
        }
    }
}
