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
 * ModuleName  : cor
 * File        : clCorDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains macros for working on object header
 *
 *
 *****************************************************************************/
#ifndef _COR_DEFS_H
#define _COR_DEFS_H
#include <clCommon.h>
#ifdef __cplusplus
extern "C" {
#endif
    
typedef ClInt8T*    clInt8_h;
typedef unsigned int corContKey_t;

/**
* COR Instance Header.
* Captures the Header information for COR Instance.
* MSB to LSB
*  32'nd Bit corresponds to
*     LOCK Bit  1  - locked & 0 - unlocked
*  31'st bit
*     Active bit  0 - Used & 1 - Unused
*  30'th bit
*     Contained bit  0 - Not contained & 1 - Contained within another obj
*  Bits (29-17)
*     Reserved
*  Bits (16-1)
*     Owner of the lock (id)
*
*/
typedef ClUint32T CORInstanceHdr_t;
typedef CORInstanceHdr_t* CORInstanceHdr_h;

#define MAX_COR_SERVICES                                64

#define COR_INSTANCE_HDR_INIT(hdr)      ((hdr)&=(0))

#define COR_INSTANCE_LOCK(hdr, owner)   ((hdr)|=(0x80000000)|((owner)&0xFFFF))
#define COR_INSTANCE_UNLOCK(hdr)        ((hdr)&=(0x7FFF0000))
                                                                                                                             
#define COR_INSTANCE_OWNER(hdr)         ((hdr)&(0xFFFF))
                                                                                                                             
#define COR_INSTANCE_ACTIVE(hdr)        ((hdr)|=(0x40000000))
#define COR_INSTANCE_UNUSED(hdr)        ((hdr)&=(0xBFFFFFFF))
                                                                                                                             
#define COR_INSTANCE_CONTAINED(hdr)     ((hdr)|=(0x20000000))
#define COR_INSTANCE_GLOBAL(hdr)        ((hdr)&=(0xDFFFFFFF))
                                                                                                                             
#define COR_INSTANCE_IS_LOCKED(hdr)     (((hdr)&(0x80000000))==0x80000000)
#define COR_INSTANCE_IS_ACTIVE(hdr)     (((hdr)&(0x40000000))==0x40000000)
#define COR_INSTANCE_IS_CONTAINED(hdr)  (((hdr)&(0x20000000))==0x20000000)


/** @pkg cl.cor */
#ifdef __cplusplus
}
#endif

#endif  /*  _INC_COR_NOTIFY_H_ */
