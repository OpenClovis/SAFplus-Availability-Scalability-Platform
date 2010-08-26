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
 * ModuleName  : amf
 * File        : clAmsMgmtServerRmd.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the server side implementation of the AMS management
 * API.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clBufferApi.h>
#include <clAmsMgmtCommon.h>
#include <clOsalApi.h>
#include <clAmsErrors.h>
#include <clAmsMgmtServerRmd.h>
#include <clAmsServerUtils.h>

/*
 * _clAmsMgmtReadInputBuffer 
 * -------------------
 * Read the input buffer sent by the client 
 */

ClRcT 
_clAmsMgmtReadInputBuffer(
        CL_IN  ClBufferHandleT  in,
        CL_INOUT  ClUint8T  *inBuf,
        CL_IN  ClUint32T  reqInLen )
{

    ClUint32T  inLen = 0;

    AMS_CHECKPTR ( !inBuf );

    /* 
     * Read the input length 
     */

    AMS_CALL( clBufferLengthGet(in,&inLen) );

    /* 
     * Verify length matches expected length 
     */

    if ( inLen != reqInLen)
    {
        return CL_AMS_RC (CL_AMS_ERR_UNMARSHALING_FAILED);
    }

    /* 
     * Read the input data 
     */

    AMS_CALL( clBufferNBytesRead(in,inBuf,&inLen) );

    return CL_OK;

}

/*
 *_clAmsMgmtWriteGetEntityListBuffer 
 * -------------------
 * Send the buffer back to the server 
 *
 */

ClRcT 
_clAmsMgmtWriteGetEntityListBuffer(
        CL_OUT  ClBufferHandleT  out,
        CL_IN  clAmsMgmtEntityGetEntityListResponseT  *res )
{

#ifdef POST_RC2

    ClUint32T  i = 0;
    ClUint32T  numNodes = 0;
    ClAmsTLVT  tlv = {0};
    ClAmsEntityTypeT  entityType = 0;
    ClAmsEntityRefT  **nodeList = NULL;
    ClAmsEntityParamsT  *params = NULL;

    AMS_CHECKPTR ( !res );

    numNodes = res->numNodes;

    nodeList = ( ClAmsEntityRefT **)res->nodeList;

    if ( nodeList && numNodes)
    {
        entityType = ((ClAmsEntityRefT *)nodeList[0])->entity.type;
    }

    /*
     * Send the start of the list marker
     */

    tlv.type = CL_AMS_TLV_TYPE_START_LIST;
    tlv.length = numNodes;
   
    AMS_CALL( clBufferNBytesWrite(
                out,
                (ClUint8T *)&tlv,
                sizeof (ClAmsTLVT)) );

    /*
     * Send the list elements
     */

    AMS_CHECK_ENTITY_TYPE (entityType);

    AMS_CALL ( clAmsEntityGetDefaults(entityType, &params) );

    tlv.type = entityType;
    tlv.length = params->entitySize;

    for ( i = 0; i< numNodes; i++)
    {

        AMS_CALL( clBufferNBytesWrite(
                    out,
                    (ClUint8T *)&tlv,
                    sizeof (ClAmsTLVT)) );

        AMS_CALL( clBufferNBytesWrite(
                    out,
                    (ClUint8T *)nodeList[i]->ptr,
                    tlv.length) );

    }

    return CL_OK;

#else

    return CL_AMS_RC ( CL_ERR_NOT_IMPLEMENTED );

#endif

}
    
ClRcT
_clAmsMgmtWriteOutputBufferFindEntityByName (
        CL_OUT  ClBufferHandleT  out,
        CL_IN  ClAmsEntityRefT  *entityRef )
{

#ifdef POST_RC2

    ClAmsEntityT  *entity = NULL;
    ClAmsTLVT  tlv = {0};
    ClAmsEntityParamsT  *params = NULL;


    AMS_CHECKPTR ( !entityRef || !entityRef->ptr );
    
    entity = (ClAmsEntityT *)entityRef->ptr; 
    
    AMS_CHECK_ENTITY_TYPE (entity->type);

    AMS_CALL ( clAmsEntityGetDefaults(entity->type, &params) );

    tlv.length = params->entitySize;
    tlv.type = CL_AMS_TLV_TYPE_START_LIST;
    
    AMS_CALL( clBufferNBytesWrite(
                out,
                (ClUint8T*)&tlv,
                sizeof (ClAmsTLVT)));
    
    AMS_CALL( clBufferNBytesWrite( out,(ClUint8T*)entity,tlv.length) );

    return CL_OK;

#else

    return CL_AMS_RC ( CL_ERR_NOT_IMPLEMENTED );

#endif

}

