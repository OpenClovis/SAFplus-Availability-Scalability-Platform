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
/*******************************************************************************
 * ModuleName  : static
 * File        : clSnmpLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_SNMP_LOG_H_
# define _CL_SNMP_LOG_H_

# ifdef __cplusplus
extern "C"
{
# endif


#define CL_SNMP_AREA "SNM" /* snmp area */
#define CL_SNMP_CTX_SOP "SOP" /* Static snmp operation context */
#define CL_SNMP_CTX_SNT "SNT" /* Static snmp notification context */
#define CL_SNMP_CTX_SUN "SFU" /* Static snmp function enter/exit context */

#define CL_SNMP_FUN_ENTER_LOG()\
    do\
    { \
        clLogTrace(CL_SNMP_AREA, CL_SNMP_CTX_SUN, "Entering into function : [%s]", __FUNCTION__); \
    }                                                                   \
    while(0)

#define CL_SNMP_FUN_EXIT_LOG()\
    do\
    { \
        clLogTrace(CL_SNMP_AREA, CL_SNMP_CTX_SUN, "Exiting into function : [%s]", __FUNCTION__); \
    }                                                                   \
    while(0)

    
extern ClCharT * clSnmpLogMsg[];

#define CL_SNMP_LOG_2_SOCK_FAILED  clSnmpLogMsg[0]
#define CL_SNMP_LOG_2_BIND_FAILED clSnmpLogMsg[1]
#define CL_SNMP_LOG_2_SOCK_RECV_FAILED clSnmpLogMsg[2]
#define CL_SNMP_LOG_1_ID_XLN_INSERT_FAILED clSnmpLogMsg[3]
#define CL_SNMP_LOG_1_OPCODE_INSERT_FAILED clSnmpLogMsg[4]
#define CL_SNMP_LOG_1_ERROR_ID_XLN_INSERT_FAILED clSnmpLogMsg[5]
#define CL_SNMP_LOG_1_CLASSTYPE_NAME_GET_FAILED clSnmpLogMsg[6]
#define CL_SNMP_LOG_1_MOID_APPEND_FAILED clSnmpLogMsg[7]

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_SNMP_LOG_H_ */
