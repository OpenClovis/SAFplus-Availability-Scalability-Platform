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
 * :-(
 */
#define CL_CPM_BOOT_LEVEL_0     0
#define CL_CPM_BOOT_LEVEL_1     1
#define CL_CPM_BOOT_LEVEL_2     2
#define CL_CPM_BOOT_LEVEL_3     3
#define CL_CPM_BOOT_LEVEL_4     4
#define CL_CPM_BOOT_LEVEL_5     5
#define CL_CPM_BOOT_LEVEL_6     6

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
