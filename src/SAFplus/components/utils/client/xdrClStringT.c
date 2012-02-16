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
 * File        : xdrClStringT.c
 *******************************************************************************/

#include <clXdrApi.h>

ClRcT clXdrMarshallClStringT(void* pGenVar, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClStringT *pVar = (ClStringT *)pGenVar;
    ClRcT rc         = CL_OK;
    ClUint32T length = 0;    

    if ((void*)0 == pVar)
    {
        clXdrMarshallClUint32T(&length, msg, 0);
    }
    else
    {
        length = 1;
        clXdrMarshallClUint32T(&length, msg, 0);

        rc = clXdrMarshallClUint32T(&(pVar->length),msg,isDelete);
        if (CL_OK != rc)
        {
            return rc;
        }
 
        rc = clXdrMarshallPtrClCharT(pVar->pValue, pVar->length,msg,isDelete);
        if (CL_OK != rc)
        {
            return rc;
        }

    }

    return rc;
}

ClRcT clXdrUnmarshallClStringT(ClBufferHandleT msg , void* pGenVar)
{
    ClStringT *pVar = (ClStringT *)pGenVar;
    ClRcT     rc     = CL_OK;
    ClUint32T length = 0;

    if ((void*)0 == pVar)
    {
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    clXdrUnmarshallClUint32T(msg, &length);
    if( 0 == length)
    {
        pGenVar = NULL;
    }
    else
    {

        rc = clXdrUnmarshallClUint32T(msg,&(pVar->length));
        if (CL_OK != rc)
        {
            return rc;
        }

        rc = clXdrUnmarshallPtrClCharT(msg,(void**)&(pVar->pValue),pVar->length);
        if (CL_OK != rc)
        {
            return rc;
        }

    }

    return rc;
}
