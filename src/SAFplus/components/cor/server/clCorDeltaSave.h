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
 * ModuleName  : cor
 * File        : clCorDeltaSave.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Cor Delta Save.
 *
 *
 *****************************************************************************/


#ifndef _CL_COR_DELTA_SAVE_H_
#define _CL_COR_DELTA_SAVE_H_

#include "clCommon.h"
#include <clCorMetaData.h>
#include <clDbalApi.h>
#include <clCorClient.h>

/* internal */
#include "clCorVector.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************************/
/* APIs which interact with Hash Table and Dbal */
/***************************************************************************************/

#define CL_COR_DFLT_CLASS_SIZE 1
#define DELTA_SAVE_NUM_BUCKETS CL_COR_HANDLE_MAX_DEPTH*3  

typedef enum ClCorDeltaSaveType {
	CL_COR_DELTA_MO_CREATE = 0,
	CL_COR_DELTA_MSO_CREATE,
	CL_COR_DELTA_SET,
	CL_COR_DELTA_DELETE,
	CL_COR_DELTA_ROUTE_CREATE,
	CL_COR_DELTA_ROUTE_STATUS_SET,
	CL_COR_DELTA_ROUTE_DELETE		
}ClCorDeltaSaveTypeT;

/* This is the key for saving data into data base. On boot up, this value is read and 
 * and is stored on a has table. The hash table is sorted by depth of MOID. For each
 * depth, an entry for MOCREATE, MSOCREATE AND SET will be present. This would be used
 * to recreate the MO Object Tree. Essentially, 2*moid.depth +  flag (flag = MOCREATE, MSOCREATE,SET).
 */
typedef struct ClCorDeltaSaveKey {
	ClVersionT	version;
	ClCorMOIdT moId;
	ClCorAttrPathT attrPath;
	ClCorDeltaSaveTypeT flag;
}ClCorDeltaSaveKeyT;

/* SRID: This should be changed to ClCorDeltaContData */
typedef struct ClCorDeltaContData {
	ClUint32T dataSize;
	void *data;
}ClCorDeltaContDataT;

typedef struct ClCorDeltaRouteSaveKey {
	ClVersionT	version;
	ClCorMOIdT moId;
	ClIocNodeAddressT nodeAddr;
    ClIocPortT  portId; 
}ClCorDeltaRouteSaveKeyT;

/* This Api stores the information of delta transaction */
ClRcT clCorDeltaDbRouteInfoStore(ClCorMOIdT moId, corRouteApiInfo_t routeInfo, ClCorDeltaSaveTypeT flag);
extern ClRcT clCorDeltaDbStore(ClCorMOIdT moId, ClCorAttrPathPtrT attrPath, ClCorDeltaSaveTypeT flag);
ClRcT clCorDeltaDbAttrPathOp(ClCorMOIdT moId, ClInt32T flag);

/* APIs which will restores the information from db to main memory */
ClRcT clCorDeltaDbRestore(void);
ClRcT clCorDeltaObjDbCreateCont() ;
ClRcT clCorDeltaContInit(void);

/* This API will clean up the tables that already exist */
void clCorDeltaSaveTablesCleanUp(void);

/* These APIs will restores the Delta Db (route and Obj) on the Slave side */
ClRcT ClCorDeltaDbSlaveCreate(void);
void clCorDeltaDbRecreate(void* pMoId, ClBufferHandleT buffer);
ClRcT clCorDeltaDbRouteRecreate(void);

/* Delta Dbs Open/close APIs */
ClRcT clCorDeltaDbsOpen(ClDBTypeT deltaDbFlag);
ClRcT clCorDeltaDbsClose() ;

/* Version Check Macro */
#define clCorDbVersionValidate(version, rc) \
		do\
		{ \
			if((rc = clVersionVerify(&gCorDbVersionDb, (&version) )) != CL_OK) \
			{ \
			    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to validate the Delta Db version . rc[0x%x]", rc)); \
			} \
		}while(0) 

#endif    /* _CL_COR_DELTA_SAVE_H_ */
