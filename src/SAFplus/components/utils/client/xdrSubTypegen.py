#!/usr/local/bin/python
# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.
################################################################################
# ModuleName  : utils
# File        : xdrSubTypegen.py
################################################################################
# Description :
################################################################################
import sys
import string
from string import Template

subTypeList = ['ClUint8T', 'ClInt8T', 'ClUint16T', 'ClInt16T']
baseTypeList = ['ClUint32T', 'ClInt32T']
superTypeList = ['ClInt64T', 'ClUint64T']
basetypeDict = dict([('ClUint8T', 'ClUint32T'), ('ClUint16T', 'ClUint32T'), ('ClInt8T', 'ClInt32T'), ('ClInt16T', 'ClInt32T'), ('ClUint64T', 'ClUint32T'), ('ClInt64T', 'ClInt32T')])

xdrSubTypeT = Template('''\
#include <string.h>
#include <netinet/in.h>
#include <clOsalApi.h>
#include <clXdrApi.h>
#include <clBufferApi.h>

/**** ${subType} ***/
/*********************************************************************************************/
ClRcT clXdrMarshall${subType}(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ${baseType} temp = HTONL(*(${subType}*)pPyld);
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    if (msg)
    {
        CL_XDR_PRINT(("num: %x\\n", *(${baseType}*)pPyld))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(${baseType}));
    }

    CL_XDR_ENTER()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshall${subType}(ClBufferHandleT msg,void* pPyld)
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
        ${baseType} temp1;
        ClUint32T temp = sizeof(${baseType});

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);

        *(${subType}*)pPyld = (${subType})NTOHL(temp1);
        CL_XDR_PRINT(("num: %x\\n", *(${baseType}*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallArray${subType}(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClUint32T i;
    ${baseType}* temp1 = (${baseType}*)pPyld;
    ${baseType} temp = 0;
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    if (0 != msg)
    {
        for (i = 0; i < (count * sizeof(${subType}) / sizeof(${baseType})); i++)
        {
            temp = HTONL(*temp1++);
            CL_XDR_PRINT(("num: %x, %x\\n", *(temp1 - 1), isDelete))
            rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(${baseType}));
            if (rc != CL_OK)
            {
                CL_XDR_EXIT()
                return rc;
            }
        }

        if (count % (sizeof(${baseType}) / sizeof(${subType})))
        {
            memcpy(&temp, temp1, (sizeof(${subType}) * (count % (sizeof(${baseType}) / sizeof(${subType})))));
            CL_XDR_PRINT(("num: %x, %x\\n", temp, isDelete))
            temp = HTONL(temp);
            rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(${baseType}));
        }
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallArray${subType}(ClBufferHandleT msg,void* pPyld, ClUint32T count)
{
    ClUint32T i;
    ${baseType} *temp = (${baseType}*)pPyld;
    ClUint32T temp2 = sizeof(${baseType});
    ${baseType} temp1;
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }

    for (i = 0; i < (count * sizeof(${subType}) / sizeof(${baseType})); i++)
    {
        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp2);
        if (rc != CL_OK)
        {
            CL_XDR_EXIT()
            return rc;
        }

        *temp++ = NTOHL(temp1);
        CL_XDR_PRINT(("num: %x\\n", *(temp - 1)))
    }

    if (count % (sizeof(${baseType}) / sizeof(${subType})))
    {
        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp2);
        if (rc != CL_OK)
        {
            CL_XDR_EXIT()
            return rc;
        }
        temp1 = NTOHL(temp1);
        CL_XDR_PRINT(("num: %x\\n", *(${baseType}*)arr))
        memcpy(temp, &temp1, (sizeof(${subType}) * (count % (sizeof(${baseType}) / sizeof(${subType})))));
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrMarshallPointer${subType}(void* pPyld, ClUint32T count, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        count = 0;
    }

    if (0 != msg)
    {
        ${baseType} temp = HTONL(count);
        CL_XDR_PRINT(("ptr num: %x, %x\\n", count, isDelete))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(${baseType}));
    }

    if ((CL_OK != rc) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return rc;
    }

    if (0 != msg)
    {
        rc = clXdrMarshallArray${subType}(pPyld, count, msg, isDelete);
    }

    if (isDelete)
    {
        CL_XDR_PRINT(("deleted\\n"))
        clHeapFree(pPyld);
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallPointer${subType}(ClBufferHandleT msg, void** pPyld)
{
    ClRcT rc = CL_OK;
    ${baseType} temp;
    ClUint32T temp1 = sizeof(${baseType});
    CL_XDR_ENTER()

    if ((0 == msg) || (NULL == pPyld))
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clBufferNBytesRead(msg, (ClUint8T*)&temp, &temp1);
    if ((CL_OK != rc) || (0 == temp))
    {
        if (0 == temp)
        {
            *pPyld = NULL;
        }
        CL_XDR_EXIT()
        return rc;
    }
    temp = NTOHL(temp);
    CL_XDR_PRINT(("ptr num: %x\\n", temp))

    *pPyld = clHeapAllocate(temp * sizeof(${baseType}));
    if (NULL == *pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NO_MEMORY);
    }

    rc = clXdrUnmarshallArray${subType}(*pPyld, temp, msg);

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/
''')


