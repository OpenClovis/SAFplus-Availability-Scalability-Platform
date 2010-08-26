/*******************************************************************************
**
** FILE:
**   SaAis.h
**
** DESCRIPTION: 
**   This file contains the prototypes and type definitions required by all
**   Service Availability(TM) Forum's AIS services. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-CPROG-B.05.01
**
** DATE: 
**   Wed Oct 08 2008
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS.
**
** Copyright 2008 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/

#ifndef _SA_AIS_H
#define _SA_AIS_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef char                  SaInt8T;
typedef short                 SaInt16T;
typedef int                   SaInt32T;
typedef long long             SaInt64T;
typedef unsigned char         SaUint8T;
typedef unsigned short        SaUint16T;
typedef unsigned int          SaUint32T;
typedef unsigned long long    SaUint64T;

/** Types used by the NTF/IMMS service **/
typedef float                 SaFloatT;
typedef double                SaDoubleT;
typedef char*                 SaStringT;

typedef SaInt64T              SaTimeT;
typedef SaUint64T             SaInvocationT;
typedef SaUint64T             SaSizeT;
typedef SaUint64T             SaOffsetT;
typedef SaUint64T             SaSelectionObjectT;

#define SA_TIME_END              0x7FFFFFFFFFFFFFFFLL
#define SA_TIME_BEGIN            0x0LL
#define SA_TIME_UNKNOWN          0x8000000000000000LL

#define SA_TIME_ONE_MICROSECOND 1000LL
#define SA_TIME_ONE_MILLISECOND 1000000LL
#define SA_TIME_ONE_SECOND      1000000000LL
#define SA_TIME_ONE_MINUTE      60000000000LL
#define SA_TIME_ONE_HOUR        3600000000000LL
#define SA_TIME_ONE_DAY         86400000000000LL
#define SA_TIME_MAX             SA_TIME_END

#define SA_MAX_NAME_LENGTH 256

#define SA_TRACK_CURRENT       0x01
#define SA_TRACK_CHANGES       0x02
#define SA_TRACK_CHANGES_ONLY  0x04
#define SA_TRACK_LOCAL         0x08
#define SA_TRACK_START_STEP    0x10
#define SA_TRACK_VALIDATE_STEP 0x20

typedef enum {
    SA_FALSE = 0,
    SA_TRUE = 1
} SaBoolT;

typedef enum {
    SA_DISPATCH_ONE = 1,
    SA_DISPATCH_ALL = 2,
    SA_DISPATCH_BLOCKING = 3
} SaDispatchFlagsT;

typedef enum {
   SA_AIS_OK = 1,
   SA_AIS_ERR_LIBRARY = 2,
   SA_AIS_ERR_VERSION = 3,
   SA_AIS_ERR_INIT = 4,
   SA_AIS_ERR_TIMEOUT = 5,
   SA_AIS_ERR_TRY_AGAIN = 6,
   SA_AIS_ERR_INVALID_PARAM = 7,
   SA_AIS_ERR_NO_MEMORY = 8,
   SA_AIS_ERR_BAD_HANDLE = 9,
   SA_AIS_ERR_BUSY = 10,
   SA_AIS_ERR_ACCESS = 11,
   SA_AIS_ERR_NOT_EXIST = 12,
   SA_AIS_ERR_NAME_TOO_LONG = 13,
   SA_AIS_ERR_EXIST = 14,
   SA_AIS_ERR_NO_SPACE = 15,
   SA_AIS_ERR_INTERRUPT =16,
   SA_AIS_ERR_NAME_NOT_FOUND = 17,
   SA_AIS_ERR_NO_RESOURCES = 18,
   SA_AIS_ERR_NOT_SUPPORTED = 19,
   SA_AIS_ERR_BAD_OPERATION = 20,
   SA_AIS_ERR_FAILED_OPERATION = 21,
   SA_AIS_ERR_MESSAGE_ERROR = 22,
   SA_AIS_ERR_QUEUE_FULL = 23,
   SA_AIS_ERR_QUEUE_NOT_AVAILABLE = 24,
   SA_AIS_ERR_BAD_FLAGS = 25,
   SA_AIS_ERR_TOO_BIG = 26,
   SA_AIS_ERR_NO_SECTIONS = 27,
   SA_AIS_ERR_NO_OP = 28,          
   SA_AIS_ERR_REPAIR_PENDING = 29,
   SA_AIS_ERR_NO_BINDINGS = 30,
   SA_AIS_ERR_UNAVAILABLE = 31,
   SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED = 32,
   SA_AIS_ERR_CAMPAIGN_PROC_FAILED = 33,
   SA_AIS_ERR_CAMPAIGN_CANCELED = 34,
   SA_AIS_ERR_CAMPAIGN_FAILED = 35,
   SA_AIS_ERR_CAMPAIGN_SUSPENDED = 36,
   SA_AIS_ERR_CAMPAIGN_SUSPENDING = 37,
   SA_AIS_ERR_ACCESS_DENIED = 38,
   SA_AIS_ERR_NOT_READY = 39,
   SA_AIS_ERR_DEPLOYMENT = 40
} SaAisErrorT;

typedef enum {
    SA_SVC_HPI  =  1,
    SA_SVC_AMF  =  2,
    SA_SVC_CLM  =  3,
    SA_SVC_CKPT =  4,
    SA_SVC_EVT  =  5,
    SA_SVC_MSG  =  6,
    SA_SVC_LCK  =  7,
    SA_SVC_IMMS =  8, 
    SA_SCV_LOG  =  9,
    SA_SVC_NTF  =  10,
    SA_SVC_NAM  =  11,
    SA_SVC_TMR  =  12,
    SA_SVC_SMF  =  13,
    SA_SVC_SEC  =  14,
    SA_SVC_PLM  =  15
} SaServicesT;

typedef struct {
   SaSizeT   bufferSize;
   SaUint8T  *bufferAddr;
} SaAnyT;

typedef struct {
    SaUint16T length;
    SaUint8T value[SA_MAX_NAME_LENGTH];
} SaNameT;

typedef struct {
    SaUint8T releaseCode;
    SaUint8T majorVersion;
    SaUint8T minorVersion;
} SaVersionT;

typedef union {
    SaInt64T int64Value;
    SaUint64T uint64Value;
    SaTimeT timeValue;
    SaFloatT floatValue;
    SaDoubleT doubleValue;
} SaLimitValueT;

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_AIS_H */

