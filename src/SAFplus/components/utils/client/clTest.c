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
#include <string.h>

#include <clTestApi.h>
#include <clDebugApi.h>

int clTestLogLevel              =       CL_LOG_SEV_INFO;  /* 7 */
int clTestOn                    =       1;
int clTestPrintIndent           =       0;
FILE* clTestFp                  =       0; 
ClTestCaseData clCurTc;
ClTestVerbosity clTestVerbosity = CL_TEST_PRINT_ALL & (~CL_TEST_PRINT_TEST_OK);
ClOsalMutexT testLogMutex; /* Serializes writes to the test output log */


enum
  {
    MaxTestCaseRecursion = 10
  };

ClTestCaseData clTestCaseStack[MaxTestCaseRecursion];
int            clTestCaseStackCurpos = 0;

void clPushTestCase(const ClTestCaseData* tcd)
{
  CL_ASSERT(clTestCaseStackCurpos < MaxTestCaseRecursion);
  memcpy(&clTestCaseStack[clTestCaseStackCurpos],tcd,sizeof(ClTestCaseData));
  clTestCaseStackCurpos++;
}

void clPopTestCase(ClTestCaseData* tcd)
{
  CL_ASSERT(clTestCaseStackCurpos > 0);
  clTestCaseStackCurpos--;
  memcpy(tcd, &clTestCaseStack[clTestCaseStackCurpos],sizeof(ClTestCaseData));  
}

void clTestGroupInitializeImpl()
{
    clOsalMutexInit(&testLogMutex); 
}

int clTestGroupFinalizeImpl()
{
  clTestPrint(("Test completed.  Cases:  [%d] passed, [%d] failed, [%d] malfunction.\n", clCurTc.passed, clCurTc.failed, clCurTc.malfunction));
  clOsalMutexDestroy(&testLogMutex); 
  return (clCurTc.failed);
}

void clTestImpl(const char* file, int line, const char* function, const char * id, const char * error, int ok)
  {
    char str[1024]="";
    if (ok)
      {
        clCurTc.passed++;
        if (clTestVerbosity & CL_TEST_PRINT_TEST_OK)
          {
            snprintf(str,1024,"%s: Ok", id);
          }
      }
    else
      {
        clCurTc.failed++;
        if (clTestVerbosity & CL_TEST_PRINT_TEST_FAILED)
          {
            snprintf(str,1024,"%s: Failed.  Error info: %s", id, error); 
          }
      }

    if (str[0] != 0) clTestPrintImpl(file, line, function, str);

  }


static int pid = 0;

void clTestPrintImpl(const char* file, int line, const char* fn, const char* str)
{
  if (pid==0) pid = getpid();

  clOsalMutexLock(&testLogMutex); /* Lock clTestFp because it is closed and reopened each time a log is written (in case the file is deleted or moved) */
  if (!clTestFp)
    {
      clTestFp = fopen("/var/log/testresults.log","a+");
      /* fprintf(clTestFp, "\n\n"); */
    }

  if (clTestFp)
    {
      char todStr[128]="";
      time_t now = time(0);
      struct tm tmStruct;
      localtime_r(&now, &tmStruct);
      strftime(todStr, 128, "[%b %e %T]", &tmStruct);

      fprintf(clTestFp, "%s: %s (%d), at %s:%d (%s): %s\n",todStr, CL_EO_NAME, pid, file, line, fn, str);
      fflush(clTestFp);

      fclose(clTestFp); /* Close and reopen each time in case testresults is deleted */
      clTestFp = 0;
    }
  clOsalMutexUnlock(&testLogMutex);
   
}

