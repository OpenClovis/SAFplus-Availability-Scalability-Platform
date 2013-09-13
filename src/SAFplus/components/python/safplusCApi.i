//swig -python -I/code/build/include binding.i 
//shopt -s extglob 
//gcc -shared binding_wrap.o -o clIoc.so /code/build/target/i686/linux-2.6.20/lib/*.a
#define CL_IN
#define CL_OUT
#define CL_INOUT
//#define CL_DEPRECATED 

// The define below is in clVersion.h, but I'm not sure how to get SWIG to use it
#define VDECL_VER(sym, rel, major, minor) sym##_##rel##_##major##_##minor

#define __inline__

/*
%inline %{

typedef unsigned long long  ClUint64T; 
typedef signed long long    ClInt64T;  
typedef unsigned int        ClUint32T;
typedef signed int          ClInt32T;
typedef unsigned short      ClUint16T;
typedef signed short        ClInt16T;
typedef unsigned char       ClUint8T;
typedef signed char         ClInt8T;
typedef char                ClCharT; 
typedef signed int          ClFdT;  
typedef unsigned long       ClWordT;
typedef ClUint16T           ClBoolT;
typedef ClInt64T            ClTimeT;
typedef ClUint64T           ClHandleT;
typedef ClUint64T           ClSizeT;
typedef ClInt64T            ClOffsetT;
typedef ClUint64T           ClInvocationT;
typedef ClUint64T           ClSelectionObjectT;
typedef ClUint64T           ClNtfIdentifierT;
typedef ClInt8T             *ClAddrT;
typedef void*               ClPtrT;
typedef ClUint32T       ClRcT;
%}
*/

%module safplusCApi
%include "typemaps.i"
%include "cpointer.i"
%include "carrays.i"
%include "cstring.i"

%{
#define CL_AMS_MGMT_FUNC
#define EO
#include "clVersion.h"
#include "clCommon.h"
#include "clLogApi.h"
#include "clLogUtilApi.h"
#include "clLogIpiWrap.h"
#include "clAmsTypes.h"
#include "clIocApi.h"
#include "clAmsMgmtClientApi.h"
#include "clAmsTriggerApi.h"
#include "saCkpt.h"
#include "saEvt.h"
%}

%init %{
%}

#define CL_AMS_MGMT_FUNC
#define EO

%include "clCommon.h"
#undef CL_DEPRECATED
#define CL_DEPRECATED

%include "clCommonErrors.h"
%include "clAmsErrors.h"

%pointer_functions(SaNameT, SaNameT_p)
%array_functions(SaNameT, SaNameT_array)
%array_functions(ClAmsCSINameValuePairT, ClAmsCSINameValuePairT_array)
%array_functions(ClAmsEntityT, ClAmsEntityT_array)
// Overrides:


//%typemap(python,varout) ClAmsEntityConfigT  **OUTPUT {
//	... Convert an int *status ...
//}


// LOGGING:
//%include "clLogUtilApi.h"
//%include "clLogApi.h"
//%include "clLogIpiWrap.h"

/* Note by allowing 1 extra arg, you can do clLogMsgWrite(...,"%s",yourString) which is safe. 
   Doing clLogMsgWrite(...,yourString) is dangerous because what if yourString contains a %?
*/

extern ClHandleT  CL_LOG_HANDLE_SYS; 
extern ClHandleT CL_LOG_HANDLE_APP;

%varargs(1, char *arg = NULL) clLogMsgWrite;
extern ClRcT
clLogMsgWrite(ClHandleT       streamHdl,
              ClLogSeverityT  severity,
              ClUint16T       serviceId,
              const ClCharT         *pArea,
              const ClCharT         *pContext,
              const ClCharT         *pFileName,
              ClUint32T       lineNum,
              const ClCharT   *pFmtStr,
              ...);

#define CL_LOG_STREAM_CREATE  0x1

extern ClRcT clLogInitialize(CL_OUT   ClLogHandleT           *OUTPUT,
                CL_IN    const void             *pLogCallbacks,
                CL_INOUT ClVersionT             *pVersion);

extern ClRcT clLogFinalize(CL_IN ClLogHandleT  hLog);

