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
 * File        : xdrClCharT.c
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

/**** ClCharT ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClCharT(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    ClUint32T temp = *(ClCharT*)pPyld;
    temp = htonl(temp); 
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
ClRcT clXdrUnmarshallClCharT(ClBufferHandleT msg,void* pPyld)
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
            *(ClCharT*)pPyld = 0;  /* At least it isn't random */
            CL_XDR_EXIT()
            return rc;
        }        
        temp1 = ntohl(temp1); 
        *(ClCharT*)pPyld = (ClCharT)temp1;
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallArrayClCharT(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClCharT *temp = NULL;	
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }
    temp = (ClCharT *)pPyld;
    if (0 != msg)
    {
        rc = clBufferNBytesWrite(msg, (ClUint8T*)temp,count);
        if (rc != CL_OK)
        {
             CL_XDR_EXIT()
             return rc;
        }
       /* for (i = 0; i < count ; i++)
        {
            temp1 =(ClUint32T)*temp++;
            CL_XDR_PRINT(("num: %x, %x\n", *(temp1 - 1), isDelete));
            temp1 = htonl(temp1);
            rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp1, sizeof(ClUint32T));
            if (rc != CL_OK)
            {
                CL_XDR_EXIT()
                return rc;
            }
        }
        */
    /*    if (count % (sizeof(ClUint32T) / sizeof(ClCharT)))
        {
            memcpy(&temp, temp1, (sizeof(ClCharT) * (count % (sizeof(ClUint32T) / sizeof(ClCharT)))));
            CL_XDR_PRINT(("num: %x, %x\n", temp, isDelete))
            rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(ClUint32T));
        }*/
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallArrayClCharT(ClBufferHandleT msg,void* pPyld, ClUint32T count)
{
    ClCharT *temp = (ClCharT *)pPyld;
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }
    ClUint32T amtRead=count;
    
    rc = clBufferNBytesRead(msg, (ClUint8T*)temp, &amtRead);
    if (amtRead != count)
    {
        rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
        clDbgCodeError(rc,("Buffer is shorter than data element"));
        *(ClCharT*)pPyld = 0;  /* At least it isn't random */
        return rc;
    }
    if (rc != CL_OK)
    {
            CL_XDR_EXIT()
            return rc;
    }
    
    /*    *temp++ = (ClCharT)ntohl(temp1);
        CL_XDR_PRINT(("num: %x\n", *(temp - 1)));*/

   /* if (count % (sizeof(ClUint32T) / sizeof(ClCharT)))
    {
        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp2);
        if (rc != CL_OK)
        {
            CL_XDR_EXIT()
            return rc;
        }
        CL_XDR_PRINT(("num: %x\n", *(ClUint32T*)arr))
        memcpy(temp, &temp1, (sizeof(ClCharT) * (count % (sizeof(ClUint32T) / sizeof(ClCharT)))));
    }*/

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

