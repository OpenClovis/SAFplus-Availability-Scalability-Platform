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
#ifndef _CL_LOGVWUTIL_H
#define _CL_LOGVWUTIL_H

#include <clCommon.h>
#include <clCommonErrors.h>
#include <errno.h>

#include "clLogVwType.h"

#ifdef __cplusplus
extern "C" {
#endif

    

#define CL_DISP_LOG_MSG(x,y) fprintf(stdout,x,y)


#define CL_LOGVW_NO_MEMORY_CHK(PTR)                 \
    do                                              \
    {                                               \
        if(NULL == PTR)                             \
        {                                           \
            fprintf(stdout,                         \
                    "Out of memory at [%s : %d]\n", \
                     __FILE__, __LINE__);           \
            return CL_ERR_NO_MEMORY;                \
        }                                           \
    }while(0)


#define CL_LOGVW_NO_MEM_CHK_WITH_STMT(PTR, STMT)    \
    do                                              \
    {                                               \
        if(NULL == PTR)                             \
        {                                           \
            fprintf(stdout,                         \
                    "Out of memory at [%s : %d]\n", \
                     __FILE__, __LINE__);           \
                                                    \
            STMT;                                   \
            return CL_ERR_NO_MEMORY;                \
        }                                           \
    }while(0)


#define CL_LOGVW_NULL_CHK(PTR)                      \
    do                                              \
    {                                               \
        if(NULL == PTR)                             \
        {                                           \
            fprintf(stdout,                         \
                    "Passed NULL ptr [%s : %d]\n",  \
                     __FILE__, __LINE__);           \
                                                    \
            return CL_ERR_NULL_POINTER;             \
        }                                           \
    }while(0)


#define CL_LOGVW_PRINT_ASCII_REC_ERROR()            \
    do                                              \
    {                                               \
        fprintf(stdout,                             \
                "\n[Probable cause : Log file might"\
                " have records of ASCII type "      \
                "(log viewer supports only Binary " \
                "records - Buffer and TLV types) ]");\
    }while(0)

#define CL_LOGVW_NANO_SEC 1000000000L

extern ClRcT clLogVwGetRevSubBytes(void *ptr, ClUint32T index, ClUint32T len);

extern ClBoolT clLogVwIsLittleEndian(void);

extern ClBoolT clLogVwIsSameEndianess( ClLogVwFlagT endianess);

#ifdef __cplusplus
}
#endif

#endif
