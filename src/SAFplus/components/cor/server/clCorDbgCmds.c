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
 * File        : clCorDbgCmds.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *  Implements the Debug-Cli commands of COR.
 *****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <clCommon.h>
#include <clCorApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clOsalApi.h>
#include <ctype.h>
#include "clCorNiLocal.h"
#include "clDebugApi.h"
#include "clCorClient.h"

/** Internal functions. **/
#include <clCorTreeDefs.h>
#include <clCorPvt.h>
#include <clCorObj.h>



ClCharT gCorCliStr[CL_COR_CLI_STR_LEN];
extern ClCorClassTypeT gClCorClassIdAlloc;

ClRcT clCorNIClassAdd(char *name, ClCorClassTypeT key);
ClRcT clCorNIClassDelete(ClCorClassTypeT classId);
ClRcT clCorNIClassIdGet(char *name, ClCorClassTypeT *key);
ClRcT clCorNIClassNameGet(ClCorClassTypeT key, char *name );
ClRcT clCorNIAttrAdd(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name);
ClRcT clCorNIAttrDel(ClCorClassTypeT classId, ClCorAttrIdT  attrId);
ClRcT clCorNIAttrNameGet(ClCorClassTypeT classId, ClCorAttrIdT  attrId, char *name );

/* S T A T I C    D E F I N I T I O N S  */

/* E X T E R N    D E F I N I T I O N S  */
extern ClRcT clCorMOClassCreateByName(char *path, ClInt32T maxInstances);
extern ClRcT clCorMSOClassCreateByName(char *path, int srvcId, int classType);
extern ClRcT clCorMOClassDeleteNodeByName(char *path);
extern ClRcT clCorMSOClassDeleteByName(char *path, int serviceId);
extern ClRcT corObjCreateByName(char *path, int svcId);
extern ClRcT corObjDeleteByName(char *path, int svcId);
extern ClRcT corObjAttrSetByName(char *path, int srvcId, char *attrP , ClInt32T attrId, ClInt32T index, ClInt64T value,
                                ClBufferHandleT *pMsgHdl);
extern ClRcT corObjAttrGetByName(char *path, int srvcId, char *attrP, ClInt32T attrId, ClInt32T Index, ClInt64T *value,
                                ClBufferHandleT *pMsgHdl);
extern ClRcT _clCorBundleJobEnqueue(ClCorBundleHandleT bundleHandle, char *path, int srvcId, char *attrP, ClInt32T attrId, ClInt32T index, 
                       ClBufferHandleT *pMsgHdl, ClCorDbgGetBundleT *pDbgBundle, ClCorOpsT opType);
extern ClRcT corObjTreeWalkByName(char *path,ClBufferHandleT * pMsg);
extern void dmShow(char* params, ClBufferHandleT *pMsgHdl);
extern void corNiTableShow(ClBufferHandleT *pMsgHdl);
extern ClRcT corObjShowByName(ClCorMOIdPtrT pMoId, ClCorServiceIdT svcId, char *cAttrPath, ClBufferHandleT *pMsgHdl);
extern void rmShow(char** params, ClBufferHandleT *pMsgHdl);
extern ClRcT clCorUtilMoAndMSOCreate( ClCorMOIdPtrT pMoId, ClCorObjectHandleT *pHandle);
extern ClRcT clCorUtilMoAndMSODelete( ClCorMOIdPtrT pMoId);
extern void _clCorDisplayInfoAppend(ClCorDbgBundleDataPtrT pBundleData, ClBufferHandleT bufH);
extern ClInt32T clCorTestBasicSizeGet(ClUint32T type);
extern ClRcT clCorDbgBundleJobCntCreate ( CL_OUT ClCntHandleT *cntHandle);
extern ClRcT corSubTreeDeleteByName(ClCharT* moPath);
extern ClRcT _clCorClassShow (ClBufferHandleT bufH);
extern ClRcT corDbgOIRegister(ClCharT* path, ClCharT);


extern ClCntHandleT corDbgBundleCnt;

ClInt64T corAtoI(char *str);
/* Attribute Conversion Table */
struct cliAttrTypeMap{
  char*     type;
  ClCorAttrIdT id;
};

typedef struct cliAttrTypeMap cliAttrTypeMap_t;
typedef struct cliAttrTypeMap* cliAttrTypeMap_h;
static cliAttrTypeMap_t cliAttrTypes[]=
{
   {"INT8",   CL_COR_INT8},                                        /**< char datatype */
   {"UINT8",  CL_COR_UINT8},                                        /**< unsigned char  */
   {"INT16",  CL_COR_INT16},                                       /**< short datatype */
   {"UINT16", CL_COR_UINT16},                                      /**< unsigned short */
   {"INT32",  CL_COR_INT32},                                       /**< integer datatype */
   {"UINT32", CL_COR_UINT32},                                      /**< unsigned int datatype */
   {"INT64",  CL_COR_INT64},                                       /**< long long integer datatype */
   {"UINT64", CL_COR_UINT64},                                      /**< unsigned long long int datatype */
   {NULL,   0}
};

/* Operation Conversion Table */
struct cliOpTypeMap{
  char*     type;
  ClCorOpsT  id;
};

typedef struct cliOpTypeMap cliOpTypeMap_t;
typedef struct cliOpTypeMap* cliOpTypeMap_h;

ClCharT* clCorServiceIdString[] = {
    "DUMMY",
    "FAULT",
    "ALARM",
    "PROV",
    "PM",
    "AMF",
    "CHM",
    "INVALID"
};

ClUint32T gClCorServiceIdStringCount = sizeof(clCorServiceIdString)/sizeof(clCorServiceIdString[0]);

static ClInt32T clCorServiceIdStringToValueGet(ClCharT* str)
{
    ClInt32T i = 0;

    for (i=0; i < gClCorServiceIdStringCount; i++)
    {
        if (! strcasecmp(clCorServiceIdString[i], str))
            return i;
    }

    return CL_COR_INVALID_SRVC_ID;
}

/**
 *  Name:  clCorCliStrPrint
 *
 *  Copies srting to return string of DBG CLI
 *  @param  str: String to be copied
 *          retStr: Return string of DBG CLI
 *
 *  @returns
 *    none
 */
                                                                                                                             
void clCorCliStrPrint(ClCharT* str, ClCharT**retStr)
{
    if (retStr != NULL)
    {
        *retStr = clHeapAllocate(strlen(str)+1);
        if(NULL == *retStr)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Malloc Failed \r\n"));
            return;
        }
        sprintf(*retStr, str);
    }
    else
        clLogError("CLI", "PTR", "NULL pointer passed for the retStr");
    return;
}
                                                                                                                             


/* Attribute Conversion Table */

/* This command creates COR class. It expects class name, class Id & superclass Id
      It will add an entry in Ni table */
