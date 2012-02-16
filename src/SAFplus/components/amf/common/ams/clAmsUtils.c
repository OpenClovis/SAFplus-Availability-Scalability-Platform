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
 * File        : clAmsUtils.c
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
#include <clAmsUtils.h> 
#include <stdarg.h> 
#include <clLogApi.h> 

char *clAmsFormatMsg (char *fmt, ...)
{
    ClCharT *buffer = NULL;
    ClInt32T len = 0;
    ClCharT c = 0;
    va_list     list;

    va_start(list,fmt);
    len = vsnprintf(&c, 1, fmt, list);
    va_end(list);

    buffer = clHeapAllocate(len+1);
    if(!buffer)
        return NULL;

    va_start(list, fmt);
    vsnprintf(buffer, len, fmt, list);
    va_end(list);

    return buffer;
}

void clAmsLogMsgClient( const ClUint32T level, char *buffer )
{
    ClLogSeverityT severity = (ClLogSeverityT)level;
    if(!buffer) return;
    clOsalPrintf (buffer);
    if(buffer[strlen(buffer)-1] != '\n') clOsalPrintf("\n");
    clLogWrite(CL_LOG_HANDLE_APP, severity, NULL, buffer);
    clAmsFreeMemory (buffer);
}


