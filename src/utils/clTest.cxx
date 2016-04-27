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

#include <clTestApi.hxx>
#include <clLogApi.hxx>
#include <clThreadApi.hxx>
#include <clGlobals.hxx>

namespace SAFplus
{

SAFplus::LogSeverity testLogLevel = LOG_SEV_INFO;  /* 7 */
  //int testOn                    =       1;
int testPrintIndent           =       0;
TestVerbosity testVerbosity = (TestVerbosity) (TEST_PRINT_ALL & (~TEST_PRINT_TEST_OK));


enum
  {
    MaxTestCaseRecursion = 10
  };

};

using namespace SAFplus;

namespace SAFplusI
{
  ClTestCaseData clTestCaseStack[SAFplus::MaxTestCaseRecursion];
int            clTestCaseStackCurpos = 0;
ClTestCaseData clCurTc;
Mutex testLogMutex; /* Serializes writes to the test output log */
FILE* clTestFp                  =       0; 
    

void clPushTestCase(const ClTestCaseData* tcd)
{
  CL_ASSERT(clTestCaseStackCurpos < SAFplus::MaxTestCaseRecursion);
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
  //clOsalMutexInit(&testLogMutex); 
}

int clTestGroupFinalizeImpl()
{
  clTestPrint(("Test completed.  Cases:  [%d] passed, [%d] failed, [%d] malfunction.\n", clCurTc.passed, clCurTc.failed, clCurTc.malfunction));
  //clOsalMutexDestroy(&testLogMutex);
  //clTestPrintAt(__FILE__, __LINE__, __FUNCTION__, ("%d",5)); 
  return (clCurTc.failed);
}

void clTestImpl(const char* file, int line, const char* function, const char * id, const char * error, int ok)
  {
    char str[1024]="";
    if (ok)
      {
        clCurTc.passed++;
        if (testVerbosity & TEST_PRINT_TEST_OK)
          {
            snprintf(str,1024,"%s: Ok", id);
          }
      }
    else
      {
        clCurTc.failed++;
        if (testVerbosity & TEST_PRINT_TEST_FAILED)
          {
            snprintf(str,1024,"%s: Failed.  Error info: %s", id, error); 
          }
      }

    if (str[0] != 0) clTestPrintImpl(file, line, function, str);

  }


static int pid = 0;

void clTestPrintImpl(const char* file, int line, const char* fn, const char* str)
{
  // This is not pretty, but the compiler prints warnings if the string passed to the test macros is empty.  So instead of empty, we use " " to denote nothing meaningful in the string.
  if (str[0] == ' ' && str[1] == 0) return;

  if (pid==0) pid = getpid();
  testLogMutex.lock(); /* Lock clTestFp because it is closed and reopened each time a log is written (in case the file is deleted or moved) */
  if (!clTestFp)
    {
      char* testDir = getenv("CL_TEST_LOG");
      if (!testDir)
        clTestFp = fopen("/var/log/testresults.log","a+");
      else
	{
	  char testFilename[512];
	  strncpy(testFilename,testDir,511);
	  strncat(testFilename,"/testresults.log",511);
	  testFilename[511]=0;
	  clTestFp = fopen(testFilename,"a+");
	}
      /* fprintf(clTestFp, "\n\n"); */
    }

      char todStr[128]="";
      time_t now = time(0);
      struct tm tmStruct;
      localtime_r(&now, &tmStruct);
      strftime(todStr, 128, "[%b %e %T]", &tmStruct);

  if (clTestFp)
    {
      fprintf(clTestFp, "%s: %s (%d), at %s:%d (%s): %s\n",todStr, ASP_COMPNAME, pid, file, line, fn, str);
      fflush(clTestFp);
      fclose(clTestFp); /* Close and reopen each time in case testresults is deleted */
      clTestFp = 0;
    }

  logMsgWrite(SAFplus::APP_LOG, testLogLevel,0,"TST", "---",file, line, "(%s): %s",fn, str);
  testLogMutex.unlock();
   
}


};

