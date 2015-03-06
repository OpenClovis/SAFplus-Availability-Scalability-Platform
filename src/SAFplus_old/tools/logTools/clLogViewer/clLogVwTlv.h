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
#ifndef _CL_LOGVWTLV_H
#define _CL_LOGVWTLV_H

#ifdef __cplusplus
extern "C" {
#endif

#define CL_LOGVW_TLV_HASH_TB_SIZE 1024
/*
 *Constants for the TLV structure.
 */

#define CL_LOGVW_TLV_LENGTH_INDEX     2

#define CL_LOGVW_TLV_VALUE_INDEX      4


/*
 *Constants for the Tag of TLV used by Logger.
 */
#define CL_LOGVW_TAG_END              0

#define CL_LOGVW_TAG_BASIC_SIGNED     1

#define CL_LOGVW_TAG_BASIC_UNSIGNED   2

#define CL_LOGVW_TAG_STRING           3



#define CL_LOGVW_TLV_MAX_COUNT        500

#define CL_LOGVW_TLV_MAX_MSG_ID_LEN   15

#define CL_LOGVW_TLV_MAX_STRING_LEN   1024

#define CL_LOGVW_TLV_DELM             "%TLV"

    
#ifdef __cplusplus
}
#endif

#endif