typedef enum
{
    /**
     * Flags specifies the streams global to the node. 
     */
    CL_LOG_STREAM_GLOBAL = 0,
    /**
     * Flag specifies the streams local to the node.
     */
    CL_LOG_STREAM_LOCAL  = 1
}ClLogStreamScopeT;

typedef enum
{
 /**
  * It directs the Log Service to create a new Log File Unit when the current 
  * Log File Unit becomes full. The number of maximum Log File Units that can 
  * simultaneously exist is limited by maxFilesRotated attribute of the Log Stream.
  * Once this limit is reached, the oldest Log File Unit is deleted and a new one is
  * created. 
  */
  CL_LOG_FILE_FULL_ACTION_ROTATE = 0,
 /**
  * It makes the Log Service treat the Log File as a circular buffer, i.e., when the Log
  * File becomes full, Log Service starts overwriting oldest records.
  */
  CL_LOG_FILE_FULL_ACTION_WRAP   = 1,
  /**
   * Log Service stops putting more records in the Log File once it becomes full.
   */
  CL_LOG_FILE_FULL_ACTION_HALT   = 2
} ClLogFileFullActionT;


typedef struct
{
/**
 * Its the prefix name of file units that are going to be created. 
 */
    ClCharT               *fileName;
/**
 * Its the path where the log file unit(s) will be created.
 */
    ClCharT               *fileLocation;
/**
 * Size of the file unit. It will be truncated to multiples of recordSize.
 */
    ClUint32T             fileUnitSize;
/**
 * Size of the log record. 
 */
    ClUint32T             recordSize;
/**
 * Log file replication property. Currently is not supported.
 */
    ClBoolT               haProperty;
/**
 * Action that log service has to take, when the log file unit becomes full.
 */
    ClLogFileFullActionT  fileFullAction;
/**
 * If fileAction is CL_LOG_FILE_FULL_ACTION_ROTATE, the maximum num of log
 * file units that will be created by logService.Otherwise ignored.
 */
    ClUint32T             maxFilesRotated;
/**
 * Num of log records after which the log stream records must be flushed.
 */
    ClUint32T             flushFreq;
/**
 * Time after which the log stream records must be flushed.
 */
    ClTimeT               flushInterval;
/**
 * The water mark for file units.When the size of file reaches this level,
 * the water mark event will be published.
 */
    ClWaterMarkT          waterMark;
} ClLogStreamAttributesT;


extern ClRcT
clLogStreamOpen(CL_IN  ClLogHandleT            hLog,
                CL_IN  SaNameT                 streamName,
                CL_IN  ClLogStreamScopeT       streamScope,
                CL_IN  ClLogStreamAttributesT  *pStreamAttr,
                CL_IN  ClLogStreamOpenFlagsT   streamOpenFlags,
                CL_IN  ClTimeT                 timeout,
                CL_OUT ClLogStreamHandleT      *phStream);


/**
 * Area string for unspecified component area
 */
#define CL_LOG_AREA_UNSPECIFIED "---"

/**
 * Context string for unspecified component context
 */
#define CL_LOG_CONTEXT_UNSPECIFIED "---"

/**
 * Default servie id for SYS components.
 */
#define CL_LOG_DEFAULT_SYS_SERVICE_ID   0x01

typedef enum
  {
/**
 * setting severity as EMERGENCY.
 */
    CL_LOG_SEV_EMERGENCY = 0x1,
/**
 * setting severity as ALERT. 
 */
    CL_LOG_SEV_ALERT,
/**
 * setting severity as CRITICAL. 
 */
    CL_LOG_SEV_CRITICAL,
/**
 * setting severity as ERROR. 
 */
    CL_LOG_SEV_ERROR,
/**
 * setting severity as WARNING. 
 */
    CL_LOG_SEV_WARNING,
/**
 * setting severity as NOTICE. 
 */
    CL_LOG_SEV_NOTICE,
/**
 * setting severity as INFORMATION.
 */
    CL_LOG_SEV_INFO,
/**
 * setting severity as DEBUG.
 */
    CL_LOG_SEV_DEBUG,
    CL_LOG_SEV_DEBUG1   =      CL_LOG_SEV_DEBUG,
    CL_LOG_SEV_DEBUG2,
    CL_LOG_SEV_DEBUG3,
    CL_LOG_SEV_DEBUG4,
    CL_LOG_SEV_DEBUG5,
/**
 * setting severity as DEBUG.
 */
    CL_LOG_SEV_TRACE  =    CL_LOG_SEV_DEBUG5,
    CL_LOG_SEV_DEBUG6,
    CL_LOG_SEV_DEBUG7,
    CL_LOG_SEV_DEBUG8,
    CL_LOG_SEV_DEBUG9,
/**
 * Maximum severity level. 
 */
  CL_LOG_SEV_MAX    =    CL_LOG_SEV_DEBUG9
  } ClLogSeverityT;



