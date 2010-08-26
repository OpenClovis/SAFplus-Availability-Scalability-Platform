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
/*********************************************************************
 * ModuleName  : idl
 * File        : xdrClGenericPtr.c
 ********************************************************************/

/*********************************************************************
 * Description :
 *     This file contains IDL APIs implementation
 ********************************************************************/

#include <string.h>
#include <netinet/in.h>
#include <clOsalApi.h>
#include <clXdrApi.h>
#include <clIdlErrors.h>
#include <clBufferApi.h>

/********************************************************************/
ClRcT
clXdrMarshallPtrClCharT( void* pPyld, ClUint32T count,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pPyld)
    {
        count = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArrayClCharT(pPyld, count, msg, isDelete);
    }

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtrClCharT( ClBufferHandleT msg, void** pPyld,
        ClUint32T multiplicity)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pPyld = NULL;
        CL_XDR_EXIT();
        return rc;
    }
    *pPyld = clHeapAllocate(multiplicity * sizeof(ClCharT));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArrayClCharT(msg,*pPyld, multiplicity);
    if( rc != CL_OK)
    {
        clHeapFree(*pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrMarshallPtrClInt16T( void* pPyld, ClUint32T count,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pPyld)
    {
        count = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArrayClInt16T(pPyld, count, msg, isDelete);
    }

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtrClInt16T( ClBufferHandleT msg, void** pPyld,
        ClUint32T multiplicity)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pPyld = NULL;
        CL_XDR_EXIT();
        return rc;
    }

    *pPyld = clHeapAllocate(multiplicity * sizeof(ClInt16T));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArrayClInt16T(msg,*pPyld, multiplicity);
    if( rc != CL_OK)
    {
        clHeapFree(*pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrMarshallPtrClUint16T( void* pPyld, ClUint32T count,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pPyld)
    {
        count = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArrayClUint16T(pPyld, count, msg, isDelete);
    }

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtrClUint16T( ClBufferHandleT msg,
        void** pPyld , ClUint32T multiplicity)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pPyld = NULL;
        CL_XDR_EXIT();
        return rc;
    }

    *pPyld = clHeapAllocate(multiplicity * sizeof(ClUint16T));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArrayClUint16T(msg,*pPyld, multiplicity);
    if( rc != CL_OK)
    {
        clHeapFree(*pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrMarshallPtrClInt8T( void* pPyld, ClUint32T count,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pPyld)
    {
        count = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArrayClInt8T(pPyld, count, msg, isDelete);
    }

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtrClInt8T( ClBufferHandleT msg, void** pPyld,
        ClUint32T multiplicity)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pPyld = NULL;
        CL_XDR_EXIT();
        return rc;
    }

    *pPyld = clHeapAllocate(multiplicity * sizeof(ClInt8T));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArrayClInt8T(msg,*pPyld, multiplicity);
    if( rc != CL_OK)
    {
        clHeapFree(*pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrMarshallPtrClUint8T( void* pPyld, ClUint32T count,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pPyld)
    {
        count = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArrayClUint8T(pPyld, count, msg, isDelete);
    }
    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtrClUint8T( ClBufferHandleT msg, void** pPyld,
        ClUint32T multiplicity)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pPyld = NULL;
        CL_XDR_EXIT();
        return rc;
    }

    *pPyld = clHeapAllocate(multiplicity * sizeof(ClUint8T));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArrayClUint8T(msg,*pPyld, multiplicity);
    if( rc != CL_OK)
    {
        clHeapFree(*pPyld);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrMarshallPtr( void* pointer, ClUint32T typeSize,
        ClUint32T multiplicity, ClXdrMarshallFuncT func,
        ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if (NULL == pointer)
    {
        multiplicity = 0;
        CL_XDR_EXIT();
        return rc ;
    }

    if( 0 != msg)
    {
        rc = clXdrMarshallArray(pointer, typeSize, multiplicity, func,
                msg, isDelete);
    }        

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\n"));
        /*
         * This function is the wrapper over the free function
         * which depends on the allocation mechansim. This will
         * internally call either clHeapFree or userProvided free
         * function.
         */
        clIdlFree(pointer);
    }

    CL_XDR_EXIT();
    return rc;
}

ClRcT
clXdrUnmarshallPtr( ClBufferHandleT msg,void** pointer,
        ClUint32T typeSize, ClUint32T multiplicity,
        ClXdrUnmarshallFuncT func)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER();

    if ((0 == msg) || (NULL == pointer))
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_INVALID_PARAMETER);
    }
    if(multiplicity == 0)
    {
        *pointer = NULL;        
        CL_XDR_EXIT();
        return rc;
    }

    *pointer = clHeapAllocate(typeSize * multiplicity);
    if (NULL == *pointer)
    {
        CL_XDR_EXIT();
        return CL_IDL_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArray(msg,*pointer, typeSize, multiplicity,
            func);
    if (CL_OK != rc)
    {
        clHeapFree(*pointer);
    }

    CL_XDR_EXIT();
    return rc;
}
