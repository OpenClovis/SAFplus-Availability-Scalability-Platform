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
 * ModuleName  : include                                                       
 * File        : clCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This header file contains all the common data types used across ASP
 *
 *
 *****************************************************************************/

/**
 * \file
 * \brief Typical defines found in any software project
 * \ingroup common_apis
 *
 */

/**
 * \addtogroup common_apis
 * \{
 */

#ifndef _CL_COMMON_H_
#define _CL_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clArchHeaders.h>    

/******************************************************************************
 *  Clovis Data Types 
 *****************************************************************************/

/** define the Truth */
#define CL_TRUE    	1
/** define the False */
#define CL_FALSE   	0

/** define Yes */
#define CL_YES     	1
/** define No */
#define CL_NO      	0

/** define Enable */    
#define CL_ENABLE   1
/** define Disable */    
#define CL_DISABLE  0

/** 
 * These define all the possible states that a component/node can be in.
 */
typedef enum {
    /**
     * This determines that the node/component is down.
     */
    CL_STATUS_DOWN = 0,

    /**
     * This determines that the node/component is up.
     */
    CL_STATUS_UP   = 1,
} ClStatusT;



/** Mininum macro */
#define CL_MIN(a,b) ( (a) < (b) ? (a) : (b) )
/** Maximum macro */
#define CL_MAX(a,b) ( (a) > (b) ? (a) : (b) )

/** Roundup macro assumes integers - char, short, int and long as argument */
#define CL_ROUNDUP(VAL,BASE)   (((VAL) + (BASE) - 1)/(BASE) * (BASE))
/** Rounddown macro assumes integers - char, short, int and long as argument */
#define CL_ROUNDDOWN(VAL,BASE)  ((VAL)/(BASE) * (BASE))

/** The following macros returns the number of items in the given array */
#define CL_SIZEOF_ARRAY(__ArrayName__) sizeof((__ArrayName__)) / sizeof((__ArrayName__)[0])

/** CL_IN macro assists in clearly defining arguments of an API, but has no actual meaning */
#define CL_IN
/** CL_INOUT macro assists in clearly defining arguments of an API, but has no actual meaning */
#define CL_INOUT
/** CL_OUT macro assists in clearly defining arguments of an API, but has no actual meaning */
#define CL_OUT

/* Note: for the 64 bit types, align yourself using "__attribute__((__aligned__(8)))".
   We cannot align automatically as part of this typedef, since SAF does not align its 
   corresponding type. See bug 7155  */
typedef unsigned long long  ClUint64T; 
typedef signed long long    ClInt64T;  

typedef unsigned int        ClUint32T;
typedef signed int          ClInt32T;
typedef unsigned short      ClUint16T;
typedef signed short        ClInt16T;
typedef unsigned char       ClUint8T;
typedef signed char         ClInt8T;
typedef char                ClCharT; /* Use this for strings */
typedef signed int          ClFdT;   /* Use this for file descriptors */
/* 
 * Special type with word size for portability b/n 32 & 64 bit 
 * Recommended type would be intptr_t but not working for some
 * reason. Need to fix this - Mynk
 */
typedef unsigned long       ClWordT;


typedef ClUint16T           ClBoolT;

typedef ClWordT             ClPidT;

  /** nanoseconds in a microsecond */
#define CL_MICRO_TO_NANO  1000ULL
  /** microseconds in a millisecond */
#define CL_MILLI_TO_MICRO 1000ULL
  /** milliseconds in a second */
#define CL_SEC_TO_MILLI   1000ULL
  /** nanoseconds in a millisecond */
#define CL_MILLI_TO_NANO  (CL_MICRO_TO_NANO * CL_MILLI_TO_MICRO)
  /** nanoseconds in a second */
#define CL_SEC_TO_NANO    (CL_MILLI_TO_NANO * CL_SEC_TO_MILLI)
  /** In practice, this will never time out, since it is 292 years */
#define CL_TIME_END             0x7fffffffffffffffULL
  /** In practice, this will never time out, since it is 292 years */
#define CL_TIME_FOREVER         0x7fffffffffffffffULL

  /** Time duration specified in nanoseconds */
typedef ClInt64T            ClTimeT;
typedef ClUint64T           ClHandleT;
  /** # of bytes in a buffer or object */
