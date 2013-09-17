/*******************************************************************************
 * ModuleName  : include                                                       
 * $File: //depot/dev/main/Andromeda/Yamuna/ASP/components/timer/newTimerPrototype/clCommon.h $
 * $Author: karthick $
 * $Date: 2007/01/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This header file contains all the common data types used across SISP
 *
 *
 *****************************************************************************/

#ifndef _CL_COMMON_H_
#define _CL_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Clovis Data Types 
 *****************************************************************************/
#define CL_TRUE    	1
#define CL_FALSE   	0
#define CL_YES     	1
#define CL_NO      	0
    
#define CL_ENABLE   1
#define CL_DISABLE  0

#define CL_MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define CL_MAX(a,b) ( (a) > (b) ? (a) : (b) )

/* Following marcos assume integers - char, short, int and long as argument */
#define CL_ROUNDUP(VAL,BASE)   (((VAL) + (BASE) - 1)/(BASE) * (BASE))
#define CL_ROUNDDOWN(VAL,BASE)  ((VAL)/(BASE) * (BASE))

/* The following macros returns the size of the given array */
#define CL_SIZEOF_ARRAY(__ArrayName__) sizeof((__ArrayName__)) / sizeof((__ArrayName__)[0])

/*
 * The following Macro assist in clearly defining arguments of an API
 */
#define CL_IN
#define CL_INOUT
#define CL_OUT

typedef unsigned long long  ClUint64T __attribute__((__aligned__(8))); 
typedef signed long long    ClInt64T  __attribute__((__aligned__(8)));
typedef unsigned int        ClUint32T;
typedef signed int          ClInt32T;
typedef unsigned short      ClUint16T;
typedef signed short        ClInt16T;
typedef unsigned char       ClUint8T;
typedef signed char         ClInt8T;
typedef char                ClCharT; /* Use this for strings */
typedef signed int          ClFdT;   /* Use this for file descriptors */


typedef ClUint16T           ClBoolT;
typedef ClInt64T            ClTimeT;
#if __WORDSIZE == 64
typedef ClUint64T           ClHandleT;
#else
typedef ClUint32T            ClHandleT; /* FIXME: This isn't SAF compliant yet */
#endif
typedef ClUint64T           ClSizeT;
typedef ClUint64T           ClOffsetT;
typedef ClUint64T           ClInvocationT;
typedef ClUint64T           ClSelectionObjectT;
typedef ClUint64T           ClNtfIdentifierT;
typedef ClInt8T             *ClAddrT;
typedef void*               ClPtrT;
typedef unsigned long       ClWordT;

/* Clovis return code type  */
typedef ClUint32T       ClRcT;

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


#define CL_MAX_NAME_LENGTH  256
typedef struct {
    SaUint16T length;
    /** Actual name represented as a null terminated ASCII string */
    SaUint8T value[CL_MAX_NAME_LENGTH];
} SaNameT;

/* Version Information for various services */
typedef struct {
    ClUint8T  releaseCode;  /* single ASCII capitol letter "A-Z" */
    ClUint8T  majorVersion; /* Major Number in range of [01-255] */
    ClUint8T  minorVersion; /* Minor Number in range of [01-255] */
} ClVersionT;

typedef enum {
    CL_DISPATCH_ONE         = 1,
    CL_DISPATCH_ALL         = 2,
    CL_DISPATCH_BLOCKING    = 3,
} ClDispatchFlagsT;

/*****************************************************************************
 *  Bit related Types 
 *****************************************************************************/
#define CL_TIME_END             0x7fffffffffffffff
#define CL_FORCED_TO_8BITS      0xff
#define CL_FORCED_TO_16BITS     0xffff
#define CL_FORCED_TO_32BITS     0xffffffff
#define CL_BITS_PER_BYTE        8
#define CL_BIT(X)               (0x1 << (X))


/*****************************************************************************
 *  Clovis Component Ids 
 *****************************************************************************/
