/*
 * POSIX Includes.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <clCommon.h>

/*
 * Basic ASP Includes.
 */

#include <clOsalApi.h>
#include <clIocServices.h>

/*
 * ASP Client Includes.
 */

#include <clRmdApi.h>
#include <clDebugApi.h>
//#include <clOmApi.h>
//#include <clOampRtApi.h>
//#include <clProvApi.h>
//#include <clAlarmApi.h>
#include <clCpmApi.h> 
#include <clEoApi.h>
//#include <clIdlApi.h>

#include <saAmf.h>

#include <Python.h>


/*
 * This template has a few default clprintfs. These can be disabled by 
 * changing clprintf to a null function
 */
 
#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
                                  CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
                                  __VA_ARGS__)

int clCreateCsiPython(char* s, int maxLen,const SaNameT* compName, SaAmfCSIDescriptorT* csiDescriptor, SaAmfHAStateT haState);
void clRunPython(char* cmd);

PyThreadState* thrdState;
ClOsalMutexT   pyMutex;
ClOsalCondT    event;
int            quit=0;

void clPyGlueInit(char* appModule,void (*init_extensions[])(void),int argc, char**argv)
{
  char buf[1024];
  ClRcT rc;
    Py_Initialize();
    PySys_SetArgv(argc, argv);
    PyEval_InitThreads();
    
    if (init_extensions) 
      {
        int i = 0;
        for(i=0; init_extensions[i]!=NULL; i++) (*init_extensions[i])();
      }
    thrdState = PyThreadState_Get();

    rc = clOsalMutexInit(&pyMutex);
    CL_ASSERT(rc==CL_OK); 

    rc = clOsalCondInit(&event);
    CL_ASSERT(rc==CL_OK); 

    rc = clOsalMutexLock(&pyMutex);
    CL_ASSERT(rc==CL_OK); 

    PyThreadState_Swap(thrdState);

    PyRun_SimpleString("import os, os.path, sys\n");
    snprintf(buf,1024,"sys.path.append(os.path.realpath('%s'))\n",CL_APP_BINDIR);
    clprintf(CL_LOG_SEV_INFO, buf);
    PyRun_SimpleString(buf);
    //PyRun_SimpleString("sys.path.append(os.path.realpath('../../bin'))\n");
    snprintf(buf,1024,"from %s import *\n",appModule);
    clprintf(CL_LOG_SEV_INFO, buf);
    PyRun_SimpleString(buf);

    PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();

    rc=clOsalMutexUnlock(&pyMutex);
    CL_ASSERT(rc==CL_OK); 

}


void clPyGlueStart(int block)
{
    ClRcT rc;
    clRunPython("eoApp.Start()");

    if (block)
      {
      ClTimerTimeOutT forever={0};
      rc = clOsalMutexLock(&pyMutex);
      CL_ASSERT(rc==CL_OK); 
      clOsalCondWait (&event,&pyMutex,forever);
      }
}

void clPyGlueTerminate()
{
    clRunPython("eoApp.Stop()");
    clOsalCondBroadcast (&event);
    clOsalMutexLock(&pyMutex);
    quit=1;
    PyEval_AcquireThread(thrdState);
    Py_Finalize();
    clOsalMutexUnlock(&pyMutex);
}

ClRcT clPyGlueHealthCheck()
{
  /* GAS TODO: get the return value out of this call */
  clRunPython("eoApp.HealthCheck()");
  return CL_OK;
}


