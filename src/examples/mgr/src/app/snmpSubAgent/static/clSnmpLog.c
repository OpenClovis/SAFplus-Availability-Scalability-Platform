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
 * ModuleName  : static
 * File        : clSnmpLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

 #include<clCommon.h>
const ClCharT	*clSnmpLogMsg[] = 
{
	"%s Socket creation has failed, Error is %d", /* 0 */
	"%s Socket bind has failed, Error is %d", /* 1 */
	"%s Receive has failed, Error is %d", /* 2 */	
	"Failed to add entry into id translation table, Error is 0x %x", /* 3 */		
            "Failed to add entry in opcode table, Error is 0x%x", /* 4 */		                        
            "Failed to add entry in errorid table, Error is 0x%x", /* 5*/		                                    
            "Failed to get name from cor class type, Error is 0x%x", /* 6 */		                                    
            "Failed to append MOId, Error is 0x%x", /* 7 */		                                                
            
	
};

 
