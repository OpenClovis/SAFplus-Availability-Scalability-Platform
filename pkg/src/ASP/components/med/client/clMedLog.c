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
 * ModuleName  : med
 * File        : clMedLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

 #include<clCommon.h>
ClCharT	*clLogMedMsg[] = 
{
	"Failed to read pre-created cor object; Error = 0x%x", /* 0 */
	"No ID translation found; Error = 0x%x", /* 1 */
	"No Opcode  found; Error = 0x%x", /* 2 */	
	"No ErrorId  found; Error = 0x%x", /* 3 */		
	"No watch attribute  found; Error = 0x%x", /* 4 */			
	"Instance xln callback has failed; Error = 0x%x", /* 5 */	
	"Failed to add Instance xln ; Error = 0x%x", /* 6*/		
	"Failed to delete Instance xln ; Error = 0x%x", /* 7*/			
	"Index to instance xln is null", /* 7*/				
};