void clPyGlueCsiSet(
                     const SaNameT       *compName,
                     SaAmfHAStateT       haState,
                     SaAmfCSIDescriptorT* csiDescriptor)
{
#define BSZ 2048
  char s[BSZ];
  int curlen = 0;
  int ok = 1;

  switch ( haState )
    {
    //case CL_AMS_HA_STATE_ACTIVE:
    case SA_AMF_HA_ACTIVE:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.ActivateWorkAssignment(");
        break;
      }

    //case CL_AMS_HA_STATE_STANDBY:
    case SA_AMF_HA_STANDBY:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.StandbyWorkAssignment(");
        break;
      }

    //case CL_AMS_HA_STATE_QUIESCED:
    case SA_AMF_HA_QUIESCED:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.AbortWorkAssignment(");
        break;
      }

    //case CL_AMS_HA_STATE_QUIESCING:
    case SA_AMF_HA_QUIESCING:
      {
        curlen += snprintf (s+curlen,BSZ-curlen, "eoApp.QuiesceWorkAssignment(");
        break;
      }

    default:
      {
        ok = 0;  // Its NOT ok
        clprintf (CL_LOG_SEV_ERROR, "Unknown AMF request %d",haState);
        break;
      }
    }

  if (ok)
    {
      clprintf (CL_LOG_SEV_INFO, "Creating python command: %s",s);
      curlen += clCreateCsiPython(s+curlen, BSZ-curlen, compName, csiDescriptor, haState);
      curlen += snprintf (s+curlen,BSZ-curlen, ")\n");
      clprintf (CL_LOG_SEV_INFO, "%s",s);
      clRunPython(s);

#if 0
      if (strlen(csiDescriptor->csiName.value))
        {
          /* Just for debugging use */
          curlen = snprintf(s,BSZ,"%s = ", csiDescriptor->csiName.value);
          curlen += clCreateCsiPython(s+curlen, BSZ-curlen, compName, csiDescriptor, haState);
          clprintf (CL_LOG_SEV_INFO, "%s",s);
          clRunPython(s);
        }
#endif

    }
}


void clPyGlueCsiRemove(
                       const SaNameT       *compName,
                       const SaNameT       *csiName,
                       SaAmfCSIFlagsT      csiFlags)
{
  char s[1024];
  snprintf(s,1024,"eoApp.RemoveWorkAssignment('%s',%s)\n", csiName->value, (csiFlags & CL_AMS_CSI_FLAG_TARGET_ALL) ? "True":"False");
  clRunPython(s);
}



void clRunPython(char* cmd)
{ 
  #define BSZ 2048
  char pyBuf[BSZ];
  clprintf (CL_LOG_SEV_INFO, "clRunPython called with [%s]", cmd);
  clOsalMutexLock(&pyMutex);

  if (quit)
    {
      clprintf(CL_LOG_SEV_INFO,"Python has quit, so not running: %s",cmd);
      clOsalMutexUnlock(&pyMutex);
      return;
    }

#if 0
  PyEval_AcquireLock();
  PyThreadState_Swap(thrdState);
#endif

  snprintf(pyBuf,BSZ,"clCmdFromAsp(\"\"\"%s\"\"\")\n",cmd);

  clprintf (CL_LOG_SEV_INFO, "clRunPython requesting python lock [%s]", cmd);
  PyEval_AcquireThread(thrdState);
  clprintf(CL_LOG_SEV_INFO,"Stage 1.  Passing to Python layer: %s",pyBuf);
  int ret = PyRun_SimpleString(pyBuf);
  clprintf (CL_LOG_SEV_INFO, "clRunPython requesting release of python lock [%s]", cmd);
  PyEval_ReleaseThread(thrdState);

  if (ret != 0)  clprintf(CL_LOG_SEV_ERROR,"Ran: %s.  There was an error.",pyBuf);

#if 0
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
#endif

  clprintf (CL_LOG_SEV_INFO, "clRunPython unlocking mutex [%s]", cmd);
  clOsalMutexUnlock(&pyMutex);
  clprintf (CL_LOG_SEV_INFO, "clRunPython unlocked mutex [%s]", cmd);
}
#undef BSZ

#define STRING_HA_STATE(S)                                                  \
(   ((S) == SA_AMF_HA_ACTIVE)             ? "Active" :                \
    ((S) == SA_AMF_HA_STANDBY)            ? "Standby" :               \
    ((S) == SA_AMF_HA_QUIESCED)           ? "Quiesced" :              \
    ((S) == SA_AMF_HA_QUIESCING)          ? "Quiescing" :             \
                                            "Unknown" )

#define STRING_CSI_FLAGS(S)                                                 \
(   ((S) == SA_AMF_CSI_ADD_ONE)            ? "Add One" :               \
    ((S) == SA_AMF_CSI_TARGET_ONE)         ? "Target One" :            \
    ((S) == SA_AMF_CSI_TARGET_ALL)         ? "Target All" :            \
                                                  "Unknown" )


