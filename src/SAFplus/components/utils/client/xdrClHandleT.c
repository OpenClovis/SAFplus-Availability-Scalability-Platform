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
#include <stdio.h>

/**** ClHandleT ***/
/*********************************************************************************************/
ClRcT clXdrMarshallClHandleT(void* pPyld, ClBufferHandleT msg, ClUint32T isDelete)
{
    return clXdrMarshallClUint64T(pPyld, msg, isDelete);
}
/*********************************************************************************************/

/*********************************************************************************************/
ClRcT clXdrUnmarshallClHandleT( ClBufferHandleT msg , void* pPyld)
{
    return clXdrUnmarshallClUint64T(msg, pPyld);
}
/*********************************************************************************************/
