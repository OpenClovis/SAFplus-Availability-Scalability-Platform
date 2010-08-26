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
 * ModuleName  : utils
 * File        : xdrClInt64T.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>

/**** ClInt64T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClInt64T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ClInt32T lower = 0;
    ClInt32T upper = 0;
    ClInt32T *pTemp = (ClInt32T *)pPyld;
    CL_XDR_ENTER()

    lower = *pTemp;
    upper = *(pTemp+1);
 
    /*lower = (ClInt32T)(*(ClInt64T*)pPyld & 0xffffffff);
    upper = (ClInt32T)((*(ClInt64T*)pPyld >> 32) & 0xffffffff);*/

    if (*(ClUint8T*)&endianTestVar == 1)
    {
        /*upper & lower if little endian swap them*/
        upper ^= lower;
        lower ^= upper;
        upper ^= lower;
    }

    rc = clXdrMarshallClInt32T(&lower, msg, isDelete);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrMarshallClInt32T(&upper, msg, isDelete);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClInt64T( ClBufferHandleT msg, void* pPyld)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ClInt32T lower = 0;
    ClInt32T upper = 0;
    ClUint64T temp = 0;
    ClInt64T *pTemp = (ClInt64T *)pPyld;
    
    CL_XDR_ENTER()

    rc = clXdrUnmarshallClInt32T(msg,&lower);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrUnmarshallClInt32T(msg,&upper);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    if (*(ClUint8T*)&endianTestVar == 1)
    {
        /*upper & lower if little endian swap them*/
        upper ^= lower;
        lower ^= upper;
        upper ^= lower;
        temp = ((ClInt64T)upper << 32) | (ClInt64T)((ClUint32T)lower);
        *pTemp = temp;
         return rc;
    }
    temp = ((ClInt64T)lower << 32) | (ClInt64T)((ClUint32T)upper);
    *pTemp = temp;
    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/