typedef enum {
    CL_CID_UNSPECIFIED  = 0,        /* 0x0  Start */
    CL_CID_OSAL         = 1,        /* 0x01 OS Abstraction Layer */
    CL_CID_HAL          = 2,        /* 0x02 Hardware Abstraction Layer */
    CL_CID_DBAL         = 3,        /* 0x03 Database Abstraction Layer */
    CL_CID_EO           = 4,        /* 0x04 Execution Object */
    CL_CID_IOC          = 5,        /* 0x05 Intelligent Object Communication */
    CL_CID_RMD          = 6,        /* 0x06 Remote Method Dispatch */
    CL_CID_NAMES        = 7,        /* 0x07 Name Service */
    CL_CID_TIMER        = 8,        /* 0x08 Timer */
    CL_CID_SHM          = 9,        /* 0x09 Shared Memory Support */
    CL_CID_DSHM         = 10,       /* 0x0a Distributed Shared Memory */
    CL_CID_LOG          = 11,       /* 0x0b Logging */
    CL_CID_TRACE        = 12,       /* 0x0c Tracing */
    CL_CID_DIAG         = 13,       /* 0x0d Diagnostics */
    CL_CID_DEBUG        = 14,       /* 0x0e Debug */
    CL_CID_CPM          = 15,       /* 0x0f Component Management */
    CL_CID_CAP          = 16,       /* 0x10 Capability Management */
    CL_CID_RES          = 17,       /* 0x11 Resource Management */
    CL_CID_GMS          = 18,       /* 0x12 Group Membership Service */
    CL_CID_EVENTS       = 19,       /* 0x13 Event Messaging */
    CL_CID_DLOCK        = 20,       /* 0x14 Distributed Locking */
    CL_CID_TXN          = 21,       /* 0x15 Transactions */
    CL_CID_CKPT         = 22,       /* 0x16 Checkpointing */
    CL_CID_COR          = 23,       /* 0x17 Clovis Object Registry */
    CL_CID_CNT          = 24,       /* 0x18 Containers */
    CL_CID_DCNT         = 25,       /* 0x19 Distributed Containers */
    CL_CID_RCNT         = 26,       /* 0x1a Resilient Containers */
    CL_CID_ALARMS       = 27,       /* 0x1b Alarm */
    CL_CID_POLICY       = 28,       /* 0x1c Policy Engine */
    CL_CID_RULE         = 29,       /* 0x1d Rule Base Engine */
    CL_CID_SCRIPTING    = 30,       /* 0x1e Scripting Engine */
    CL_CID_CM           = 31,       /* 0x1f Chassis Manager */
    CL_CID_HPI          = 32,       /* 0x20 Hardware Plateform Interface */
    CL_CID_FAULTS       = 33,       /* 0x21 Fault Management */
    CL_CID_AMS          = 34,       /* 0x22 Availability Management */
    CL_CID_MED          = 35,       /* 0x23 Mediation */
    CL_CID_BUFFER       = 36,       /* 0x24 Buffer Management */
    CL_CID_QUEUE        = 37,       /* 0x25 Queue Management */
    CL_CID_CLIST        = 38,       /* 0x26 Circular List Management */
    CL_CID_SNMP         = 39,       /* 0x27 SNMP Agent */
    CL_CID_NS           = 40,       /* 0x28 Name Service */
    CL_CID_OM           = 41,       /* 0x29 Object Manger */
    CL_CID_POOL         = 42,       /* 0x2a Pool management */
    CL_CID_CD           = 43,       /* 0x2b Common Diagnostics */
    CL_CID_DM           = 44,       /* 0x2c Diagnostics Manager */
    CL_CID_OAMP_RT      = 45,       /* 0x2d OAMP RT parser */
    CL_CID_PROV         = 46,       /* 0x2e provisioning Manger */    
    CL_CID_UM           = 47,       /* 0x2f upgrade manager */
    CL_CID_HANDLE       = 48,       /* 0x30 handle database */
    CL_CID_VERSION      = 49,       /* 0x31 version checker library */
    CL_CID_XDR          = 50,       /* 0x32 xdr library */
    CL_CID_IDL          = 51,       /* 0x33 IDL */
    CL_CID_HEAP         = 52,       /* 0x34 HEAP */
    CL_CID_MEM          = 53,
    CL_CID_PARSER       = 54,
    /* Add more CID here if required */

    CL_CID_MAX      /* This will help Validate if need be */
                                    
} ClCompIdT;

typedef struct ClWaterMark
{
    ClUint32T lowLimit;
    ClUint32T highLimit;
} ClWaterMarkT;

typedef enum
{
    CL_WM_LOW_LIMIT = 0,
    CL_WM_HIGH_LIMIT = 1,
}
ClEoWaterMarkFlagT;

typedef enum{
    CL_WM_LOW  = 0,
    CL_WM_HIGH = 1,
    CL_WM_MED  = 2,
    CL_WM_MAX
}ClWaterMarkIdT;

#define CL_EO_ACTION_CUSTOM  (1<<0)
#define CL_EO_ACTION_EVENT   (1<<1)
#define CL_EO_ACTION_LOG     (1<<2)
#define CL_EO_ACTION_NOT     (1<<3)
#define CL_EO_ACTION_MAX     (1<<32)     // ClUnt8T type bieng used for bitMap 

typedef struct ClEoActionInfo {
    ClUint32T     bitMap;      /*The bitmap of functions that are enabled.*/
} ClEoActionInfoT;

/**
 * The argument list that can be provided to the custom action taken
 * on water mark hit.
 */
typedef ClPtrT* ClEoActionArgListT;

#define CL_DEPRECATED __attribute__((__deprecated__))

#ifdef __cplusplus
}
#endif

#endif /* _CL_COMMON_H_ */
