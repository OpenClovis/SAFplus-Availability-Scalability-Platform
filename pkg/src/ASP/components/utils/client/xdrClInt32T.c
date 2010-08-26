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
 * File        : xdrClInt32T.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>
#include <clDebugApi.h>

/**** ClInt32T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClInt32T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    ClInt32T temp = htonl(*(ClInt32T*)pPyld);

    if (msg)
    {
        CL_XDR_PRINT(("num: %x, %x\n", *(ClInt32T*)pPyld, isDelete))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClInt32T));
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClInt32T( ClBufferHandleT msg , void* pPyld)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    if (msg)
    {
        ClInt32T temp1 = 0;
        ClUint32T temp = sizeof(ClInt32T);

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);
        if (temp != sizeof(ClInt32T))
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            *(ClUint32T*)pPyld = 0;  /* At least it isn't random */
            return rc;
        }        

        *(ClInt32T*)pPyld = (ClInt32T)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(ClInt32T*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/
