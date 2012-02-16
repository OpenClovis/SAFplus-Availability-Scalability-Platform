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
 * ModuleName  : amf
 * File        : clAmsParser.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _CL_AMS_PARSER_H_
#define _CL_AMS_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clAmsEntities.h>
#include <clParserApi.h>

/*
 * Structure to hold name value pairs of a CSI
 */
typedef struct nvpList
{
    ClCharT *csiName;
    ClCharT *paramName;
    ClCharT *paramValue;
    struct  nvpList *next;

}ClAmsParserCSINVPListT;

/*
 * Structure to hold the configuration for entity definitions
 */

typedef struct List
{
    ClAmsEntityConfigT      *entityConfig;
    ClAmsParserCSINVPListT  *nvp; /* only valid for csi */
    struct List             *next;
}ClAmsParserConfigTypeT;

/*
 * Structure to hold the names of the entity instances created by the parser
 * This can be used in the end to print all the entities created by the
 * parser to validate that the database has been properly built
 */

typedef struct instanceList
{
    ClAmsEntityT         entity;
    struct instanceList *next;
}ClAmsParserInstanceNameT;

/*
 * This function add a entity definition type configuration to a list
 */

extern ClRcT   
clAmsParserAddEntityTypeConfig( 
        CL_INOUT ClAmsParserConfigTypeT **head,
        CL_IN ClAmsParserConfigTypeT newEntityType );

/*
 * This function finds a entity definition type configuration from the list
 */

extern ClRcT   
clAmsParserFindConfigType(
       CL_IN ClAmsParserConfigTypeT *head,
       CL_IN ClCharT *typeName,
       CL_IN ClAmsEntityTypeT entityType,
       CL_IN ClAmsParserConfigTypeT *entityConfigType );

/*
 * Function to parse the boolen values in the XML file 
 */

extern ClRcT 
clAmsParserBooleanParser(
        CL_OUT ClBoolT *data_ptr,
        CL_IN ClParserPtrT ptr,
        CL_IN ClCharT *str );

/*
 * Function to parse the Uint32 values in the XML file 
 */

extern ClRcT 
clAmsParserUint32Parser(
        CL_OUT ClUint32T *data_ptr, 
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str );

/*
 * Function to parse the Int64 values in the XML file 
 */

extern ClRcT 
clAmsParserTimeoutParser ( 
        CL_OUT ClInt64T *data_ptr, 
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str,
        CL_IN ClCharT *entityTypeName );


/*
 * Function to parse the classtype values for node entities 
 */

extern ClRcT clAmsParserNodeClassTypeParser ( 
        CL_OUT ClAmsNodeConfigT *nodeConfig,
        CL_IN ClParserPtrT ptr );

/*
 * Function to parse the loading strategy  for a service group 
 */

extern ClRcT 
clAmsParserSGLoadingStrategyParser( 
        CL_OUT ClAmsSGConfigT *sgConfig,
        CL_IN ClParserPtrT ptr );

/*
 * Function to parse the redundancy model for a service group 
 */

extern ClRcT 
clAmsParserSGRedundancyModelParser ( 
        CL_OUT ClAmsSGConfigT *sgConfig,
        CL_IN ClParserPtrT ptr);

/*
 * Function to parse the component property
 */

extern ClRcT 
clAmsParserCompPropertyParser(
        CL_OUT ClUint32T *property,
        CL_IN ClParserPtrT ptr);

/*
 * Function to parse the component timeouts
 */

extern ClRcT 
clAmsParserCompTimeoutsParser( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr);

/*
 * Function to parse the recovery on timeout value for a component 
 */

extern ClRcT 
clAmsParserRecoveryOnTmOutParser( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr);

/*
 * Function to parse the capability model for a component 
 */

extern ClRcT 
clAmsParserCompCapabilityModelParser ( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr);

/*
 * Function to parse the admin state of an entity 
 */

extern ClRcT 
clAmsParserEntityAdminStateParser( 
        CL_OUT ClAmsAdminStateT *adminState,
        CL_IN ClParserPtrT ptr );

/*
 * Function to parse a string tag in the XML 
 */

extern ClRcT 
clAmsParserStringParser(
        ClCharT **data_ptr, 
        ClParserPtrT ptr, 
        ClCharT *str );


/*
 * Function to parse the ClName type tag in the XML 
 */

extern ClRcT 
clAmsParserClNameParser (
        CL_OUT ClNameT *clName, 
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str );

/*
 * Function to parse the entity name attribute in the XML file 
 */

extern ClRcT clAmsParserEntityAttrParser(
        CL_OUT ClAmsEntityConfigT *entityConfig,
        CL_IN ClAmsEntityTypeT entityType,
        CL_IN ClParserPtrT ptr,
        CL_IN ClCharT *str);

/*
 * Function to parse the CSI definition type values 
 */

