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
#include <string.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>
#include <clDebugApi.h>
#include <stdio.h>

/**** ClWordT ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClWordT(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClUint32T wordSize = sizeof(ClWordT);
    ClUint32T size = 0;
    
    CL_XDR_ENTER();

    if(!msg)
    {
        CL_XDR_EXIT();
        return rc;
    }

    if(pPyld == NULL)
    {
        CL_XDR_EXIT();
        return rc;
    }

    size = htonl(wordSize);

    if( ( rc = clBufferNBytesWrite(msg,(ClUint8T*)&size,sizeof(size) ) ) != CL_OK ) 
    {
       CL_XDR_EXIT();
       return rc;
    }
    if (wordSize == 4)
    {
        rc = clXdrMarshallClUint32T(pPyld, msg, isDelete);
    }
    else if (wordSize == 8)
    {
        rc = clXdrMarshallClUint64T(pPyld, msg, isDelete);
    }

    CL_XDR_EXIT();
    return rc;
}

/*********************************************************************************************/
ClRcT clXdrUnmarshallClWordT( ClBufferHandleT msg , void* pPyld)
{
    ClRcT rc = CL_OK;
    ClUint32T size = 0;
    ClUint32T tempSize = sizeof(size);
    CL_XDR_ENTER()

    if(!msg)
    {
        CL_XDR_EXIT();
        return rc;
    }

    if(pPyld == NULL)
    {
        CL_XDR_EXIT();
        return rc;
    }
    ClUint32T amtRead = tempSize;
    if((rc = clBufferNBytesRead(msg,(ClUint8T*)&size,&amtRead)) != CL_OK)
    {
       CL_XDR_EXIT();
       return rc;
    }
    if (amtRead != tempSize)
    {
            rc = CL_XDR_RC(CL_ERR_INVALID_BUFFER);
            clDbgCodeError(rc,("Buffer is shorter than data element"));
            CL_XDR_EXIT()
            return rc;           
    }
        
    size = ntohl(size);
    if (size == 4) 
    {
        ClUint32T temp = 0;
        rc = clXdrUnmarshallClUint32T(msg,(void*)&temp);
        if(rc == CL_OK)
        {
            *(ClWordT*)pPyld = (ClWordT)temp;
        }
    }
    else if (size == 8)
    {
        ClInt64T temp = 0;
        rc = clXdrUnmarshallClUint64T(msg,(void*)&temp);
        if(rc == CL_OK) 
        {
            *(ClWordT*)pPyld = (ClWordT)temp;
        }
    }
    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/
