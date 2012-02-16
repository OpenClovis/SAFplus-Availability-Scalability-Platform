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
 * File        : clCorLog.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains LOG related definitions for COR.   
 *****************************************************************************/

/* INCLUDES */
#include "clDebugApi.h"
#include "clCorLog.h"

ClCharT *gCorClientLibName = "cor_client";

/* Definition of the COR related log messages */

ClCharT *clCorLogMsg[] = {
	"Failed to initialize %s. rc [0x%x]", /*0*/
	"Failed to register COR service with transaction agent. rc [0x%x]",/*1*/
	"Failed to process all the transaction jobs. rc [0x%x]",/*2*/
	"Failed to get component list from route manager or components interested. rc [0x%x]",/*3*/
	"Failed to set component addresss for this job. rc [0x%x]",/*4*/
	"Failed to create COR class. rc [0x%x]",/*5*/
	"Failed to delete COR class. rc [0x%x]",/*6*/
	"Failed to create COR class attribute. rc [0x%x]",/*7*/
	"Failed to delete COR class attribute. rc [0x%x]",/*8*/
	"Failed to set COR class attribute value. rc [0x%x]",/*9*/
	"Failed to set COR class attribute flag. rc [0x%x]",/*10*/
	"Failed to get COR class attribute. rc [0x%x]",/*11*/
	"Data synchronization with master COR failed.   rc [0x%x]",/*12*/
	"Data synchronization with master COR completed succesfully.", /*13*/
	"Data restoration from persistent database failed.  rc [0x%x]",/*14*/
	"Loaded the default information model to COR",/*15*/
	"Default information model could not be loaded to COR. rc [0x%x]",/*16*/
	"Data restoration from persistent database done successfully. ",/*17*/
	"%s lib initialization failed.  rc [0x%x]",/*18*/
	"COR could not get data from any source. No information model present. rc [0x%x]",/*19*/
	"Failed to get EO object.  rc [0x%x]",/*20*/
	"COR failed to get mapping information from XML.  rc [0x%x]",/*21*/
	"COR server fully up",/*22*/
	"COR EO interface is already initialized.",/*23*/
	"Native function table installation failed. rc [0x%x]",/*24*/
	"Failed to create MO class. rc [0x%x]",/*25*/
	"Failed to create MSO class. rc [0x%x]",/*26*/
	"Failed to delete MO class. rc [0x%x]",/*27*/
	"Failed to delete MSO class. rc [0x%x]",/*28*/
	"Event publish channel open failed for COR. rc [0x%x]",/*29*/
	"Event message allocation failed for COR. rc [0x%x]",/*30*/
	"Event subscribe channel open failed for CPM (component termination). rc [0x%x]",/*31*/
	"CPM event (component termination) subscription Failed. rc [0x%x]",/*32*/
	"Event subscribe channel open failed for CPM (node arrival). rc [0x%x]",/*33*/
	"CPM event (node arrival) subscription Failed. rc [0x%x]",/*34*/
	"Event publish failed for COR. rc [0x%x]",/*35*/
	"Failed to get COR object attribute. rc [0x%x]",/*36*/
	"Failed to set COR object attribute. rc [0x%x]",/*37*/
	"Failed to create object tree. rc [0x%x]",/*38*/
	"Hello request to COR master returns error. rc [0x%x]",/*39*/
	"%sUnpack - Failed to unpack. rc [0x%x]",/*40*/
	"%s - Sync request failed. rc [0x%x]",/*41*/
	"Failed to add COR instance to corList. rc [0x%x]",/*42*/
	"Failed to pack the %s tree. rc [0x%x]",/*43*/
	"Failed to create COR object. rc [0x%x]. %s",/*44*/
	"Failed to delete COR object. rc [0x%x]. %s",/*45*/
    "Failed to create Delta Db file name. rc [0x%x]",/*46*/
    "Successfully restored the object information from Db. ",/*47*/
    "COR initialization succesfully completed ",  /*48*/
    "COR finalization succesfully completed ",  /*49*/
    "IM creation failure. Configuration attribute [0x%x:0x%x] has incorrect qualifiers[%s].", /* 50 */
    "IM creation failure. Runtime attribute [0x%x:0x%x] has incorrect qualifiers [%s].", /* 51 */
    "IM creation failure. Operational attribute [0x%x:0x%x] has incorrect qualifiers [%s].", /* 52 */
    "IM creation failure. Attribute [0x%x:0x%x] flag is invalid [%s]. ", /* 53 */
    "Failed while adding the OI [0x%x:0x%x] in the OI-List. ", /* 54 */
    "MO [%s] donot have any OI configured .", /* 55 */
    "Preprocessing failed for the get operation [%s].", /* 56 */
    "Attribute Id [0x%x] does not exist for ClassId [0x%x] " /* 57 */
	};
