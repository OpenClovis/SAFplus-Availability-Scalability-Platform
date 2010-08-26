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
#ifndef _CL_LOGVWCONSTANTS_H
#define _CL_LOGVWCONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define CL_LOGVW_EXEC_NAME   "asp_binlogviewerexec"
#define CL_LOGVW_EXEC_APPEND "exec"

#define CL_LOGVW_LITTLE_ENDIAN    1
#define CL_LOGVW_BIG_ENDIAN       0

#define CL_LOGVW_MSGID_BINARY     0
#define CL_LOGVW_MSGID_ASCII      1


#define CL_LOGVW_MTD_CFG_MARKER_LEN           strlen(CL_LOG_DEFAULT_CFG_FILE_STRING)
#define CL_LOGVW_LOG_FILE_MARKER_LEN          strlen(CL_LOG_DEFAULT_FILE_STRING)
    

#define CL_LOGVW_MTD_CFG_MARKER_INDEX         256
#define CL_LOGVW_MTD_ENDIAN_INDEX             287 
#define CL_LOGVW_MTD_FILE_UNIT_SIZE_INDEX     288
#define CL_LOGVW_MTD_MAX_REC_SIZE_INDEX       292 
#define CL_LOGVW_MTD_MAX_FILES_ROTATED_INDEX  300 

#define CL_LOGVW_STREAMID_MAP_INDEX           320
#define CL_LOGVW_COMPID_MAP_INDEX             324
    
    

#define CL_LOGVW_HASH_TABLE_MAX_COUNT 3000

#ifdef __cplusplus
}
#endif

#endif
