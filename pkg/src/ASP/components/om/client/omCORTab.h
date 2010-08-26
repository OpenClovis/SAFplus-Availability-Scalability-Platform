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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : om
 * File        : omCORTab.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _OM_COR_TAB_H_
#define _OM_COR_TAB_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <clCorMetaData.h>

#define MOID_TO_OMH_MAX_BUCKETS		512
#define OMH_TO_MOID_MAX_BUCKETS		MOID_TO_OMH_MAX_BUCKETS
#define HASH_SHIFT					1


/* MO ID header length */
#define COR_MOID_HDR_SIZE			(sizeof(ClCorMOIdT) - \
									(sizeof(ClCorMOHandleT) * CL_COR_HANDLE_MAX_DEPTH))
/* Get the length of MO handles at different levels */
#define COR_CURR_MOH_SIZE(hMoId)	((hMoId)->depth * sizeof(ClCorMOHandleT))

typedef struct omCorLkupTbl
	{
	/* hash table for MO ID to OM Handle */
    ClCntHandleT moIdToOmHHashTbl; 
	/* hash table for OM Handle to MO ID */
    ClCntHandleT omHToMoIdHashTbl; 
	} omCorLkupTbl_t;

typedef struct omCORTabEntry {
	ClHandleT     omHandle;
	ClCorObjectHandleT corHandle;

} omCORTabEntry_t;

/* Internal APIs */

#ifdef __cplusplus
}
#endif

#endif  /* _OM_COR_TAB_H_ */

