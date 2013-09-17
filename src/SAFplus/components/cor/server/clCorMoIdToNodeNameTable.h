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
 * File        : clCorMoIdToNodeNameTable.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * MOId to Node Name live table.
 *
 *
 *****************************************************************************/


#ifndef _CL_COR_MOID_TO_NODE_NAME_TABLE_H_
#define _CL_COR_MOID_TO_NODE_NAME_TABLE_H_

#include "clCommon.h"
#include <clCorMetaData.h>

#ifdef __cplusplus
extern "C"
{
#endif

/***************************************************************************************/
/* APIs which interact with Hash table */
/***************************************************************************************/

/* This structure is used as data in case of NodenameToMoId hash table */
struct ClCorNodeData{
    SaNameT nodeName;
    ClCorMOIdPtrT pMoId;
};

typedef struct ClCorNodeData ClCorNodeDataT;
typedef ClCorNodeDataT * ClCorNodeDataPtrT;

/* API to create the tables that map MOId to Node Name */
/* This creates the hash tables */
extern ClRcT clCorMoIdNodeNameMapCreate(void);

/* This API adds an entry into the hash table for given MOId <-> node Name map */
/* This API shall be called when a new entry is to be added in the table. This typically */
/* happens only once during the booting of the system */
extern ClRcT clCorMOIdNodeNameMapAdd(ClCorMOIdPtrT pMoId, SaNameT *nodeName);

/* This API shall update the MOId corresponding to the specific node name. This is required */
/* when a card is plugged in or out. Whn a card is plugged in exact moId is stored. When */
/* the card is plugged out, moId with wildcard cardtype is added */
extern ClRcT clCorMoIdForNodeNameChange(ClCorMOIdPtrT pMoId, SaNameT *nodeName);

/* This API shall clean up the tables that already exist */
void clCorMoIdToNodeNameTablesCleanUp(void);

/* This API cleans up the table */
extern void clCorMoIdToNodeNameTableFinalize(void);


#endif    /* _CL_COR_MOID_TO_NODE_NAME_TABLE_H_ */