typedef ClUint64T           ClSizeT;
  /** Offset of a buffer or object within another  */
typedef ClInt64T            ClOffsetT;
typedef ClUint64T           ClInvocationT;
typedef ClUint64T           ClSelectionObjectT;
typedef ClUint64T           ClNtfIdentifierT;
typedef ClInt8T             *ClAddrT;
typedef void*               ClPtrT;

  /** Clovis return code type. 
      \sa clCommonErrors.h  */
typedef ClUint32T       ClRcT;

/**
 ************************************
 * \brief Definition of a generic single argument callback function.
 *
 * \param invocation A "cookie". The caller can pass a piece of arbitrary data
 * along with the function pointer in all calls that take a callback.  This data
 * passed as the "invocation" parameter to the callback.
 *
 */
typedef ClRcT (*ClCallbackT) (CL_IN ClPtrT invocation);

typedef union cl_u64_u /* FIXME: This is not endian-safe; shouldn't be used */
    {
    struct {
        ClUint32T high;
        ClUint32T low;
        } DWord;
    ClUint32T dWords[2];
    ClUint16T words[4];
    ClInt8T bytes[8];
    } ClUnion64T;


  /** The Maximum length of most string names in the OpenClovis ASP framework */
#define CL_MAX_NAME_LENGTH  256

  /** A name */
typedef struct {
    /** Length of the name in bytes excluding '\0' */
    ClUint16T       length;
    /** Actual name represented as a null terminated ASCII string */
    ClCharT         value[CL_MAX_NAME_LENGTH];
} ClNameT;

  /** \brief  Load the ClNameT structure.
      \param  name The structure you want to load
      \param  str  The value to be put into the ClNameT structure

      If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
   */
void clNameSet(ClNameT* name, const char* str);

  /** \brief  Load the ClNameT structure.
      \param  name The structure you want to load
      \param  name The structure to be put into the ClNameT structure

      If length is too long, then this function will ASSERT in debug mode, and crop in production mode 
   */
void clNameCopy(ClNameT* nameOut, const ClNameT *nameIn);

  /** \brief  Join ClNameT structures
      \param  nameOut The result
      \param  prefix The beginning string.  Pass NULL if there is no beginning
      \param  separator The middle string. Pass NULL for no separator
      \param  suffix The ending string. Pass NULL for no ending

      If the sum of the lengths of the prefix, separator, and suffix is too long, the function will crop.
   */
void clNameConcat(ClNameT* nameOut, const ClNameT *prefix, const char* separator, const ClNameT *suffix);

  /** \brief  Duplicate a string
      \param  str The string to be duplicated
      \retval Storage pointed to a duplicated string or NULL
     
      \par Description:
      This API is used to duplicate a string. The storage pointed by the returned string
      should be freed using ASP heap API: clHeapFree.
   */

ClCharT *clStrdup(const ClCharT *str);

ClBoolT clParseEnvBoolean(ClCharT* envvar);

ClInt32T clCreatePipe(ClInt32T fds[2], ClUint32T numMsgs, ClUint32T msgSize);

ClUint32T clBinaryPower(ClUint32T size);

  /** Version Information for various services */
typedef struct {
    /** single ASCII capitol letter "A-Z" */
    ClUint8T  releaseCode;
    /** Major Number in range of [01-255] */
    ClUint8T  majorVersion;
    /** Minor Number in range of [01-255] */
    ClUint8T  minorVersion;
} ClVersionT;

  /** Dispatch flags */
typedef enum {
    CL_DISPATCH_ONE         = 1,
    CL_DISPATCH_ALL         = 2,
    CL_DISPATCH_BLOCKING    = 3,
} ClDispatchFlagsT;

/*****************************************************************************
 *  Bit related Types 
 *****************************************************************************/
#define CL_FORCED_TO_8BITS      0xff
#define CL_FORCED_TO_16BITS     0xffff
#define CL_FORCED_TO_32BITS     0xffffffff
#define CL_BITS_PER_BYTE        8
#define CL_BIT(X)               (0x1 << (X))

#if 0
typedef enum ClMetricId
{
    CL_METRIC_ALL,
    CL_METRIC_CPU,
    CL_METRIC_MEM,
    CL_METRIC_MAX,
}ClMetricIdT;