%include "clMetricApi.h"

%include "saAis.h"
%include "saEvt.h"
%include "clAmsEntities.h"

//%typemap(newfree) ClAmsNodeConfigT* "clHeapFree($1);";
//%typemap(newfree) ClAmsSGConfigT* "clHeapFree($1);";
//%typemap(newfree) ClAmsSUConfigT* "clHeapFree($1);";
//%typemap(newfree) ClAmsSIConfigT* "clHeapFree($1);";
//%typemap(newfree) ClAmsCompConfigT* "clHeapFree($1);";

//%newobject clAmsMgmtNodeGetConfig;
//%newobject clAmsMgmtServiceGroupGetConfig;
//%newobject clAmsMgmtServiceUnitGetConfig;
//%newobject clAmsMgmtServiceInstanceGetConfig;
//%newobject clAmsMgmtCompGetConfig;

extern ClAmsNodeConfigT* clAmsMgmtNodeGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

extern ClAmsSGConfigT* clAmsMgmtServiceGroupGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);
   
extern ClAmsSUConfigT* clAmsMgmtServiceUnitGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);
    
extern ClAmsSIConfigT* clAmsMgmtServiceInstanceGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

extern ClAmsCSIConfigT* clAmsMgmtCompServiceInstanceGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

extern ClAmsCSIStatusT* clAmsMgmtCompServiceInstanceGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);
    
extern ClAmsCompConfigT* clAmsMgmtCompGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/* Make OpenClovis return codes into exceptions */
/* Suppress the return code */
/* It can't be empty in case the function has no output params */
%typemap(out) ClRcT 
  {
    if (result!=CL_OK)
      {
      PyErr_SetObject(PyExc_SystemError,PyInt_FromLong(result));
      goto fail;
      }
    $result = Py_None;
    Py_INCREF($result);
  };


extern ClRcT clAmsMgmtInitialize(ClAmsMgmtHandleT  *OUTPUT, const ClAmsMgmtCallbacksT  *amsMgmtCallbacks, ClVersionT  *version);

extern ClRcT clAmsMgmtCCBInitialize(ClAmsMgmtHandleT    amlHandle, ClAmsMgmtCCBHandleT *OUTPUT );
extern ClRcT clAmsMgmtEntityGet(ClAmsMgmtHandleT handle, ClAmsEntityRefT    *INOUT);
extern ClRcT clAmsMgmtEntityGetConfig(ClAmsMgmtHandleT    handle,ClAmsEntityT    *entity,ClAmsEntityConfigT  **OUTPUT );
extern ClRcT clAmsMgmtEntityGetStatus(ClAmsMgmtHandleT handle,ClAmsEntityT    *entity,ClAmsEntityStatusT  **OUTPUT );
extern ClRcT clAmsMgmtGetCSINVPList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *csi,
        CL_OUT  ClAmsCSINVPBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetNodeDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetNodeSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);

extern ClRcT clAmsMgmtGetSGSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSGSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSUCompList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSISURankList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSIDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSICSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);
extern ClRcT clAmsMgmtGetSGInstantiableSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *OUTPUT);

%apply (char *STRING, int LENGTH) { (ClCharT *data, ClUint32T len) };
extern ClRcT clAmsMgmtEntityUserDataSetKey(ClAmsMgmtHandleT handle, 
                                           ClAmsEntityT *entity,
                                           SaNameT *key,
                                           ClCharT *data, ClUint32T len);
extern ClRcT clAmsMgmtEntityUserDataSet(ClAmsMgmtHandleT handle, 
                                        ClAmsEntityT *entity,
                                        ClCharT *data, ClUint32T len);

%cstring_output_allocate_size(ClCharT **data, ClUint32T *len, clHeapFree(*$1));
extern ClRcT clAmsMgmtEntityUserDataGetKey(ClAmsMgmtHandleT handle, 
                                           ClAmsEntityT *entity,
                                           SaNameT *key,
                                           ClCharT **data, ClUint32T *len);
