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
#ifndef _CL_MEM_TRACKER_H_
#define _CL_MEM_TRACKER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CL_OUTPUT(...) do { \
    char buf[0xff+1];\
    snprintf(buf,sizeof(buf),__VA_ARGS__);\
    clOsalPrintf(buf);\
}while(0)

#define CL_MEM_TRACKER_ADD(id,address,size,private,logFlag) \
    clMemTrackerAdd(id,address,size,private,logFlag)

#define CL_MEM_TRACKER_DELETE(id,address,size,logFlag) \
    clMemTrackerDelete(id,address,size,logFlag)

#define CL_MEM_TRACKER_TRACE(id,address,size) \
    clMemTrackerTrace(id,address,size)

extern void clMemTrackerAdd(const char *id,
                            void *pAddress,
                            ClUint32T size,
                            void *pPrivate,
                            ClBoolT logFlag
                            );

extern void clMemTrackerDelete(const char *id,
                               void *pAddress,
                               ClUint32T size,
                               ClBoolT logFlag
                               );

extern void clMemTrackerTrace   ( const char *id,
                                  void *pAddress,
                                  ClUint32T size
                                );

extern ClRcT clMemTrackerGet(void *pAddress,ClUint32T *pSize,void **ppPrivate);

extern void clMemTrackerDisplayLeaks(void);

extern ClRcT clMemTrackerInitialize(void);

#ifdef __cplusplus
}
#endif

#endif