ClRcT
cliCorClassCreate(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClInt32T classId;
    ClInt32T parentClassId;
    ClInt32T tmpClassId;
    ClRcT ret;
    char temp[100] = {'\0'};
    int i = 0;

    if((argc < 2) || (argc > 4))
    {
        clCorCliStrPrint("\nUsage: corClassCreate <Class name> <Class Id> <Parent Class>\n \
          <Class name>      : [STRING]              - Eg. EthernetPort. It should not include special characters\n \
          <Class Id>        : [HEX/DEC]             - Eg. 0x102 or 258 \n \
          <Parent Class>    : [STRING or HEX/DEC]   - Eg. Port or 0x103 or 259 \n" , retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
 
    /* Check if the class name does not have special characters or numbers */
    while(argv[1][i] != '\0')
    {
        if(!isalpha(argv[1][i]))
        {
            clCorCliStrPrint("\nThe class name can not have digits or"
                  "special characters\n \nCOR class creation failed\n", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
        i++;
    }

    /*TODO: The logic of getting the class id should be changed.
     */
    switch (argc)
    {
        case 2:
            /* only class name is given */
            if ((ClUint32T) gClCorClassIdAlloc + 1 < gClCorClassIdAlloc)
            {
                clCorCliStrPrint("Class Ids reached the maximum limit.", retStr);
                return CL_COR_SET_RC(CL_ERR_OUT_OF_RANGE);
            }

            classId = gClCorClassIdAlloc++;
            parentClassId = 0;
            break;

        case 3:
            /* class name and class id is given */
            classId = corAtoI(argv[2]);
            if (classId <= 0)
                classId = gClCorClassIdAlloc++;

            parentClassId = 0;
            break;

        case 4:
            /* super class id is also given */
            classId = corAtoI(argv[2]);
            if (classId <= 0)
                classId = gClCorClassIdAlloc++;

            if (isdigit(argv[3][0]))
                parentClassId = corAtoI(argv[3]);
            else
            {
                ret = clCorNIClassIdGet(argv[3], &parentClassId);
                if (ret != CL_OK)
                {
                    clCorCliStrPrint("Parent class doesn't exist.", retStr);
                    return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
                }
            }
            break;

        default:
            clCorCliStrPrint("Invalid no. of arguments given.", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    if( (ret = clCorNIClassIdGet(argv[1], &tmpClassId)) == CL_OK)
    {
        /* Class Id already defined. check if it is same as that one passed */
        /* Get the classId passed by user */
        if(tmpClassId != classId)
        {
            clCorCliStrPrint("\nERROR: The name is already assigned to"
                  "another class !!!! \n", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
        else
        {
            /*Entry already present in NI table. Create class */
            goto TryToCreateCORClass;
        }
    }
        
    if( (ret = clCorNIClassNameGet(classId, temp)) == CL_OK)
    {
        /* Yes the classId is associated with some name. Check if it is same as passed */
        if(strcmp(temp, argv[1]) != 0)
        {
            /* The class Id is associated with different name */
            clCorCliStrPrint("\nERROR: The class Id is already assigned with"
                  " different name  !!!!\n", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }
        else
        {
            /* Entry already present. go to class create */
            goto TryToCreateCORClass;
        }
    }

    /* Add an entry in NI Table */
    if( (ret = clCorNIClassAdd(argv[1], classId)) != CL_OK)
    {
            clCorCliStrPrint("\nCould not add an entry in NI Table\n",
                  retStr);
            return ret;
    }
    
TryToCreateCORClass :

    ret = clCorClassCreate(classId, parentClassId);
    if(ret != CL_OK)
    {
        gCorCliStr[0]='\0';
        sprintf(gCorCliStr, "\n Could not create COR class, rc = 0x%x \n",
            ret);
        clCorCliStrPrint(gCorCliStr, retStr);

        clCorNIClassDelete(classId);
    }
    else
    {
        gCorCliStr[0]='\0';
        sprintf(gCorCliStr, "\nExecution Result : PASSED\n");
        clCorCliStrPrint(gCorCliStr, retStr);
    }

    return ret;
}

ClRcT
cliCorClassDelete(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{

    ClInt32T classId;
    ClRcT rc;
    
    if(argc != 2)
    {
        clCorCliStrPrint("\nUsage: corClassDelete <Class Id> \n \
       <Class Id > : [HEX or DEC] - Ex.  0x102 or 258 \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    classId = corAtoI(argv[1]);

    /* Handle Obtained try to delete object */
    if( (rc = clCorClassDelete(classId)) != CL_OK)
    {
            gCorCliStr[0]='\0';
            sprintf(gCorCliStr, "\n COR class Deletion failed. rc = 0x%x \n",
                rc);
            clCorCliStrPrint(gCorCliStr, retStr);
            return rc;
    }

    gCorCliStr[0]= '\0';
    sprintf(gCorCliStr, "\nExecution Result : PASSED\n");
    clCorCliStrPrint(gCorCliStr, retStr);
    
    return CL_OK;
}

ClRcT
cliCorClassSimpleAttributeCreate(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{

    ClInt32T classId;
    ClInt32T attrId;
    char *attrType;
    ClRcT rc;
    int entryFound;
    cliAttrTypeMap_h attrTypeMapPtr;
    ClCorTypeT attrTypeEnum = CL_COR_INVALID_DATA_TYPE;
    
    if(argc != 4)
    {
        clCorCliStrPrint("\nUsage: corClassAttrCreate <Class Id> <Attr Id> <Attr Type> \n\
            <Class Id >  : [HEX or DEC] - Ex.  0x102 or 258   \n \
            <Attr Id  >  : [HEX or DEC] - Ex.  0x1022 or 4130 \n \
            <Attr Type > : [STRING]  - UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64\n ", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    classId = corAtoI(argv[1]);
    attrId = corAtoI(argv[2]);
    attrType = argv[3];
    attrTypeMapPtr = cliAttrTypes;
    entryFound = CL_FALSE;
     
    while((entryFound != CL_TRUE) && (attrTypeMapPtr->type != NULL))
    {
        if(strcasecmp(attrTypeMapPtr->type, attrType) == 0)
        {
            attrTypeEnum = attrTypeMapPtr->id;
            entryFound = CL_TRUE;
        }

        attrTypeMapPtr++;
    }

    if( entryFound != CL_TRUE)
    {
        clCorCliStrPrint("\nUsage ERROR :- Provide proper attribute type. For e.g. for unsigned 32 bit int, type UINT32 \n", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    /* Handle Obtained try to delete object */
    if( (rc = clCorClassAttributeCreate(classId,attrId,attrTypeEnum)) != CL_OK)
    {
            gCorCliStr[0]='\0';
            sprintf(gCorCliStr, "\nERROR: COR class Attribute Create failed !!!! rc = 0x%x \n",
                rc);
            clCorCliStrPrint(gCorCliStr, retStr);
            return (rc);
    }

    clCorCliStrPrint("\nExecution Result : PASSED\n", retStr);
    return CL_OK;
}

ClRcT
cliCorClassAttributeValueSet(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClInt32T classId;
    ClInt64T attrId;
    ClInt64T init;
    ClInt64T min;
    ClInt64T max;
    ClRcT rc;
    
    if(argc != 6)
    {
        clCorCliStrPrint("\nUsage: corClassAttrValSet <Class Id> <Attr Id> <Init> <Min> <Max> \n\
        <Class Id > : [HEX or DEC] - Ex.  0x102 or 258 \n \
        <Attr Id  > : [HEX or DEC] - Ex.  0x1022 or 4130 \n \
        <Init >     : [HEX or DEC] - Initial value for the attribute. \n \
        <Min  >     : [HEX or DEC] - Minimum value which the attribute can take.\n \
        <Max  >     : [HEX or DEC] - Maximum value which the attribute can take. \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    classId = corAtoI(argv[1]);
    attrId = corAtoI(argv[2]);
    init = corAtoI(argv[3]);
    min = corAtoI(argv[4]);
    max = corAtoI(argv[5]);

    /* Handle Obtained try to delete object */
    if( (rc = clCorClassAttributeValueSet(classId,attrId,init,min,max)) != CL_OK)
    {
        gCorCliStr[0]='\0';
        sprintf(gCorCliStr, "\nERROR: COR class attr val set failed !!!! rc = 0x%x \n",
            rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        return (rc);
    }

    clCorCliStrPrint("\nExecution Result : PASSED\n", retStr);

    return CL_OK;
}


ClRcT 
cliCorMOClassCreateByName(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClInt32T maxInstances;
    ClRcT rc = CL_OK;

    if (argc != 3)
    {
        clCorCliStrPrint("\nUsage: moClassCreate <Mo Class Path> <Max Instances> \n \
          <Mo Class Path >   : [HEX or STRING] -  Ex. \\Chassis\\Blade or  \\0x102\\0x103 \n \
          <Max Instances >   : [HEX or DEC]    -  Maximum number of instances for the MO class. \n",
          retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        maxInstances = corAtoI(argv[2]);
        maxInstances = (maxInstances < 0) ? 0 : maxInstances;

        rc = clCorMOClassCreateByName(argv[1], maxInstances);

        if (rc != CL_OK)
        {
            clLogError("CLI", "MOC", "MO Class creation failed. rc [0x%x]", rc);
            clCorCliStrPrint("MO class creation failed.", retStr);
            return rc;
        }

        clCorCliStrPrint("Execution Result : PASSED", retStr);
    }

    return CL_OK;
}

ClRcT
cliCorMOClassDeleteNodeByName(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT rc = CL_OK;

    if (argc != 2)
    {
        clCorCliStrPrint("\n Usage: moClassDelete <Mo class Path> \n \
	        <Mo Class Path > : [HEX or STRING] - Ex. \\chassis\\blade or \\0x102\\0x103 \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        rc = clCorMOClassDeleteNodeByName(argv[1]);
        if (rc != CL_OK)
        {
            clLogError("CLI", "DEL", "Failed to delete the MO class. rc [0x%x]", rc);
            clCorCliStrPrint("\nFailed to delete the MO class. \n", retStr);
            return rc;
        }

        clCorCliStrPrint("\nExecution Result : PASSED\n", retStr);
    }

    return(CL_OK);
}


ClRcT
cliCorMSOClassCreateByName(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT msoClassId = 0;

    if (argc != 4)
    {
        clCorCliStrPrint("\nUsage: msoClassCreate <Mo Class Path> <Service ID> <ClassType> \n \
            <Mo Class Path>     : [STRING or HEX]       -   Eg. \\Chassis\\Blade or \\0x102\\0x103 \n \
            <Service ID>        : [HEX/DEC]             -   Service ID of the MSO class.\n \
                                                            Provisiong Service ID   : [3] \n \
                                                            Alarm Service ID        : [2] \n \
            <ClassType>         : [STRING or HEX/DEC]   -   Class Name/ID of the MSO class.\n \
                                                            Eg. ProvBlade or 0x201", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        if (isdigit(argv[3][0]))
            msoClassId = corAtoI(argv[3]);
        else
        {
            rc = clCorNIClassIdGet(argv[3], &msoClassId);
            if (rc != CL_OK)
            {
                clCorCliStrPrint("Mso class doesn't exist.", retStr);
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
            }
        }

        rc = clCorMSOClassCreateByName(argv[1], corAtoI(argv[2]), msoClassId);
        if (rc != CL_OK)
        {
            clLogError("CLI", "MSC", "Failed to create the MSO class. rc [0x%x]", rc);
            clCorCliStrPrint("\nFailed to create the MSO class.\n", retStr);
            return rc;
        }

        clCorCliStrPrint("\nExecution Result : PASSED\n", retStr);
    }

    return(CL_OK);
}

ClRcT
cliCorMSOClassDeleteByName(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT rc = CL_OK;

    if (argc != 3)
    {
        clCorCliStrPrint("\nUsage: msoClassDelete <Mo Class Path> <Service ID> \n \
            <Mo Class Path >       : [HEX or STRING] - \\chassis\\blade or \\0x102\\0x103 \n \
	        <Service ID >           : [HEX or DEC]           -  Service ID of the MSO class. \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        rc = clCorMSOClassDeleteByName(argv[1], corAtoI(argv[2]));
        if (rc != CL_OK)
        {
            clLogError("CLI", "MSD", "Failed to delete the MSO class. rc [0x%x]", rc);
            clCorCliStrPrint("\nFailed to delete the MSO class.\n", retStr);
            return rc;
        }

        clCorCliStrPrint("\nExecution Result : PASSED\n", retStr);
    }

    return(CL_OK);
}

ClRcT
cliCorObjCreate(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_COR_MAX_NAME_SZ] = {0};

    if( argc < 2 )
    {
        clCorCliStrPrint("\nUsage: corObjCreate <MoId> <Service ID> \n \
            <MoId>       : [STRING or HEX]  -   \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n \
            <Service ID> : [HEX or  DEC]    -   Service ID of the Object \n \
                                                Provisioning Object : [3] \n \
                                                Alarm Object        : [2] \n ", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {       
        if(argc < 3)
        {
            rc = corObjCreateByName( argv[1], CL_COR_INVALID_SRVC_ID);
            if (CL_OK != rc)
            {
                sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
                clCorCliStrPrint(corStr, retStr);
                return rc;
            }
        }
        else
        {
            rc = corObjCreateByName( argv[1], corAtoI(argv[2]));
            if (CL_OK != rc)
            {
                sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
                clCorCliStrPrint(corStr, retStr);
                return rc;
            }
        }
    }
    
    clCorCliStrPrint("Execution Result : PASSED", retStr);

    return(CL_OK);
}

ClRcT
cliCorObjDelete(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClCharT corStr[CL_MAX_NAME_LENGTH];
    ClRcT rc = CL_OK;

    if( argc < 2 )
    {
        clCorCliStrPrint("\nUsage: corObjDelete <MoId> <Service ID> \n \
            <MoId >       : [HEX or STRING] -  \\chassis:0\\blade:1 or \\0x102:0\\0x103:1 \n \
            <Service ID > : [HEX or DEC]		  -  Service ID of the Object \n", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        if(argc < 3)
        {
            rc = corObjDeleteByName( argv[1], CL_COR_INVALID_SRVC_ID);
            if (rc != CL_OK)
            {
                sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
                clCorCliStrPrint(corStr, retStr);
                return rc;
            }
        }
        else
        {
            rc = corObjDeleteByName( argv[1], corAtoI(argv[2]));
            if (rc != CL_OK)
            {
                sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
                clCorCliStrPrint(corStr, retStr);
                return rc;
            }
        }
    }

    clCorCliStrPrint("Execution Result : PASSED", retStr);

    return(CL_OK);
}

/* There is only one argument which is moId */
ClRcT cliCorMoAndMsoObjectsCreate(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moid;
    ClCorObjectHandleT tempHandle;
    ClCharT     corStr[CL_COR_MAX_NAME_SZ] = {0};

    if(argc != 2)
    {
        clCorCliStrPrint("\nUsage: corMoMsoObjCreate <MoId> \n \
          <MoId >       : [STRING or HEX] -  \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    rc = corXlateMOPath(argv[1], &moid);
    if (CL_OK == rc)
    {
        rc = clCorUtilMoAndMSOCreate(&moid, &tempHandle);
        if(CL_OK != rc)
        {
            sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
            clCorCliStrPrint(corStr, retStr);
            return rc;
        }
    }
    else
    {
        sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
        clCorCliStrPrint(corStr, retStr);
        return rc;
    }

    clCorCliStrPrint("Execution Result : PASSED", retStr);
    return CL_OK;
}


ClRcT cliCorMoAndMsoObjectsDelete(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moid;
    ClCharT     corStr[CL_COR_MAX_NAME_SZ] = {0};

    if(argc != 2)
    {
        clCorCliStrPrint("\nUsage: corMoMsoObjDelete <MoId> \n \
            <MoId >       : [STRING or HEX] -  \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    rc = corXlateMOPath(argv[1], &moid);
    if (CL_OK == rc)
    {
        rc = clCorUtilMoAndMSODelete(&moid);
         if(CL_OK != rc)
        {
            sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
            clCorCliStrPrint(corStr, retStr);
            return rc;
        }
    }
    else
    {
        sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
        clCorCliStrPrint(corStr, retStr);
        return rc;
    }

    clCorCliStrPrint("Execution Result : PASSED", retStr);
    return CL_OK;
}

ClRcT cliCorSubTreeDelete(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];

    CL_FUNC_ENTER();

    if (argc != 2)
    {
        clCorCliStrPrint("\nUsage: corSubTreeDelete <MoId> \n \
            <MoId>       : [HEX or STRING] - \\chassis:0\\blade:1 or \\0x102:0\\0x103:1 \n", retStr);
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
   
    rc = corSubTreeDeleteByName(argv[1]);
    if (rc != CL_OK)
    {
        sprintf(corStr, "Execution Result : FAILED. rc [0x%x]", rc);
        clCorCliStrPrint(corStr, retStr);
        CL_FUNC_EXIT();
        return rc;
    }
    
    clLogInfo("CLI", "OBT", "SubTree [%s] is deleted successfully", argv[1]);

    clCorCliStrPrint("Execution Result : PASSED.", retStr);

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT 
cliCorSet(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClInt64T value;
    ClUint32T size = 0;
    ClBufferHandleT inMsgHandle;
    ClUint8T *pTempBuff = NULL;
    ClRcT rc = CL_OK;
    ClCharT corStr[CL_MAX_NAME_LENGTH];
    
    if (argc != 7)
    {
        clCorCliStrPrint("\nUsage: attrSet <MoId> <Service ID> <Attribute Path> <Attribute ID> <Index> <Value> \n \
            <MoId >           : [HEX or STRING] - Ex. \\chassis:0\\blade:1 or \\0x102:0\\0x103:1 \n \
            <Service ID >     : [HEX or DEC]    - Service Id of the object.\n \
            <Attribute Path > : [HEX or STRING] - Ex. \\AttrA:0\\AttrB:0 or \\0x1034:0\\0x201:1. It can be null. \n \
            <Attribute ID >   : [HEX or DEC]    - Ex. 0x300. Attribute to be set.\n \
            <Index >          : [HEX or DEC]    - It is -1 for Simple Attribute. Some valid value for Array Attribute.\n \
            <Value >          : [HEX or DEC]    - Value to set. \n", retStr);

            return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        rc = clBufferCreate (&inMsgHandle);
        if (rc != CL_OK)
        {
            clLogError("CLI", "SET", "Failed to create the buffer. rc [0x%x]", rc);
            sprintf(corStr, "Failed to create the buffer message. rc [0x%x]", rc);
            clCorCliStrPrint(corStr, retStr);
            return rc;
        }

        if ((argv[6][1] == 'x') || (argv[6][1] == 'X'))
            value = (ClInt64T)strtoll(argv[6], NULL, 16);
        else
            value = (ClInt64T)strtoll(argv[6], NULL, 10);
 
        if( (!strcasecmp(argv[3],"null")) || (!strcmp(argv[3],"0")) )
            rc = corObjAttrSetByName(argv[1], corAtoI(argv[2]),  NULL,
                    corAtoI(argv[4]), corAtoI(argv[5]), value, &inMsgHandle);
        else
            rc = corObjAttrSetByName(argv[1], corAtoI(argv[2]),  argv[3],
                    corAtoI(argv[4]), corAtoI(argv[5]), value, &inMsgHandle);
         
        clBufferLengthGet(inMsgHandle, &size);
        if(size > 0)
        {
            pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);
            if(pTempBuff == NULL)
            {
                clLogError("CLI", "SET", "Memory Allocation Failed");
                sprintf(corStr, "Failed to allocate memory");
                clCorCliStrPrint(corStr, retStr);
                clBufferDelete(&inMsgHandle);
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }

            memset(pTempBuff, '\0', size + 1);

            clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
            clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
            clHeapFree(pTempBuff);
        }

        clBufferDelete(&inMsgHandle);
    }

    return (rc);
}

ClRcT 
cliCorGet(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClInt64T value=0;
    ClInt32T index = 0;
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    ClRcT rc = CL_OK;

    if (argc < 5 || argc >6) 
    {
         clCorCliStrPrint("\nUsage: attrGet <MoId> <Service ID> <Attribute Path> <Attribute ID> <Index> \n \
	        <MoId >           : [HEX or STRING] - Ex. \\chassis:0\\blade:1 or \\0x102:0\\0x103:1 \n \
	        <Service ID >     : [HEX or DEC]    - Service Id of the object.\n \
	        <Attribute Path > : [HEX or STRING] - Ex. \\AttrA:0\\AttrB:0 or \\0x1034:0\\0x201:1. It can be null. \n \
	        <Attribute ID >   : [HEX or DEC]    - Ex. 0x300. Attribute to be get.\n \
	        <Index >          : [HEX or DEC]    - It is -1 for Simple Attribute. Some valid value for Array Attribute.\n            ", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {
        if(argc == 5)   /* simple attribute */
            index = -1;
        else           /* array */
            index = corAtoI(argv[5]);

        clBufferCreate (&inMsgHandle);
        if( (!strcasecmp(argv[3],"null")) || ( !strcmp (argv[3],"0")) )
            rc = corObjAttrGetByName(argv[1], corAtoI(argv[2]),NULL,
                   corAtoI(argv[4]), index, &value, &inMsgHandle);
        else
            rc = corObjAttrGetByName(argv[1], corAtoI(argv[2]),argv[3],
                   corAtoI(argv[4]), index, &value, &inMsgHandle);

        clBufferLengthGet(inMsgHandle, &size);
        if(size>0)
        {
            pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);
            if(pTempBuff == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }
            memset(pTempBuff, '\0', size + 1);
            clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
            clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
            clHeapFree(pTempBuff);
        }

        clBufferDelete(&inMsgHandle);
    }

    return(rc);
}

ClRcT
cliCorObjTreeShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    ClRcT rc = CL_OK;

    if (argc > 2)
    {
        clCorCliStrPrint("\nUsage: objTreeShow <MoId>\n \
	            <MoId>           : [HEX or STRING] - Eg. \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n \
	            \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    clBufferCreate (&inMsgHandle);

    if (argc == 1)
        rc = corObjTreeWalkByName(0, &inMsgHandle);
    else
        rc = corObjTreeWalkByName(argv[1], &inMsgHandle);

    clBufferLengthGet(inMsgHandle, &size);

    if(size > 0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);
        if(pTempBuff == NULL)
        {
            clLogError("CLI", "OBT", "Memory Allocation Failed");
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pTempBuff, '\0', (size + 1));
        clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    clBufferDelete(&inMsgHandle);

    return(rc);
}

ClRcT
cliCorMOClassTreeShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    clBufferCreate (&inMsgHandle);

    if (argc > 1)
    {
        clCorCliStrPrint("\nUsage: moClassTreeShow \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    corMOTreeShow(&inMsgHandle);
    clBufferLengthGet(inMsgHandle, &size);

    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size+1);
        if(pTempBuff == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pTempBuff, 0, size+1);
        clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    clBufferDelete(&inMsgHandle);

    return(CL_OK);
}


ClRcT
cliCorDMShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    clBufferCreate (&inMsgHandle);
    ClCharT *pData = NULL;
                                                                                                                            
    if (argc > 2)
    {
        clCorCliStrPrint("\nUsage: dmShow  <classId> \n \
	        <classId>           : [HEX or DEC] - Ex. 0x102 or 258 \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if(argc == 2)
        pData = argv[1]; 

    dmShow(pData, &inMsgHandle);
    clBufferLengthGet(inMsgHandle, &size);

    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);

        if(pTempBuff == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pTempBuff, 0, size + 1);
        clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    clBufferDelete(&inMsgHandle);

    return(CL_OK);
}

ClRcT
cliCorObjectShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClBufferHandleT buffer = 0;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    ClRcT rc = CL_OK;
    ClInt32T svcId = CL_COR_INVALID_SVC_ID; 
    ClCorMOIdT moId = {{{0}}};
    ClCharT corStr[CL_COR_CLI_STR_LEN] = {0};

    if (argc > 4 || argc < 2)
    {
         clCorCliStrPrint("\nUsage: objectShow <MoId> <MSO Service ID> <Attribute Path>\n \
	        <MoId> [HEX or STRING]            : MoId of the object. Eg. \\Chassis:0\\Blade:1 (or) \\0x102:0\\0x103:1\n \
	        <MSO Service ID> [STRING or INT]  : Service Id of the MSO object.\n \
                                                   \"fault\" (or) 1 : Fault MSO.\n \
                                                   \"alarm\" (or) 2 : Alarm MSO.\n \
                                                   \"prov\"  (or) 3 : Provisioning MSO.\n \
                                                   \"pm\"    (or) 4 : Performance Management MSO.\n \
                                                   \"amf\"   (or) 5 : AMF Management MSO.\n \
                                                   \"chm\"   (or) 6 : Chassis Management MSO.\n \
	        <AttributePath> [HEX or STRING]   : Attribute path of containment object. Eg. \\AttrA:0\\AttrB:1 (or) \\0x1034:0\\0x201:1\n",
            retStr);

         return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    rc = clBufferCreate (&buffer);
    if (rc != CL_OK)
    {
        clLogError("CLI", "OBJECTSHOW", "Failed to create buffer. rc [0x%x]", rc);
        return rc;
    }

    rc = corXlateMOPath(argv[1], &moId);
    if (rc != CL_OK)
    {
        clLogError("CLI", "OBJECTSHOW", "Invalid MoId specified. rc [0x%x]", rc);
        clBufferDelete(&buffer);
        return rc;
    }
    
    if(argc == 2)
    {
        /* Show all the MSO objects. */
        for (svcId = CL_COR_SVC_ID_DUMMY_MANAGEMENT + 1; svcId < CL_COR_SVC_ID_MAX; svcId++)
        {
            rc = clCorMoIdServiceSet(&moId, svcId);
            if (rc != CL_OK)
            {
                sprintf(corStr, "ERROR: Invalid service Id specified. rc [0x%x]", rc);
                clBufferNBytesWrite(buffer, (ClUint8T *) corStr, strlen(corStr));
                goto exit;
            }

//        rc = _clCorObjectHandleGet(&moId, &objHandle);
            rc = _clCorObjectValidate(&moId);
            if(CL_OK != rc)
            {
                /* Service is not configured for this MO. */
                rc = CL_OK;
                continue;
            }

            rc = corObjShowByName(&moId, svcId, NULL, &buffer);
            if (rc != CL_OK)
                goto exit;
        }
    }
    else if (argc > 2)
    {
        /* Service Id is specified. */
        if (isdigit(argv[2][0]))
            svcId = corAtoI(argv[2]);
        else
            svcId = clCorServiceIdStringToValueGet(argv[2]);

        rc = clCorMoIdServiceSet(&moId, svcId);
        if ((rc != CL_OK) || (svcId == CL_COR_INVALID_SVC_ID))
        {
            sprintf(corStr, "ERROR: Invalid service Id specified.");
            clBufferNBytesWrite(buffer, (ClUint8T *) corStr, strlen(corStr));
            goto exit;
        }

//        rc = _clCorObjectHandleGet(&moId, &objHandle);
        rc = _clCorObjectValidate(&moId);
        if(CL_OK != rc)
        {
            sprintf(corStr, "Invalid MoId passed or [%s] service is not configured for this MO.",
                    clCorServiceIdString[moId.svcId]);
            clBufferNBytesWrite (buffer, (ClUint8T*) corStr, strlen(corStr));
            goto exit;
        }

        if (argc == 4)
            rc = corObjShowByName(&moId, svcId, argv[3], &buffer);
        else
            rc = corObjShowByName(&moId, svcId, NULL, &buffer);

        if (rc != CL_OK)
            goto exit;
    }

exit:    
    clBufferLengthGet(buffer, &size);

    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size+1);
	    if(pTempBuff == NULL)
	    {
		    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
            clBufferDelete(&buffer);
		    return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
	    }

        memset(pTempBuff, 0, size + 1);
        clBufferNBytesRead(buffer, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    /* Delete the message buffers */
    clBufferDelete(&buffer);

    return(rc);
}

ClRcT
cliCorNiShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
                                                                                                                            
    if (argc > 1)
    {
        clCorCliStrPrint("\nUsage: niShow \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    clBufferCreate (&inMsgHandle);

    corNiTableShow(&inMsgHandle);

    clBufferLengthGet(inMsgHandle, &size);
    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size+1);
        if(pTempBuff == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pTempBuff, 0, size+1);
        clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    clBufferDelete(&inMsgHandle);

    return(CL_OK);
}


ClRcT
cliCorRmShow(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    
    ClBufferHandleT inMsgHandle;
    ClUint32T size = 0;
    ClUint8T *pTempBuff = NULL;
    clBufferCreate (&inMsgHandle);
    
    rmShow(NULL, &inMsgHandle);
    clBufferLengthGet(inMsgHandle, &size);
    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size+1);
        if(pTempBuff == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory Allocation Failed"));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        memset(pTempBuff, 0, size+1);
        clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
    }

    clBufferDelete(&inMsgHandle);

    return(CL_OK);
}


/**
  Utility Routine to convert the string to number.
  It takes cares if the number is given in Hex or Decimal form.
 */

ClInt64T corAtoI(char *str)
{
    ClUint8T len;
    ClInt64T outVal;
    ClUint64T temp;

    len = strlen(str);
    /* if string length < 2, then the number can-not be in hex for sure.*/
    if (len<=2)
    {
            sscanf(str, "%lld", &outVal);
        return outVal;
    }
    else
    {   
        ClUint8T tmplen = strlen("0x");
        /* check if entered number is in hex format */
        if(!strncmp(str, "0x", tmplen) || !strncmp(str, "0X", tmplen))
        {
            sscanf(str, "%llx", &temp);
            outVal = (ClInt64T)temp;
            return outVal;
        }
        else
        {
            sscanf(str, "%lld", &outVal);
            return outVal;
        }
    }
}



/**
 * The callback function for the get bundle.
 */

ClRcT clCorDbgBundleCallback(CL_IN ClCorBundleHandleT bundleHandle,
                                  CL_IN ClPtrT          userArg )
{
    ClRcT           rc = CL_OK;
    SaNameT         moIdName    = {0};
    ClCntNodeHandleT    nodeH  = 0;
    ClCorDbgBundleDataPtrT pCorBundleData = NULL;
    ClCorDbgGetBundlePtrT  pCorGetBundle = (ClCorDbgGetBundlePtrT)userArg;

    clOsalMutexLock(pCorGetBundle->corGetMutexId);

    if(NULL != pCorGetBundle)
    {
        rc = clCntFirstNodeGet(pCorGetBundle->cntHandle, &nodeH);
        if(CL_OK != rc)
        {
            gCorCliStr[0] = '\0';
            sprintf (gCorCliStr, "\n Cannot find the first bundle container handle. rc[0x%x] \n", rc);
            clBufferNBytesWrite(pCorGetBundle->msgH, (ClUint8T *)gCorCliStr, strlen(gCorCliStr)); 
            goto handleError;
        }

        while(nodeH)
        {
            rc = clCntNodeUserDataGet(pCorGetBundle->cntHandle, nodeH, (ClCntDataHandleT *)&pCorBundleData);
            if(CL_OK != rc)
            {
                gCorCliStr[0] = '\0';
                sprintf (gCorCliStr, "\n Failed to get the bundle job data . rc[0x%x] \n", rc);
                clBufferNBytesWrite(pCorGetBundle->msgH, (ClUint8T *)gCorCliStr, strlen(gCorCliStr)); 
                goto handleError;
            } 

            if(NULL == pCorBundleData)
            {
                gCorCliStr[0] = '\0';
                sprintf (gCorCliStr, "\n NULL pointer obtained for bundle job data.\n");
                clBufferNBytesWrite(pCorGetBundle->msgH, (ClUint8T *)gCorCliStr, strlen(gCorCliStr)); 
                goto handleError;
            }

            if(pCorBundleData->jobStatus != CL_OK)
            {
               gCorCliStr[0] = '\0';
                /* Get the moId name from moid. */
                rc = clCorMoIdToMoIdNameGet(&pCorBundleData->moId, &moIdName);
                if(rc != CL_OK)
                    sprintf (gCorCliStr, "\n Get failed for : AttrId[0x%x], Error[0x%x] \n", 
                            pCorBundleData->attrId, pCorBundleData->jobStatus);
                else
                    sprintf (gCorCliStr, "\n Get failed for : MoId [%s] AttrId[0x%x] Error[0x%x] \n", 
                        moIdName.value, pCorBundleData->attrId, pCorBundleData->jobStatus);
                clBufferNBytesWrite(pCorGetBundle->msgH, (ClUint8T *)gCorCliStr, strlen(gCorCliStr)); 
            }
            else
            {
                /* Function for creating the display string based on the apposite type. */
                _clCorDisplayInfoAppend(pCorBundleData, pCorGetBundle->msgH);
            }
            
            rc = clCntNextNodeGet(pCorGetBundle->cntHandle, nodeH, &nodeH);
            if(CL_OK != rc)
            {
                if(CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
                {
                    rc = CL_OK;
                    goto handleError;
                }

                gCorCliStr[0] = '\0';
                sprintf (gCorCliStr, "\n Failed to get the next bundle job data node. rc[0x%x] \n", rc);
                clBufferNBytesWrite(pCorGetBundle->msgH, (ClUint8T *)gCorCliStr, strlen(gCorCliStr)); 
                goto handleError;
            }
        }
    }
    else
    {
        clLogError("CLI", "GET", 
                "Bundle data obtained is NULL for bundle handle [%#llX]", 
                bundleHandle);
		return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

handleError:

    clOsalCondSignal(pCorGetBundle->corGetCondId);

    clOsalMutexUnlock(pCorGetBundle->corGetMutexId);

    return rc;
}

/**
 * This function will create the bundle handle. This handle would be used 
 * in all the subsequent calls of clCorBundleObjAttrGet and finally clCorBundleCommit
 * to get all the attribute values.
 */

ClRcT
cliCorBundleInitialize(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                   rc = CL_OK;
    ClCorBundleHandleT bundleHandle = -1;
    ClCorDbgGetBundlePtrT  pCorGetBundle = NULL;
    ClCorBundleConfigT      bundleConfig = {0};
    ClUint32T               bundleType  = 0;
    ClCorBundleHandleT      *pBundleHandle = NULL;


    if(argc != 2)
    {
        clCorCliStrPrint("\n Usage: corBundleInitialize <Bundle Type> \n \
        <Bundle Type> [DEC]: The type of bundle. It can have following values \n \
        1 - Transactional Bundle. All the object modification operation \n  \
        2 - Non-Transactional Bundle. All the get operation are done using non-transactional bundle. \n", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    bundleType = corAtoI(argv[1]);

    if(bundleType == CL_COR_BUNDLE_TRANSACTIONAL)
    {
        /*Cannot Initialize a transactional bundle in this release.*/
        clCorCliStrPrint("Transactional bundle creation is not supported in this release. ", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else if (bundleType == CL_COR_BUNDLE_NON_TRANSACTIONAL)
        bundleType = CL_COR_BUNDLE_NON_TRANSACTIONAL;
    else
    {
        clCorCliStrPrint("Invalid bundle type specified.", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    pCorGetBundle = clHeapAllocate(sizeof(ClCorDbgGetBundleT));
    if(NULL == pCorGetBundle)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while allocating the memory. rc[0x%x]", CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        clCorCliStrPrint(gCorCliStr, retStr);
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    
    memset(pCorGetBundle, 0, sizeof(ClCorDbgGetBundleT));

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while creating the bundle. rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto handleError;
    }        

    rc = clOsalMutexCreate(&pCorGetBundle->corGetMutexId);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while creating the mutex. rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto handleError;
    }        

    rc = clOsalCondCreate(&pCorGetBundle->corGetCondId);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = 0;
        snprintf(gCorCliStr, sizeof(gCorCliStr), "Cond create failed with [%#x]\n", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto handleError;
    }
    rc = clBufferCreate(&pCorGetBundle->msgH);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while creating the message handle. rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto handleError;
    }        

    rc = clCorDbgBundleJobCntCreate(&pCorGetBundle->cntHandle);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while creating the bundle job container.  rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto handleError;
    }

    pBundleHandle = clHeapAllocate(sizeof(ClCorBundleHandleT));
    if (NULL == pBundleHandle)
    {
        clCorCliStrPrint("Failed while allocating the handle for storing in the container", retStr);
        goto handleError;
    }

    *pBundleHandle = bundleHandle;

    rc = clCntNodeAdd(corDbgBundleCnt, (ClCntKeyHandleT)pBundleHandle, (ClCntDataHandleT)pCorGetBundle, NULL);
    if(CL_OK != rc)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while adding the bundle handle.  rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        clHeapFree(pBundleHandle);
        goto handleError;
    }

    gCorCliStr[0] = '\0';
    sprintf (gCorCliStr, "\n The bundle handle is [%d]", (ClUint32T)bundleHandle);
    clCorCliStrPrint(gCorCliStr, retStr);

    return rc;

handleError:
    clCorCliStrPrint("The bundle initialization failed. \n", retStr);
    if(pCorGetBundle != NULL)
    {
        if(pCorGetBundle->corGetMutexId)
            clOsalMutexDelete(pCorGetBundle->corGetMutexId);
        if(pCorGetBundle->corGetCondId)
            clOsalCondDelete(pCorGetBundle->corGetCondId);
        if(pCorGetBundle->msgH)
            clBufferDelete(&pCorGetBundle->msgH);
        clHeapFree(pCorGetBundle);
    }
    return rc;
}


/**
 * This function would be used to queue all the attribute 
 * set operations which are part of a single bundle.
 */

ClRcT
cliCorBundleSetJobAdd(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    clCorCliStrPrint("corBundleSetJobAdd: Do not support adding the set job to the bundle.", retStr);
    return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
}

/**
 * This function would be used to queue all the MO creation
 * jobs in the bundle.
 */

ClRcT
cliCorBundleCreateJobAdd(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    clCorCliStrPrint("corBundleCreateJobAdd: Do not support adding the create job to the bundle.", retStr);
    return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
}


/**
 * This function would be used to queue all MO deletion  
 * jobs in the bundle.
 */

ClRcT
cliCorBundleDeleteJobAdd(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    clCorCliStrPrint("corBundleDeleteJobAdd: Do not support adding the delete job to the bundle.", retStr);
    return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
}

/**
 * This function would be used to queue all the attribute 
 * get operations which are part of a single bundle.
 */

ClRcT
cliCorBundleGetJobAdd(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                   rc          = CL_OK;
    ClInt32T                index       = 0;
    ClBufferHandleT         inMsgHandle = 0;
    ClUint32T               size        = 0;
    ClUint8T                *pTempBuff  = NULL;
    ClCorBundleHandleT      bundleHandle = 0;
    ClCntNodeHandleT        nodeH = 0;
    ClCorDbgGetBundlePtrT  pCorGetBundle = NULL;


    if (argc != 7) 
    {
         clCorCliStrPrint("\nUsage: corBundleGetJobAdd <bundleHandle> <MoId> <Service ID> <Attribute Path> <Attribute ID> <Index> \n \
                      <bundleHandle>   : [DEC]           - Handle obtained after executing clCorBundleInitialize   \n \
                      <MoId >           : [HEX or STRING] - Ex. \\chassis:0\\blade:1 or \\0x102:0\\0x103:1 [CREATE/SET/DELETE/GET]\n \
                      <Service ID >     : [HEX or DEC]    - Service Id of the object.[CREATE/SET/DELETE/GET]\n \
                      <Attribute Path > : [HEX or STRING] - Ex. \\AttrA:0\\AttrB:0 or \\0x1034:0\\0x201:1. It can be null. [SET/GET]\n \
                      <Attribute ID >   : [HEX or DEC]    - Ex. 0x300. Attribute to be get.[SET/GET]\n \
                      <Index>           : [DEC]           - Ex. Index of the attribute. -1 for simple attribute. [SET/GET] \n"
	                  , retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }
    else
    {

        bundleHandle = corAtoI(argv[1]);

        rc = clCntNodeFind(corDbgBundleCnt, (ClCntKeyHandleT)&bundleHandle, &nodeH);
        if(CL_OK != rc)
        {
            clCorCliStrPrint(" Bundle handle is invalid ", retStr); 
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
        }

        rc = clCntNodeUserDataGet(corDbgBundleCnt, nodeH, (ClCntDataHandleT *)&pCorGetBundle);
        if(CL_OK != rc)
        {
            clCorCliStrPrint(" Bundle data not found. ", retStr); 
            return CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);
        }

        index = corAtoI(argv[6]);

        clBufferCreate (&inMsgHandle);
    
        if( (!strcasecmp(argv[4],"null")) || ( !strcmp (argv[4],"0")) )
            rc = _clCorBundleJobEnqueue(bundleHandle, argv[2], corAtoI(argv[3]), NULL, corAtoI(argv[5]), index,
                          &inMsgHandle, pCorGetBundle, CL_COR_OP_GET);
        else
            rc = _clCorBundleJobEnqueue(bundleHandle, argv[2], corAtoI(argv[3]), argv[4], corAtoI(argv[5]), index,
                         &inMsgHandle, pCorGetBundle, CL_COR_OP_GET);

        clBufferLengthGet(inMsgHandle, &size);

        if(size > 0)
        {
            pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);
            if(pTempBuff == NULL)
            {
                clCorCliStrPrint("Memory Allocation Failed", retStr);
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }
            memset(pTempBuff, '\0', size + 1);
            clBufferNBytesRead(inMsgHandle, (ClUint8T*)pTempBuff, &size);
            clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
            clHeapFree(pTempBuff);
            rc = CL_OK;
        }

        clBufferDelete(&inMsgHandle);
    }            
    return rc;
}


/** 
 *  This function would be used to start the bundle and get the attribute values.
 */
ClRcT
cliCorBundleApply(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                   rc              = CL_OK;
    ClCorBundleHandleT      bundleHandle    = 0;
    ClUint32T               size            = 0;
    ClUint8T                *pTempBuff      = NULL;
    ClCorDbgGetBundlePtrT  pCorGetBundle  = NULL;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec=0};

    if(argc != 2)
    {
        clCorCliStrPrint("\n Usage: corBundleApply <BundleHandle> \n \
                <bundelHandle> [DEC] : Handle to the bundle obtained after running clCorBundleInitialize. \n",
                retStr );
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    bundleHandle = corAtoI(argv[1]);
     
    rc = clCntDataForKeyGet(corDbgBundleCnt, (ClCntKeyHandleT)&bundleHandle, (ClCntDataHandleT *)&pCorGetBundle);
    if(CL_OK != rc)
    {
       clCorCliStrPrint("\nBundle handle passed is invalid.\n ", retStr); 
        return rc;
    }

    if(NULL == pCorGetBundle)
    {
        clCorCliStrPrint("\n Bundle Data is NULL. \n", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /*Acquiring the lock which will be unlocked by the calling function */
    clOsalMutexLock(pCorGetBundle->corGetMutexId);

    rc = clCorBundleApplyAsync(bundleHandle, 
                    clCorDbgBundleCallback, pCorGetBundle);
    if(rc != CL_OK)
    {
        gCorCliStr[0] = '\0';
        sprintf (gCorCliStr, "\n Failed while commiting the bundle. rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        goto freeResource;
    }

    clOsalCondWait(pCorGetBundle->corGetCondId, pCorGetBundle->corGetMutexId, delay);

freeResource:

    rc = clBufferLengthGet(pCorGetBundle->msgH, &size);
    if(size>0)
    {
        pTempBuff = (ClUint8T *) clHeapAllocate(size + 1);
        if(pTempBuff == NULL)
        {
            clCorCliStrPrint("Memory Allocation Failed", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        memset(pTempBuff, '\0', size + 1);
        clBufferNBytesRead(pCorGetBundle->msgH, (ClUint8T*)pTempBuff, &size);
        clCorCliStrPrint((ClCharT*)pTempBuff, retStr);
        clHeapFree(pTempBuff);
        rc = CL_OK;
    }

    clBufferClear(pCorGetBundle->msgH);
    
    clOsalMutexUnlock(pCorGetBundle->corGetMutexId);

    return rc;
}

/** 
 *  This function would be used to start the bundle and get the attribute values.
 */
ClRcT
cliCorBundleFinalize(ClUint32T argc, ClCharT **argv, ClCharT** retStr)
{
    ClRcT                   rc            = CL_OK;
    ClCorBundleHandleT      bundleHandle  = 0;
    ClCntNodeHandleT        nodeH         = 0;
    ClCorDbgGetBundlePtrT   pCorGetBundle = NULL;


    if(argc != 2)
    {
        clCorCliStrPrint("\n Usage: corBundleFinalize <BundleHandle> \n \
                <bundelHandle> [DEC] : Handle to the bundle obtained after running clCorBundleInitialize. \n",
                retStr );
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    bundleHandle = corAtoI(argv[1]);
 
    rc = clCntNodeFind(corDbgBundleCnt, (ClCntKeyHandleT)&bundleHandle, &nodeH ); 
    if(CL_OK != rc)
    {
        clCorCliStrPrint("\nBundle handle passed is invalid.\n ", retStr); 
        return rc;
    }

    if(nodeH)
    {
        rc = clCntNodeUserDataGet(corDbgBundleCnt, nodeH, (ClCntDataHandleT *)&pCorGetBundle);
        if(CL_OK != rc)
        {
           clCorCliStrPrint("\nBundle data not found.\n ", retStr); 
            return rc;
        }

        if(NULL == pCorGetBundle)
        {
            clCorCliStrPrint("\n Bundle Data is NULL. \n", retStr);
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }    

        clOsalMutexDelete(pCorGetBundle->corGetMutexId);
        clOsalCondDelete(pCorGetBundle->corGetCondId);
        clBufferDelete(&(pCorGetBundle->msgH));

        rc = clCorBundleFinalize(bundleHandle);
        if( CL_OK != rc)
        {   
            gCorCliStr[0] = '\0';
            sprintf(gCorCliStr, "\nFailed to finalize the bundle. rc[0x%x]\n", rc);
            clCorCliStrPrint(gCorCliStr, retStr );
        }

        clCntDelete(pCorGetBundle->cntHandle);    

        clCntNodeDelete(corDbgBundleCnt, nodeH);  
    }
    else
    {
        clCorCliStrPrint("\nBundle handle passed is invalid.\n ", retStr); 
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    return rc;
}


/**
 * Function which imlements the listing of COR classes
 * in the format which is comprehensible to the user.
 */

ClRcT 
cliCorListClasses(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT       rc = CL_OK;
    ClBufferHandleT   bufH = 0;
    ClCharT     tempStr[CL_COR_CLI_STR_LEN] = {0};
    ClUint32T   bufSize = 0;
    ClCharT     *pTempBuf = NULL;

    if (retStr == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to the retStr is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (argc > 1)
    {
        clCorCliStrPrint("Usage listClasses: \n "
                         "Shows all the cor MO classes defined while modeling", retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    memset(gCorCliStr, 0, sizeof(gCorCliStr));

    rc = clBufferCreate(&bufH);
    if (CL_OK != rc)
    {
        clLogError("CLI", "LCL", "Failed while creating the buffer handle. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Failed while creating the buffer handle. rc[0x%x]", rc);
        clCorCliStrPrint(gCorCliStr, retStr);
        return rc;
    }


    sprintf(tempStr, "ClassId \t\t\t ClassName \n");
    clBufferNBytesWrite(bufH, (ClUint8T *)tempStr, strlen(tempStr));

    sprintf(tempStr, "---------------------------------------------------------------------- \n");
    rc = clBufferNBytesWrite(bufH, (ClUint8T *)tempStr, strlen(tempStr));
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed while writing the information into buffer. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Execution failed. [0x%x]", rc);
        goto handleError;
    }

    rc = _clCorClassShow(bufH);
    if (CL_OK != rc)
    {
        clLogError("CLI", "LCL", "Failed to while doing the class walk. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Execution failed. [0x%x]", rc);
        goto handleError;
    }

    rc = clBufferLengthGet(bufH, &bufSize);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed while getting the size of the buffer. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Execution failed. [0x%x]", rc);
        goto handleError; 
    }

    pTempBuf = clHeapAllocate(bufSize + 1);
    if (NULL == pTempBuf)
    {
        clLogError("CLI", "CLS", 
                "Failed while allocating the memory for copying the output");
        sprintf(gCorCliStr, "Failed while allocating the memory for copying the output");
        rc = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        goto handleError;
    }

    memset(pTempBuf, 0 , bufSize + 1);

    rc = clBufferNBytesRead(bufH, (ClUint8T*)pTempBuf, &bufSize);
    if (CL_OK != rc)
  {
        clLogError("CLI", "CLS", 
                "Failed while reading the information from the buffer. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Failed while reading from buffer. rc[0x%x]", rc);
        clHeapFree(pTempBuf);
        goto handleError;
    }

    clCorCliStrPrint((ClCharT*)pTempBuf, retStr);
    clHeapFree(pTempBuf);
    clBufferDelete(&bufH);

    return rc;

handleError:
    clCorCliStrPrint(gCorCliStr, retStr);
    clBufferDelete(&bufH);
    return rc;
}

static ClRcT
corSlotGet(SaNameT *nodeName, ClIocNodeAddressT *nodeAddr)
{
    ClCpmSlotInfoT slotInfo = {0};
    ClRcT rc = CL_OK;
    saNameCopy(&slotInfo.nodeName, nodeName);
    rc = clCpmSlotGet(CL_CPM_NODENAME, &slotInfo);
    if(rc == CL_OK)
    {
        *nodeAddr = slotInfo.nodeIocAddress;
    }
    return rc;
}

ClRcT 
cliCorPrimaryOIClear(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT nodeAddr = 0;
    ClIocAddressT compAddr = {{0}};
    ClCorMOIdT  moId = {{{0}}};
    SaNameT compName = {0};
    SaNameT nodeName = {0};
    ClInt32T svcId = CL_COR_INVALID_SVC_ID;

    if (retStr == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to the retStr is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (argc != 4 && argc != 5)
    {
        clCorCliStrPrint("Usage : corPrimaryOIClear <MoId> <Mso Service Id> <AppName> <NodeName>\n \
            <MoId>          : [HEX or STRING]   -   Eg. \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n \
            <Mso service>   : [STRING]          -   Service Id of the MSO object. \n \
                                                       \"fault\" : Fault MSO.\n \
                                                       \"alarm\" : Alarm MSO.\n \
                                                       \"prov\"  : Provisioning MSO.\n \
                                                       \"pm\"    : Performance Management MSO.\n \
                                                       \"amf\"   : AMF Management MSO.\n \
                                                       \"chm\"   : Chassis Management MSO.\n \
            <AppName>       : [STRING]          -   Name of the application \n \
            <NodeName>      : [STRING]          -   Node name of the application \n", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if (argc == 5)
    {
        saNameSet(&nodeName, argv[4]);
        if((rc = corSlotGet(&nodeName, &nodeAddr)) != CL_OK)
        {
            clCorCliStrPrint("Invalid node name specified", retStr);
            return rc;
        }
    }
    else
    {
        nodeAddr = clIocLocalAddressGet();
    }

    rc = corXlateMOPath(argv[1], &moId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Invalid MoId specified.", retStr);
        return rc; 
    }

    svcId = clCorServiceIdStringToValueGet(argv[2]);
    if (svcId == CL_COR_INVALID_SVC_ID)
    {
        clCorCliStrPrint("Invalid service Id specified.", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clCorMoIdServiceSet(&moId, svcId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to set the service-id for the MoId.", retStr);
        return rc; 
    }

    saNameSet(&compName, argv[3]);

    rc = clCpmComponentAddressGet(nodeAddr, &compName, &compAddr);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to get the component address.", retStr);
        return rc; 
    }

    rc = clCorPrimaryOIClear(&moId, &(compAddr.iocPhyAddress));
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to unset the primary OI in COR.", retStr);
        return rc; 
    }

    clCorCliStrPrint("Successfully unset the primary OI.", retStr);

    return rc;
}

ClRcT 
cliCorPrimaryOISet(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT nodeAddr = 0;
    ClIocAddressT compAddr = {{0}};
    ClCorMOIdT  moId = {{{0}}};
    SaNameT compName = {0};
    SaNameT nodeName = {0};
    ClInt32T svcId = CL_COR_INVALID_SVC_ID;

    if (retStr == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to the retStr is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (argc != 4 && argc != 5)
    {
        clCorCliStrPrint("Usage : corPrimaryOISet <MoId> <Mso Service Id> <AppName> <NodeName>\n \
            <MoId>          : [HEX or STRING]   -   Eg. \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n \
            <Mso service>   : [STRING]          -   Service Id of the MSO object. \n \
                                                       \"fault\" : Fault MSO.\n \
                                                       \"alarm\" : Alarm MSO.\n \
                                                       \"prov\"  : Provisioning MSO.\n \
                                                       \"pm\"    : Performance Management MSO.\n \
                                                       \"amf\"   : AMF Management MSO.\n \
                                                       \"chm\"   : Chassis Management MSO.\n \
            <AppName>       : [STRING]          -   Name of the application \n \
            <NodeName>      : [STRING]          -   Node name of the application \n", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if (argc == 5)
    {
        saNameSet(&nodeName, argv[4]);
        if((rc = corSlotGet(&nodeName, &nodeAddr)) != CL_OK)
        {
            clCorCliStrPrint("Invalid node name specified", retStr);
            return rc;
        }
    }
    else
    {
        nodeAddr = clIocLocalAddressGet();
    }

    rc = corXlateMOPath(argv[1], &moId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Invalid MoId specified.", retStr);
        return rc; 
    }

    svcId = clCorServiceIdStringToValueGet(argv[2]);
    if (svcId == CL_COR_INVALID_SVC_ID)
    {
        clCorCliStrPrint("Invalid service Id specified.", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clCorMoIdServiceSet(&moId, svcId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to set the service-id for the MoId.", retStr);
        return rc; 
    }

    saNameSet(&compName, argv[3]);

    rc = clCpmComponentAddressGet(nodeAddr, &compName, &compAddr);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to get the component address.", retStr);
        return rc; 
    }

    rc = clCorPrimaryOISet(&moId, &(compAddr.iocPhyAddress));
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to register the primary oi in COR.", retStr);
        return rc; 
    }

    clCorCliStrPrint("Successfully registered the primary OI.", retStr);

    return rc;
}

ClRcT 
cliCorOIRegister(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT nodeAddr = 0;
    ClIocAddressT compAddr = {{0}};
    ClCorMOIdT  moId = {{{0}}};
    SaNameT compName = {0};
    SaNameT nodeName = {0};
    ClInt32T svcId = CL_COR_INVALID_SVC_ID;

    if (retStr == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to the retStr is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (argc != 4 && argc != 5)
    {
        clCorCliStrPrint("Usage : corOIRegister <MoId> <Mso Service Id> <AppName> <NodeName>\n \
            <MoId>          : [HEX or STRING]   -   Eg. \\Chassis:0\\Blade:1 mgmtApp or \\0x102:0\\0x103:1 mgmtApp \n \
            <Mso service>   : [STRING]          -   Service Id of the MSO object. \n \
                                                       \"fault\" : Fault MSO.\n \
                                                       \"alarm\" : Alarm MSO.\n \
                                                       \"prov\"  : Provisioning MSO.\n \
                                                       \"pm\"    : Performance Management MSO.\n \
                                                       \"amf\"   : AMF Management MSO.\n \
                                                       \"chm\"   : Chassis Management MSO.\n \
            <AppName>       : [STRING]          -   Name of the application \n \
            <NodeName>      : [STRING]          -   Node name of the application \n", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if (argc == 5)
    {
        saNameSet(&nodeName, argv[4]);
        if((rc = corSlotGet(&nodeName, &nodeAddr)) != CL_OK)
        {
            clCorCliStrPrint("Invalid node name specified", retStr);
            return rc;
        }
    }
    else
    {
        nodeAddr = clIocLocalAddressGet();
    }

    rc = corXlateMOPath(argv[1], &moId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Invalid MoId specified.", retStr);
        return rc; 
    }

    svcId = clCorServiceIdStringToValueGet(argv[2]);
    if (svcId == CL_COR_INVALID_SVC_ID)
    {
        clCorCliStrPrint("Invalid service Id specified.", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clCorMoIdServiceSet(&moId, svcId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to set the service-id for the MoId.", retStr);
        return rc; 
    }

    saNameSet(&compName, argv[3]);

    rc = clCpmComponentAddressGet(nodeAddr, &compName, &compAddr);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to get the component address.", retStr);
        return rc; 
    }

    rc = clCorOIRegister(&moId, &(compAddr.iocPhyAddress));
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to register the oi in COR.", retStr);
        return rc; 
    }

    clCorCliStrPrint("Successfully registered the OI.", retStr);

    return rc;
}

ClRcT 
cliCorOIUnRegister(ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT rc = CL_OK;
    ClIocNodeAddressT nodeAddr = 0;
    ClIocAddressT compAddr = {{0}};
    ClCorMOIdT  moId = {{{0}}};
    SaNameT compName = {0};
    SaNameT nodeName = {0};
    ClInt32T svcId = CL_COR_INVALID_SVC_ID;

    if (retStr == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to the retStr is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (argc != 4 && argc != 5)
    {
        clCorCliStrPrint("Usage : corOIUnRegister <MoId> <Mso Service Id> <AppName> <NodeName>\n \
            <MoId>          : [HEX or STRING]   -   Eg. \\Chassis:0\\Blade:1 or \\0x102:0\\0x103:1 \n \
            <Mso service>   : [STRING]          -   Service Id of the MSO object. \n \
                                                       \"fault\" : Fault MSO.\n \
                                                       \"alarm\" : Alarm MSO.\n \
                                                       \"prov\"  : Provisioning MSO.\n \
                                                       \"pm\"    : Performance Management MSO.\n \
                                                       \"amf\"   : AMF Management MSO.\n \
                                                       \"chm\"   : Chassis Management MSO.\n \
            <AppName>       : [STRING]          -   Name of the application \n \
            <NodeName>      : [STRING]          -   Node name of the application \n", retStr);

        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    if (argc == 5)
    {
        saNameSet(&nodeName, argv[4]);
        if((rc = corSlotGet(&nodeName, &nodeAddr)) != CL_OK)
        {
            clCorCliStrPrint("Invalid node name specified", retStr);
            return rc;
        }
    }
    else
    {
        nodeAddr = clIocLocalAddressGet();
    }

    rc = corXlateMOPath(argv[1], &moId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Invalid MoId specified.", retStr);
        return rc; 
    }

    svcId = clCorServiceIdStringToValueGet(argv[2]);
    if (svcId == CL_COR_INVALID_SVC_ID)
    {
        clCorCliStrPrint("Invalid service Id specified.", retStr);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    rc = clCorMoIdServiceSet(&moId, svcId);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to set the service-id for the MoId.", retStr);
        return rc; 
    }

    saNameSet(&compName, argv[3]);

    rc = clCpmComponentAddressGet(nodeAddr, &compName, &compAddr);
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to get the component address.", retStr);
        return rc; 
    }

    rc = clCorOIUnregister(&moId, &(compAddr.iocPhyAddress));
    if (rc != CL_OK)
    {
        clCorCliStrPrint("Failed to unregister the oi in COR.", retStr);
        return rc; 
    }

    clCorCliStrPrint("Successfully unregistered the OI.", retStr);

    return rc;
}

/**
 * Function to implement the debug cli which 
 * will give the information about a class given
 * a class Id.
 */
ClRcT 
cliCorClassShow (ClUint32T argc, ClCharT **argv, ClCharT **retStr)
{
    ClRcT   rc = CL_OK;
#if 0

    if (argc < 3 || argc > 5)
    {
showusage:
        cliCorClassShow("Usage: getClassInfo -c <classId> or \n" 
                        "       getClassInfo -p <MOID path> -s <serviceId> \n"
                        "       classId   : Class Identifier in hexadecimal format eg. 0x10001. \n"
                        "       MOID path : Path of the Class in the Object tree \n"
                        "       Service Id: Serivce Id of the MSO: -1 - for MO Class, 3 - MO's provisioning Class \n", 
                        retStr);
        return CL_COR_SET_RC(CL_COR_CLI_ERR_INVALID_USAGE);
    }

    rc = clBufferCreate(&bufH);
    if (CL_OK != rc)
    {
        clLogError("CLI", "GCI", "Failed while creating the buffer. rc[0x%x]", rc);
        sprintf(gCorCliStr, "Execution failed: [0x%x]", rc);
        cliCorClassShow(gCorCliStr, retStr);
        return rc;
    }

    if ( strcmp(argv[1], "-c") == 0 )
    {
        if (argc != 3)
        {
            clLogError("CLI", "GCI", 
                    "The number of arguments passed for the class show is not proper");
            clBufferDelete(&bufH);
            goto showUsage;
        }

        classId = strtol (argv[2], NULL, 16);

        rc = _clCorGetClassInfoFromClassId(classId, bufH);
    }
    else (strcmp(argv[1], "-p") == 0)
    {
        if (argc != 5)
        {
            clLogError("CLI", "GCI", "The number of arguments passed to get the class information given"
                    " the MO Path are not proper.");
            goto showUsage;
        }

        rc = corXlateMOPath(argv[2], &moId);

        svcId = strtol(argv[4], NULL, 0);

        clCorMoIdServiceSet(&moId, svcId);

        rc = _corMOPathToClassIdGet(&moId, svcId, &classId);
        if (CL_OK != rc)
        {
        }

        rc = _clCorGetClassInfoFromClassId(classId, bufH);
    }
#endif
    return rc;
}
