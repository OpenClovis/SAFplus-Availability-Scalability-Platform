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
 * ModuleName  : utils
 * File        : xdrSaNameTMarshall.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/



#include "clXdrApi.h"

ClRcT clXdrMarshallSaNameT(void* pGenVar, ClBufferHandleT msg, ClUint32T isDelete)
{
    SaNameT* pVar = (SaNameT*)pGenVar;
    ClRcT rc;

    if ((void*)0 == pVar)
    {
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    
    if(pVar->length >= CL_MAX_NAME_LENGTH)
        pVar->length = CL_MAX_NAME_LENGTH;

    rc = clXdrMarshallClUint16T(&(pVar->length),msg,isDelete);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallArrayClCharT(pVar->value, pVar->length, msg, isDelete);
    if (CL_OK != rc)
    {
        return rc;
    }

    return rc;
}
