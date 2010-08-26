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
 * File        : xdrClUint64T.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>

/**** ClUint64T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClUint64T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ClUint32T lower = 0;
    ClUint32T upper = 0;
    ClUint32T *pTemp = (ClUint32T *)pPyld;
    CL_XDR_ENTER()

    lower = *pTemp;
    upper = *(pTemp+1);
  /* lower = (ClUint32T)(*(ClUint64T*)pPyld & 0xffffffff);
    upper = (ClUint32T)((*(ClUint64T*)pPyld >> 32) & 0xffffffff);*/
    if (*(ClUint8T*)&endianTestVar == 1)
    {
        /*upper & lower if little endian swap them*/
        upper ^= lower;
        lower ^= upper;
        upper ^= lower;
    }
    rc = clXdrMarshallClUint32T(&lower, msg, isDelete);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrMarshallClUint32T(&upper, msg, isDelete);
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
ClRcT clXdrUnmarshallClUint64T( ClBufferHandleT msg, void* pPyld)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ClUint32T lower = 0;
    ClUint32T upper = 0;
    ClUint64T *pTemp = (ClUint64T *)pPyld;
    CL_XDR_ENTER()

    rc = clXdrUnmarshallClUint32T(msg,&lower);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&upper);
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
       *pTemp = ((ClUint64T)upper << 32) | (ClUint64T)lower;
   /* (ClUint64T*)pPyld = ((ClUint64T)upper << 32) | (ClUint64T)lower;*/
        return rc;
    }
      
/*    (ClUint64T*)pPyld = ((ClUint64T)lower << 32) | (ClUint64T)upper;*/
    *pTemp = ((ClUint64T)lower << 32) | (ClUint64T)upper;
    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/
