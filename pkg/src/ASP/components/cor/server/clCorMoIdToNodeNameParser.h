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
 * ModuleName  : cor
 * File        : clCorMoIdToNodeNameParser.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Parser for MOId to node Name map table.
 *
 *****************************************************************************/

#ifndef _CL_COR_MOID_TO_NODE_NAME_PARSER_H_
#define _CL_COR_MOID_TO_NODE_NAME_PARSER_H_
#ifdef __cplusplus
extern "C"
{
#endif
#include "clParserApi.h"

#define ASP_CONFIG_FILE_NAME "clAmfConfig.xml"

/* The structure for MOID to node Name table entry */
typedef struct moIdToNodeNameParseTableEntry
{
    char* NodeName;
    char* MoId;
}moIdToNodeNameParseTableEntryT;


/************************************************************/
/* Top level API which will be called by COR Init function */
/************************************************************/
ClRcT clCorMoIdToNodeNameTableFromConfigCreate(void);

ClRcT clCorMoIdToNodeNameTableRead(ClParserPtrT fileName, moIdToNodeNameParseTableEntryT entries[], ClUint32T * numOfEntries);
#endif  /* _CL_COR_MOID_TO_NODE_NAME_PARSER_H_ */