typedef struct ClMetric
{
    const ClCharT *pType;
    ClMetricIdT id;
    ClUint32T maxThreshold;
    ClUint32T currentThreshold;
    ClUint32T maxOccurences;
    ClUint32T numOccurences;
}ClMetricT;

#define CL_METRIC_STR(id) \
    ((id) == CL_METRIC_ALL) ? "all" :           \
    ((id) == CL_METRIC_CPU) ? "cpu" :           \
    ((id) == CL_METRIC_MEM) ? "mem" :           \
    "unknown"
#endif    

/**
 *  Clovis Component Ids 
 */
typedef enum {
    /** Unspecified */
    CL_CID_UNSPECIFIED  = 0x0,

    /** OS Abstraction Layer */    
    CL_CID_OSAL         = 0x01,
    
    /** Hardware Abstraction Layer */
    CL_CID_HAL          = 0x02,
    
    /** Database Abstraction Layer */    
    CL_CID_DBAL         = 0x03,
    
    /** Execution Object */    
    CL_CID_EO           = 0x04,
    
    /** Intelligent Object Communication */   
    CL_CID_IOC          = 0x05,
    
    /** Remote Method Dispatch */    
    CL_CID_RMD          = 0x06,
    
    /** Name Service */    
    CL_CID_NAMES        = 0x07,
    
    /** Timer */
    CL_CID_TIMER        = 0x08,
    
    /** Shared Memory Support */
    CL_CID_SHM          = 0x09,
    
    /** Distributed Shared Memory */
    CL_CID_DSHM         = 0x0a,
    
    /** Logging */
    CL_CID_LOG          = 0x0b,
    
    /** Message Service */
    CL_CID_MSG          = 0x0c,
    
    /** Diagnostics */
    CL_CID_DIAG         = 0x0d,
    
    /** Debug */
    CL_CID_DEBUG        = 0x0e,
    
    /** Component Management */
    CL_CID_CPM          = 0x0f,
    
    /** Capability Management (for future use) */
    CL_CID_CAP          = 0x10,
    
    /** Resource Management (for future use) */
    CL_CID_RES          = 0x11,
    
    /** Group Membership Service */
    CL_CID_GMS          = 0x12,
    
    /** Event Service */
    CL_CID_EVENTS       = 0x13,
    
    /** Distributed Locking (for future use) */    
    CL_CID_DLOCK        = 0x14,
    
    /** Transactions */    
    CL_CID_TXN          = 0x15,
    
    /** Checkpointing Service*/    
    CL_CID_CKPT         = 0x16,
    
    /** Clovis Object Registry */    
    CL_CID_COR          = 0x17,
    
    /** Containers */    
    CL_CID_CNT          = 0x18,
    
    /** Distributed Containers (for future use) */    
    CL_CID_DCNT         = 0x19,
    
    /** Resilient Containers (for future use) */    
    CL_CID_RCNT         = 0x1a,
    
    /** Alarm Manager */    
    CL_CID_ALARMS       = 0x1b,
    
    /** Policy Engine */    
    CL_CID_POLICY       = 0x1c,
    
    /** Rule Base Engine */    
    CL_CID_RULE         = 0x1d,
    
    /** Scripting Engine (for future use) */    
    CL_CID_SCRIPTING    = 0x1e,
    
    /** Chassis Manager */    
    CL_CID_CM           = 0x1f,
    
    /** Hardware Platform Interface */    
    CL_CID_HPI          = 0x20,
    
    /** Fault Management */    
    CL_CID_FAULTS       = 0x21,
    
    /** Availability Management Service*/    
    CL_CID_AMS          = 0x22,
    
    /** Mediation Library */    
    CL_CID_MED          = 0x23,
    
    /** Buffer Management */    
    CL_CID_BUFFER       = 0x24,
    
    /** Queue Management */    
    CL_CID_QUEUE        = 0x25,
    
    /** Circular List Management */    
    CL_CID_CLIST        = 0x26,
    
    /** SNMP Agent */    
    CL_CID_SNMP         = 0x27,
    
    /** Name Service */    
    CL_CID_NS           = 0x28,
    
    /** Object Manager */    
    CL_CID_OM           = 0x29,
    
    /** Pool Management */    
    CL_CID_POOL         = 0x2a,
    
    /** Common Diagnostics (for future use) */    
    CL_CID_CD           = 0x2b,
    
    /** Diagnostics Manager (for future use) */    
    CL_CID_DM           = 0x2c,
    
    /** OAMP RT parser */    
    CL_CID_OAMP_RT      = 0x2d,
    
    /** Provisioning Manager */        
    CL_CID_PROV         = 0x2e,
    
    /** Upgrade Manager (for future use) */    
    CL_CID_UM           = 0x2f,
    
    /** Handle Database */    
    CL_CID_HANDLE       = 0x30,
    
    /** Version Checker Library */    
    CL_CID_VERSION      = 0x31,
    
    /** XDR Library */   
    CL_CID_XDR          = 0x32,
    
    /** IDL */   
    CL_CID_IDL          = 0x33,
    
    /** Heap Management */    
    CL_CID_HEAP         = 0x34,
    
    /** Memory Management */        
    CL_CID_MEM          = 0x35,
    
    /** Parser */            
    CL_CID_PARSER       = 0x36,
    
    CL_CID_BACKING_STORAGE = 0x37,

    CL_CID_JOB          = 0x38,
    CL_CID_JOBQUEUE     = 0x38,

    CL_CID_THREADPOOL   = 0x39,
    CL_CID_TASKPOOL     = 0x39,

    /** Bitmap Management */    
    CL_CID_BITMAP       = 0x3a,

    
    /* Add more CID here if required */
    CL_CID_LEAKY_BUCKET = 0x3b,

    /** Mso Services Management */
    CL_CID_MSO          = 0x3c,

    /** Performance Management */ 
    CL_CID_PM          = 0x3d,

    /** SAF Notification service */
    CL_CID_NF          = 0x3e,

    /** This will help validate if needs to be */
    CL_CID_MAX 
                                    
} ClCompIdT;

