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
 * File        : xdrClVersionTUnMarshall.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#include "clXdrApi.h"

        
ClRcT clXdrUnmarshallClVersionT(ClBufferHandleT msg , void* pGenVar)
{
    ClVersionT* pVar = (ClVersionT*)pGenVar;
    ClRcT rc = CL_OK;

    if ((void*)0 == pVar)
    {
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }


    rc = clXdrUnmarshallClUint8T(msg,&(pVar->releaseCode));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint8T(msg,&(pVar->majorVersion));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint8T(msg,&(pVar->minorVersion));
    if (CL_OK != rc)
    {
        return rc;
    }


    return rc;
}

