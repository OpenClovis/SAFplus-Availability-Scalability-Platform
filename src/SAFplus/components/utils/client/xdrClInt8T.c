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
 * File        : xdrClInt8T.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <string.h>
#include <netinet/in.h>
#include <clOsalApi.h>
#include <clXdrApi.h>
#include <clBufferApi.h>
#include <clDebugApi.h>

/**** ClInt8T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClInt8T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    ClInt32T temp = htonl(*(ClInt8T*)pPyld);

    if (msg)
    {
        CL_XDR_PRINT(("num: %x\n", *(ClInt32T*)pPyld))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClInt32T));
    }

    CL_XDR_ENTER()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClInt8T(ClBufferHandleT msg,void* pPyld)
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
        ClInt32T temp1=0;
        ClUint32T temp = sizeof(ClInt32T);

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);
        if (rc != CL_OK)
        {
            clDbgCodeError(rc,("Failed to read the data"));
            *(ClInt8T*)pPyld = 0;  /* At least it isn't random */
            CL_XDR_EXIT()
            return rc;
        }        

        if (temp != sizeof(ClInt32T))
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            *(ClInt8T*)pPyld = 0;  /* At least it isn't random */
            CL_XDR_EXIT()
            return rc;
        }        

        *(ClInt8T*)pPyld = (ClInt8T)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(ClInt32T*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallArrayClInt8T(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    
    if(0 != msg)
    {
        if(clBufferAppendHeap(msg, pPyld, count) != CL_OK)
        {
            rc = clBufferNBytesWrite(msg, (ClUint8T*)pPyld, count);
        }
    }
    CL_XDR_EXIT();
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallArrayClInt8T(ClBufferHandleT msg,void* pPyld, ClUint32T count)
{
    ClRcT rc = CL_OK;
    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }

    ClUint32T amtRead=count;
    
    if(0 != msg)
        rc = clBufferNBytesRead(msg, (ClUint8T*)pPyld, &amtRead);

    if (amtRead != count)
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
        }
    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

