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
 * File        : xdrClUint8T.c
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

/**** ClUint8T ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClUint8T(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;

    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    ClUint32T temp = htonl(*(ClUint8T*)pPyld);

    if (msg)
    {
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)pPyld))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClUint32T));
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClUint8T(ClBufferHandleT msg,void* pPyld)
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
        ClUint32T temp1=0;
        ClUint32T temp = sizeof(ClUint32T);

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);
        if (rc != CL_OK)
        {
            clDbgCodeError(rc,("Failed to read the data"));
            *(ClUint8T*)pPyld = 0;  /* At least it isn't random */
            CL_XDR_EXIT()
            return rc;
        }

        if (temp != sizeof(ClUint32T))
        {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            *(ClUint8T*)pPyld = 0;  /* At least it isn't random */
            CL_XDR_EXIT()
            return rc;
        }        

        *(ClUint8T*)pPyld = (ClUint8T)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }
    else
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);


    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallArrayClUint8T(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
            return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    if( msg != 0)
    {
#if 0
        rc = clBufferNBytesWrite(msg, (ClUint8T*)pPyld, count);
#else
        /*
         * Try stiching the chunk to the buffer. In case of failure, dup.
         */
        if(clBufferAppendHeap(msg, (ClUint8T*)pPyld, count) != CL_OK)
        {
            rc = clBufferNBytesWrite(msg, (ClUint8T*)pPyld, count);
        }
#endif
    }
    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallArrayClUint8T(ClBufferHandleT msg,void* pPyld, ClUint32T count)
{
    ClRcT rc = CL_OK;
     if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }

    ClUint32T amtRead=count;
    if( msg != 0 )
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