int clCreateCsiPython(char* s, int maxLen, const SaNameT* compName, SaAmfCSIDescriptorT* csiDescriptor, SaAmfHAStateT haState)
{
  //  #define maxLen 2048
  //  char s[maxLen];
  int curlen = 0;

  //snprintf(s+curlen,maxLen-curlen,"csi = { 'flags':'0' }\n"); 

  curlen += snprintf (s+curlen,maxLen-curlen, "{ 'csiFlags':'%s'",STRING_CSI_FLAGS(csiDescriptor->csiFlags));

  curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiHaState':'%s' ",STRING_HA_STATE(haState));

  if (CL_AMS_CSI_FLAG_TARGET_ALL != csiDescriptor->csiFlags)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiName':'%s' ", csiDescriptor->csiName.value);
    }

  if ((CL_AMS_CSI_FLAG_ADD_ONE == csiDescriptor->csiFlags) || (CL_AMS_CSI_FLAG_TARGET_ONE == csiDescriptor->csiFlags))
    {
      ClUint32T i = 0;
        
      clprintf (CL_LOG_SEV_INFO, " %d Name Value Pairs:",csiDescriptor->csiAttr.number);
      for (i = 0; i < csiDescriptor->csiAttr.number; i++)
        {
          clprintf (CL_LOG_SEV_INFO, ", '%s':'%s' ", csiDescriptor->csiAttr.attr[i].attrName, csiDescriptor->csiAttr.attr[i].attrValue);          
          curlen += snprintf (s+curlen,maxLen-curlen, ", '%s':'%s' ", csiDescriptor->csiAttr.attr[i].attrName, csiDescriptor->csiAttr.attr[i].attrValue);
        }

      if (0)  
        {
          ClRcT rc;
          ClCpmCompCSIRefT csiRef = { 0 };
          unsigned int i,j;
          SaNameT clcName;
          
          clprintf(CL_LOG_SEV_INFO,"New CODE");

          strncpy (clcName.value, (ClCharT*) compName->value,compName->length);
          clcName.length = compName->length;
          clcName.value[clcName.length]=0;

          rc = clCpmCompCSIList(&clcName, &csiRef);
          if(rc != CL_OK)
            {
              clLogError("APP", "CSISET", "Comp CSI get returned [%#x]", rc);
            }
          else
            {
              for(i = 0; i < csiRef.numCSIs; ++i)
                {
                  ClCpmCompCSIT *pCSI = &csiRef.pCSIList[i];
                  clprintf(CL_LOG_SEV_INFO,"me: [%.*s] compared: [%.*s]",csiDescriptor->csiName.length,csiDescriptor->csiName.value, pCSI->csiDescriptor.csiName.length, pCSI->csiDescriptor.csiName.value);
                  if (strncmp((char*)csiDescriptor->csiName.value,(char*)pCSI->csiDescriptor.csiName.value,csiDescriptor->csiName.length)==0)
                    {
                      ClAmsCSIDescriptorT* cd = &pCSI->csiDescriptor;
                      for (j = 0; j < cd->csiAttributeList.numAttributes; j++)
                        {
                          curlen += snprintf (s+curlen,maxLen-curlen, ", '%s':'%s' ", cd->csiAttributeList.attribute[j].attributeName, cd->csiAttributeList.attribute[j].attributeValue);
                          clprintf(CL_LOG_SEV_INFO,s);
                        }
                   }
                }
              if(csiRef.pCSIList)
                clHeapFree(csiRef.pCSIList);
            }
        }
    }
    

  if (SA_AMF_HA_ACTIVE == haState)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiTransitionDescriptor':'%d' ", (int) csiDescriptor->csiStateDescriptor.activeDescriptor.transitionDescriptor);
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiActiveComponent':'%s' ", csiDescriptor->csiStateDescriptor.activeDescriptor.activeCompName.value);
    }
  else if (SA_AMF_HA_STANDBY == haState)
    {
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiStandbyRank':'%d' ", (int) csiDescriptor->csiStateDescriptor.standbyDescriptor.standbyRank);
      curlen += snprintf (s+curlen,maxLen-curlen, ", 'csiActiveComponent':'%s' ", csiDescriptor->csiStateDescriptor.activeDescriptor.activeCompName.value);
    }


  curlen += snprintf (s+curlen,maxLen-curlen, "}");

  //clprintf (CL_LOG_SEV_INFO, "%s",s);
  //clRunPython(s);

  return curlen;

}

