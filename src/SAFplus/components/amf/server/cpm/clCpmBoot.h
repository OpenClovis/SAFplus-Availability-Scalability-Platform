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

/**
 * This header file contains definitions of internal data types and
 * data structures used by CPM-BM submodule.
 */

#ifndef _CL_CPM_BOOT_H_
#define _CL_CPM_BOOT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP header files 
 */
#include <clCommon.h>

#include <clQueueApi.h>
#include <clEoApi.h>
#include <clParserApi.h>

/**
 * The process started in each boot level is defined
   in clCpmParser.c
 */
    /* AMF process running only */
#define CL_CPM_BOOT_LEVEL_0     0
    /* Log */
#define CL_CPM_BOOT_LEVEL_1     1
    /* GMS */
#define CL_CPM_BOOT_LEVEL_2     2
#define CL_CPM_BOOT_LEVEL_3     3
#define CL_CPM_BOOT_LEVEL_4     4
#define CL_CPM_BOOT_LEVEL_5     5
    /* #define CL_CPM_BOOT_LEVEL_6     6 */

typedef struct bootRow bootRowT;

/**
 * Linked list of component names.
 */
struct bootRow
{
    ClCharT compName[CL_MAX_NAME_LENGTH];
    bootRowT *pNext;
};

/**
 * Boot table data structure.
 */
typedef struct bootTable bootTableT;

struct bootTable
{
    /**
     * The bootLevel and listHead together with number of components
     * in this row represent one row.
     */
    ClUint32T bootLevel;
    ClUint32T numComp;
    bootRowT *listHead;
    bootTableT *pUp;
    bootTableT *pDown;
};

typedef struct cpmBM
{
    /**
     * Condition variable and mutex to use once starting up all the
     * components in a given boot level.
     */
    ClOsalCondIdT lcmCondVar;
    ClOsalMutexIdT lcmCondVarMutex;

    /**
     *  Indicates how many components are starting/shutting down
     *  successfully in the current boot level.
     */
    ClUint32T countComponent;

    /**
     * Condition variable and mutex for controlling the execution of
     * setlevel requests.
     */
    ClOsalCondIdT bmQueueCondVar;
    ClOsalMutexIdT bmQueueCondVarMutex;

    /**
     * Queue in which setlevel requests are stored.
     */
    ClQueueT setRequestQueueHead;

    ClUint32T maxBootLevel;
    ClUint32T defaultBootLevel;
    ClUint32T currentBootLevel;

    
    /**
     * Number of components in the current boot level.
     */
    ClUint32T numComponent;

    /**
     * Whether last booting(shutting down) was success or failure.
     */
    ClUint32T lastBootStatus;

    /**
     * This is the structure which holds boot manager related data of
     * clAmfConfig.xml.
     */
    bootTableT *table;

    ClTimerHandleT bmRespTimer;
}cpmBMT;


ClRcT cpmBmInitDS(void);

ClRcT cpmBmParseDeployConfigFile(ClParserPtrT configFile);

ClRcT cpmBmInitialize(ClOsalTaskIdT *pTaskId, ClEoExecutionObjT *pThis);

ClRcT cpmBmCleanupDS(void);

ClRcT cpmBmRespTimerCallback(ClPtrT unused);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_BOOT_H_ */
