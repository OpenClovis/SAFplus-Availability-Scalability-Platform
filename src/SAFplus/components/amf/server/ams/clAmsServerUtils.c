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
 * ModuleName  : amf
 * File        : clAmsServerUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains utility functions required by AMS.
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
#include <clAmsServerUtils.h> 
#include <clAms.h> 
#include <stdarg.h> 
#include <clLogApi.h> 

ClAmsEntityRefT* clAmsAllocEntityRef()
{
    ClAmsEntityRefT* ret = (ClAmsEntityRefT*) clHeapAllocate(sizeof(ClAmsEntityRefT));
    CL_ASSERT(ret);  /* I can't gracefully handle an out of memory condition */
    ret->ptr = NULL;
    ret->nodeHandle = NULL;
    ret->entity.type = CL_AMS_ENTITY_TYPE_ENTITY;  /* Invalid */
    ret->entity.name.length = 0;
    ret->entity.name.value[0] = 0;
    return ret;
}

ClAmsEntityRefT* clAmsCreateEntityRef(ClAmsEntityT* ent)
{
    ClAmsEntityRefT* ret = clAmsAllocEntityRef();
    ret->ptr = ent;
    memcpy(&ret->entity, ent, sizeof(ClAmsEntityT) );
    return ret;    
}

void clAmsFreeEntityRef(ClAmsEntityRefT* ref)
{
    CL_ASSERT(ref);  /* Don't allow freeing a NULL pointer */
    ref->ptr = NULL;
    ref->nodeHandle = NULL;
    ref->entity.type = CL_AMS_ENTITY_TYPE_ENTITY;  /* Invalid */
    ref->entity.name.length = 0;
    ref->entity.name.value[0] = 0;
    
    clHeapFree(ref);
}


void clAmsLogMsgServer( const ClUint32T level, char *buffer, const ClCharT* file, ClUint32T line )
{
    ClLogSeverityT severity = (ClLogSeverityT)level;

    if(!buffer) return;

    if ( gAms.debugLogToConsole == CL_TRUE )
    { 
        clOsalPrintf (buffer);
        if(buffer[strlen(buffer)-1] != '\n') clOsalPrintf("\n");
    }

    clLogMsgWrite(CL_LOG_HANDLE_SYS, severity, CL_CID_AMS, "AMS","SVR",file,line, buffer);

    clAmsFreeMemory (buffer);
}

