/*
 * 
 *   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
 * 
 *   The source code for this program is not published or otherwise divested
 *   of its trade secrets, irrespective of what has been deposited with  the
 *   U.S. Copyright office.
 * 
 *   No part of the source code  for this  program may  be use,  reproduced,
 *   modified, transmitted, transcribed, stored  in a retrieval  system,  or
 *   translated, in any form or by  any  means,  without  the prior  written
 *   permission of OpenClovis Inc
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : static
 * File        : clSnmpLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

 #include<clCommon.h>
ClCharT	*clSnmpLogMsg[] = 
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

 