typedef struct ClWaterMark
{
    ClUint64T lowLimit;
    ClUint64T highLimit;
} ClWaterMarkT;

typedef enum
{
    CL_WM_LOW_LIMIT = 0,
    CL_WM_HIGH_LIMIT = 1,
}
ClEoWaterMarkFlagT;

typedef enum{
    CL_WM_LOW,
    CL_WM_HIGH,
    CL_WM_MED,
    CL_WM_SENDQ,
    CL_WM_RECVQ,
    CL_WM_MAX
}ClWaterMarkIdT;

#define CL_WEAK __attribute__((weak))

#define CL_EO_ACTION_CUSTOM  (1<<0)
#define CL_EO_ACTION_EVENT   (1<<1)
#define CL_EO_ACTION_LOG     (1<<2)
#define CL_EO_ACTION_NOT     (1<<3)
#define CL_EO_ACTION_MAX     (1<<31)     // ClUnt8T type bieng used for bitMap 

typedef struct ClEoActionInfo {
    ClUint32T     bitMap;      /**<The bitmap of functions that are enabled.*/
} ClEoActionInfoT;

/**
 * The argument list that can be provided to the custom action taken
 * on water mark hit.
 */
typedef ClPtrT* ClEoActionArgListT;

typedef struct
{
    ClUint32T  length;
    ClCharT    *pValue;
}ClStringT;

extern ClStringT *clStringDup(const ClStringT *);

/* Macro to print into the name */
  /** \brief  Load the ClNameT structure.
      \param  name The structure you want to load
      \param  fmtString & params 
   */
#define clNamePrintf(name, ...)       \
do                                    \
{                                     \
    name.length = snprintf(name.value, CL_MAX_NAME_LENGTH - 1, __VA_ARGS__);\
    name.value[CL_MIN(name.length, CL_MAX_NAME_LENGTH - 1)] = '\0';         \
}while(0)

#ifdef __GNUC__
#define CL_DEPRECATED __attribute__((__deprecated__))
#define CL_PRINTF_FORMAT(fmtPos, argPos) __attribute__((format(printf, fmtPos, argPos)))
#else
#define CL_DEPRECATED
#define CL_PRINTF_FORMAT(fmtPos, argPos)
#endif

#ifdef __cplusplus
}
#endif

/**
 * \}
 */

#endif /* _CL_COMMON_H_ */