xdrTypeT = Template('''\
#include <string.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>

/**** ${type} ***/
/*********************************************************************************************/
ClRcT clXdrMarshall${type}(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ${type} temp = HTONL(*(${type}*)pPyld);
    CL_XDR_ENTER()

    if (NULL == pPyld)
    {
        CL_XDR_EXIT()
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    if (msg)
    {
        CL_XDR_PRINT(("num: %x, %x\\n", *(${type}*)pPyld, isDelete))
        rc = clBufferNBytesWrite(msg, (ClUint8T*)&temp, sizeof(${type}));
    }

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshall${type}( ClBufferHandleT msg , void* pPyld)
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
        ${type} temp1;
        ClUint32T temp = sizeof(${type});

        rc = clBufferNBytesRead(msg, (ClUint8T*)&temp1, &temp);

        *(${type}*)pPyld = (${type})NTOHL(temp1);
        CL_XDR_PRINT(("num: %x\\n", *(${type}*)pPyld))

        CL_XDR_EXIT()
        return rc;
    }

    CL_XDR_EXIT()
    return CL_XDR_RC(CL_ERR_INVALID_PARAMETER);
}
/*********************************************************************************************/
''')

xdrSuperTypeT = Template('''\
#include <string.h>
#include <netinet/in.h>
#include <clXdrApi.h>
#include <clBufferApi.h>

/**** ${superType} ***/
/*********************************************************************************************/
ClRcT clXdrMarshall${superType}(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ${baseType} lower = 0;
    ${baseType} upper = 0;
    CL_XDR_ENTER()

    lower = (${baseType})(*(${superType}*)pPyld & 0xffffffff);
    upper = (${baseType})((*(${superType}*)pPyld >> 32) & 0xffffffff);

    if (*(ClUint8T*)&endianTestVar == 1)
    {
        /*upper & lower if little endian swap them*/
        upper ^= lower;
        lower ^= upper;
        upper ^= lower;
    }

    rc = clXdrMarshall${baseType}(&lower, msg, isDelete);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrMarshall${baseType}(&upper, msg, isDelete);
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
ClRcT clXdrUnmarshall${superType}( ClBufferHandleT msg, void* pPyld)
{
    ClRcT rc = CL_OK;
    ClUint32T endianTestVar = 1;
    ${baseType} lower = 0;
    ${baseType} upper = 0;
    CL_XDR_ENTER()

    rc = clXdrUnmarshall${baseType}(&lower, msg);
    if (CL_OK != rc)
    {
        CL_XDR_EXIT()
        return rc;
    }

    rc = clXdrUnmarshall${baseType}(&upper, msg);
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
    }

    *(${superType}*)pPyld = ((${superType})upper << 32) | (${superType})lower;

    CL_XDR_EXIT()
    return rc;
}
/*********************************************************************************************/
''')

for type in subTypeList:
	typeF = open('xdr' + type + '.c', 'w')
	map = dict()
	map['subType'] = type
	map['baseType'] = basetypeDict[type]
	typeF.write(xdrSubTypeT.safe_substitute(map))
	typeF.close()

for type in baseTypeList:
	typeF = open('xdr' + type + '.c', 'w')
	map = dict()
	map['type'] = type
	typeF.write(xdrTypeT.safe_substitute(map))
	typeF.close()

for type in superTypeList:
	typeF = open('xdr' + type + '.c', 'w')
	map = dict()
	map['superType'] = type
	map['baseType'] = basetypeDict[type]
	typeF.write(xdrSuperTypeT.safe_substitute(map))
	typeF.close()