extern ClRcT 
clAmsParserCSIDefParser(
        CL_IN ClParserPtrT csi );

/*
 * Function to parse the Component definition type values 
 */

extern ClRcT 
clAmsParserCompDefParser(
        CL_IN ClParserPtrT comp );

/*
 * Function to parse the SI definition type values 
 */

extern ClRcT 
clAmsParserSIDefParser(
        CL_IN ClParserPtrT si );

/*
 * Function to parse the SU definition type values 
 */

extern ClRcT 
clAmsParserSUDefParser( 
        CL_IN ClParserPtrT su );

/*
 * Function to parse the SG definition type values 
 */

extern ClRcT 
clAmsParserSGDefParser(
       CL_IN ClParserPtrT sg );

/*
 * Function to parse the Application definition type values 
 */

extern ClRcT 
clAmsParserAppDefParser(
        CL_IN ClParserPtrT app);

/*
 * Function to parse the Node definition type values 
 */

extern ClRcT 
clAmsParserNodeDefParser(
        CL_IN ClParserPtrT node );

/*
 * Function to parse the entity type definition 
 */

extern ClRcT 
clAmsParserEntityTypeParser(
        CL_IN ClCharT *listName,
        CL_IN ClCharT *entityName,
        CL_IN ClAmsEntityTypeT entityType,
        CL_IN ClParserPtrT fileParserPtr );

/*
 * entry point into XML parser 
 */

extern ClRcT 
clAmsParserMain(
       CL_IN ClCharT *amfDefinitionFileName,
       CL_IN ClCharT *amfConfigFileName );

/*
 * Function to create the node containment hierarchy 
 */

extern ClRcT 
clAmsParserNodeCreation(
        CL_IN ClParserPtrT fileParserPtr );

/*
 * Function to create the su containment hierarchy 
 */

extern ClRcT 
clAmsParserSUCreation( 
        CL_IN ClParserPtrT nodePtr ,
        CL_IN ClAmsNodeConfigT *nodeConfig );


/*
 * Function to create the component containment hierarchy 
 */

extern ClRcT 
clAmsParserCompCreation( 
        CL_IN ClParserPtrT suPtr,
        CL_IN ClAmsSUConfigT *suConfig,
        ClCharT *parentNode );

/*
 * Function to create the sg containment hierarchy 
 */

extern ClRcT 
clAmsParserSGCreation(
        CL_IN ClParserPtrT fileParserPtr );

/*
 * Function to create the si containment hierarchy 
 */

extern ClRcT 
clAmsParserSICreation( 
        CL_IN ClParserPtrT sgPtr ,
        CL_IN ClAmsSGConfigT *sgConfig );


/*
 * Function to create the csi containment hierarchy 
 */

extern ClRcT clAmsParserCSICreation( 
        CL_IN ClParserPtrT siPtr,
        CL_IN ClAmsSIConfigT *siConfig );

/*
 * Function to create the relationships for entities 
 */

extern ClRcT 
clAmsParserCreateRelationship( 
        CL_IN ClParserPtrT fileParserPtr,
        CL_IN ClCharT *instancesName,
        CL_IN ClCharT *instanceName,
        CL_IN ClCharT *listName,
        CL_IN ClCharT *entityName,
        CL_IN ClAmsEntityListTypeT entityListName,
        CL_IN ClAmsEntityTypeT sourceEntityType ,
        CL_IN ClAmsEntityTypeT targetEntityType );


/*
 * Set the name attribute for an entity by parsing the name attribute 
 */

extern ClRcT   
clAmsParserSetEntityName(
        CL_IN ClAmsEntityConfigT *entityConfig,
        CL_IN ClCharT *name );

/*
 * Function to set the entity's configuration based on the definition 
 * type configuration in the XML file. 
 */

extern ClRcT   
clAmsParserSetEntityConfig (
        CL_IN ClParserPtrT ptr,
        CL_IN ClAmsEntityTypeT entityType,
        CL_OUT ClAmsEntityConfigT *entityConfig,
        CL_IN ClCharT *parentNode);

/*
 * Function which creates an entity in the database by calling the 
 * mgmt API  create function
 */

extern ClRcT    
clAmsParserEntityCreate (
        CL_IN ClAmsEntityConfigT *entityConfig );


/*
 * Function to create the csi nvp list 
 */

extern ClRcT 
clAmsParserCreateCSINVPList (
        CL_IN ClCharT *csiName,
        CL_IN ClCharT *csiType );

/*
 * This function is called to delete all the memory used for types
 */

extern ClRcT   
clAmsParserFreeConfigType(
       CL_IN ClAmsParserConfigTypeT *head );


/*
 * This function is called to parse the ams specific configuration. Currently
 * this parses the AMS logging related value.
 */

extern ClRcT
clAmsParserAmsConfigParser( 
        CL_IN  ClParserPtrT  fileParserPtr );


#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_PARSER_H_ */
