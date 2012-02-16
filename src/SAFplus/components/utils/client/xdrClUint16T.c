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
 * File        : xdrClUint16T.c
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

/**** ClUint16T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClUint16T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    ClUint32T temp = htonl(*(ClUint16T*)pPyld);

    if (msg)
    {
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)pPyld))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClUint32T));
    }

    CL_XDR_ENTER()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClUint16T(ClBufferHandleT msg,void* pPyld)
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
        ClUint32T temp1 = 0;
        ClUint32T temp = sizeof(ClUint32T);

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);
        if (temp != sizeof(ClUint32T))
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            *(ClUint16T*)pPyld = 0;  /* At least it isn't random */
        }
        else
        {            
        *(ClUint16T*)pPyld = (ClUint16T)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)pPyld))
        }
        
        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallArrayClUint16T(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClUint32T i;
    ClUint16T* temp1 = (ClUint16T*)pPyld;
    ClUint32T temp = 0;
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    if (0 != msg)
    {
        for (i = 0; i < count ; i++)
        {
            temp = htonl(*temp1);
            ++temp1;
            CL_XDR_PRINT(("num: %x, %x\n", *(temp1 - 1), isDelete))
            rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClUint32T));
            if (rc != CL_OK)
            {
                CL_XDR_EXIT()
                return rc;
            }
        }

    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallArrayClUint16T(ClBufferHandleT msg,void* pPyld, ClUint32T count)
{
    ClUint32T i;
    ClUint16T *temp = (ClUint16T*)pPyld;
    ClUint32T temp2 = sizeof(ClUint32T);
    ClUint32T temp1;
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }

    for (i = 0; i < count ; i++)
    {
        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp2);
        if (rc != CL_OK)
        {
            CL_XDR_EXIT()
            return rc;
        }
        if (temp2 != sizeof(ClInt32T))
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            CL_XDR_EXIT()
            return rc;
        }        

        *temp++ = (ClUint16T)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(temp - 1)))
    }


    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