extern ClRcT clAmsMgmtEntityUserDataGet(ClAmsMgmtHandleT handle, 
                                        ClAmsEntityT *entity,
                                        ClCharT **data, ClUint32T *len);

%cstring_output_maxsize(ClCharT *aspInstallInfo, ClUint32T len);
extern ClRcT clAmsMgmtGetAspInstallInfo(ClAmsMgmtHandleT handle, const ClCharT *nodeName,
                                        ClCharT *aspInstallInfo, ClUint32T len);


%include "clAmsMgmtCommon.h"
%include "clAmsMgmtClientApi.h"
%include "clAmsTriggerApi.h"
%include "clIocApi.h"
/* %include "clCpmApi.h" */
%include "clAmsTypes.h"



/* Make SAF return codes into exceptions */

/* Generate the exception */
%exception {
 $action
 if (result!=SA_AIS_OK)
   {
   PyErr_SetObject(PyExc_SystemError,PyInt_FromLong(result));
   goto fail;
   }
}

/* Suppress the return code */
/* It can't be empty in case the function has no output params */
%typemap(out) SaAisErrorT 
  {
    $result = Py_None;
    Py_INCREF($result);
  };


/* Convert SaNameT from Python --> C
   Note that this function null terminates the string in SaNameT, so its not quite SAF compliant
   in the case where the name is exactly SA_MAX_NAME_LENGTH bytes long.
*/
%typemap(in) SaNameT* (SaNameT tmp) {
    $1 = &tmp;
    $1->length = PyString_Size($input);
    if ($1->length >= SA_MAX_NAME_LENGTH) $1->length=SA_MAX_NAME_LENGTH-1;
    memcpy($1->value,PyString_AsString($input),$1->length);
    $1->value[$1->length]=0;
}

/* Convert SaNameT from C --> Python */
%typemap(out) SaNameT* {
    $result = PyString_FromStringAndSize((char*) $1->value,$1->length);
}

/* Convert Creation attributes to just take a string which will be the section name.
   It would be nice to be able to change the expiration time...
 */
%typemap(in) SaCkptSectionCreationAttributesT* (SaCkptSectionCreationAttributesT tmp,SaCkptSectionIdT tmpsec) {
    $1 = &tmp;
    $1->expirationTime = SA_TIME_MAX;
    $1->sectionId      = &tmpsec;
    $1->sectionId->idLen = PyString_Size($input);
    $1->sectionId->id    = (SaUint8T *) PyString_AsString($input);
}

extern SaAisErrorT saCkptInitialize(SaCkptHandleT *OUTPUT, const SaCkptCallbacksT *callbacks, SaVersionT *version);

%apply (char *STRING, int LENGTH) { (const SaUint8T *initialData,SaSizeT initialDataSize) };
extern SaAisErrorT saCkptSectionCreate(SaCkptCheckpointHandleT checkpointHandle,
                    SaCkptSectionCreationAttributesT *sectionCreationAttributes,
                    const SaUint8T *initialData,
                    SaSizeT initialDataSize);

extern SaAisErrorT saCkptCheckpointOpen(SaCkptHandleT ckptHandle,
                     const SaNameT *ckeckpointName,
                     const SaCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
                     SaCkptCheckpointOpenFlagsT checkpointOpenFlags,
                     SaTimeT timeout,
                     SaCkptCheckpointHandleT *OUTPUT);

%typemap(in) const SaCkptSectionIdT * (SaCkptSectionIdT tmp) {
    $1 = &tmp;
    $1->idLen = PyString_Size($input);
    $1->id    = (SaUint8T *) PyString_AsString($input);
}

%apply (char *STRING, int LENGTH) { (const void *dataBuffer,SaSizeT dataSize) };

extern SaAisErrorT saCkptSectionOverwrite(SaCkptCheckpointHandleT checkpointHandle,
                       const SaCkptSectionIdT *sectionId,
                       const void *dataBuffer,
                       SaSizeT dataSize);


/* Convert the CheckpointWrite arguments into a list of tuples, i.e [("key","value"),("key1","value)] */
%typemap(in) (const SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements,SaUint32T *erroneousVectorIndex) {
  if (PyList_Check($input)) 
    {
      int i;
      $2 = PyList_Size($input);
      $1 = (SaCkptIOVectorElementT *) malloc($2*sizeof(SaCkptIOVectorElementT));
      $3 = NULL; /* (SaUint32T *) malloc($2*sizeof(SaUint32T)); */
      if ($1 == NULL) { PyErr_SetString(PyExc_MemoryError,"when allocating iovector"); goto fail; }
      for (i = 0; i < $2; i++) 
        {
        PyObject *s = PyList_GetItem($input,i);
        PyObject *name = PyTuple_GetItem(s,0);
        PyObject *value = PyTuple_GetItem(s,1);
        $1[i].sectionId.idLen = PyString_Size(name);
        $1[i].sectionId.id    = (SaUint8T *) PyString_AsString(name);

        $1[i].dataBuffer = (SaUint8T *) PyString_AsString(value);
        $1[i].dataSize   = PyString_Size(value);
        //printf("Write: %d %s %s %d\n", i,$1[i].sectionId.id,$1[i].dataBuffer,$1[i].dataSize);
        $1[i].dataOffset = 0;
        $1[i].readSize   = 0;
        }

    }
  else
    {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    goto fail;
    }

}

%typemap(freearg) (const SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements,SaUint32T *erroneousVectorIndex)
{
  if ($1) { free($1); $1 = NULL; }
  if ($3) { free($3); $3 = NULL; }
}


extern SaAisErrorT saCkptCheckpointWrite(SaCkptCheckpointHandleT checkpointHandle,
                      const SaCkptIOVectorElementT *ioVector,
                      SaUint32T numberOfElements,
                      SaUint32T *erroneousVectorIndex);


/* Convert the CheckpointRead arguments into a list of keys, i.e ["key","key1"] */

%typemap(in) (SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements,SaUint32T *erroneousVectorIndex) {
  if (PyList_Check($input)) 
    {
      int i;
      $2 = PyList_Size($input);
      $1 = (SaCkptIOVectorElementT *) malloc($2*sizeof(SaCkptIOVectorElementT));
      $3 = NULL; /* (SaUint32T *) malloc($2*sizeof(SaUint32T)); */
      if ($1 == NULL) { PyErr_SetString(PyExc_MemoryError,"when allocating iovector"); goto fail; }
      for (i = 0; i < $2; i++) 
        {
        PyObject *name = PyList_GetItem($input,i);
        $1[i].sectionId.idLen = PyString_Size(name);
        $1[i].sectionId.id    = (SaUint8T *) PyString_AsString(name);

        $1[i].dataBuffer = NULL;
        $1[i].dataSize   = 0;
        $1[i].dataOffset = 0;
        $1[i].readSize   = 0;
        }

    }
  else
    {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    goto fail;
    }

}

/* Convert the CheckpointRead output argument into a list of values, i.e ["value","va1"] */

%typemap(argout) (SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements,SaUint32T *erroneousVectorIndex) {
   //Py_XDECREF($result);   /* Blow away any previous result */
   int i;
   for (i = 0; i < $2; i++) 
     {
     //printf("read: %d val: %s %d\n",i,arg2[i].dataBuffer,arg2[i].readSize);
     $result = SWIG_Python_AppendOutput($result,PyString_FromStringAndSize($1[i].dataBuffer,$1[i].readSize));
     // for B.02.02: if ($1[i].dataBuffer) saCkptIOVectorElementDataFree(arg1, $1[i].dataBuffer);
     if ($1[i].dataBuffer) clHeapFree($1[i].dataBuffer);
     }
   
}

%typemap(freearg) (SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements,SaUint32T *erroneousVectorIndex)
{
  if ($1) { free($1); $1 = NULL; }
  if ($3) { free($3); $3 = NULL; }
}



extern SaAisErrorT saCkptCheckpointRead(SaCkptCheckpointHandleT checkpointHandle,
                     SaCkptIOVectorElementT *ioVector,
                     SaUint32T numberOfElements,
                     SaUint32T *erroneousVectorIndex);

extern SaAisErrorT saCkptCheckpointStatusGet(SaCkptCheckpointHandleT checkpointHandle, SaCkptCheckpointDescriptorT *INOUT);

%include "saCkpt.h"

%exception;
%typemap(out) SaAisErrorT;
