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
 * File        : clCorUtilityServer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * COR utility function implementation in server
 *****************************************************************************/

#include <string.h>
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clBitApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorPvt.h>
#include <clCorModule.h>
#include <clXdrApi.h>
#include <clOampRtApi.h>

/* Internal Headers*/
#include "clCorClient.h"
#include "clCorTreeDefs.h"
//#include "clCorOHLib.h"
#include "clCorNiIpi.h"
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorIpi.h"
#include "clCorEO.h"
#include "clCorObj.h"
#include "clCorDeltaSave.h"
#include "clCorRMDWrap.h"

#include <xdrClCorServiceIdT.h>
#include <xdrClCorTypeT.h>
#include <xdrClCorMOIdT.h>
#include <xdrClCorMoIdOpT.h>
#include <xdrClCorAlarmConfiguredDataIDLT.h>
#include <xdrClIocPhysicalAddressT.h>
#include <xdrClCorMsoConfigOpIDLT.h>
#include <xdrClCorMsoConfigIDLT.h>

/* MACRO */
#define CL_COR_META_DATA_FILE "clCorMOClass.xml"
#define CL_COR_CONFIG_FILE "clCorConfig.xml"


/* GLOBALS */
extern ClRcT  corXlateMOPath(char *path, ClCorMOIdPtrT cAddr);
extern ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh,  SaNameT *moIdName);
extern ClCorInitStageT    gCorInitStage;
extern ClOsalMutexIdT gCorRouteInfoMutex;
extern _ClCorServerMutexT gCorMutexes;
extern ClUint32T gCorSlaveSyncUpDone;
extern ClCorSyncStateT pCorSyncState;
extern ClUint32T gCorTxnRequestCount;
#ifdef CL_COR_MEASURE_DELAYS
extern _ClCorOpDelayT gCorOpDelay;
#endif

typedef struct ClCorClassConfig
{
    ClOmClassTypeT omClassIdBase;
    ClCorClassTypeT corClassIdBase;
    ClCorOmClassDataT *pOmClassData;
    ClUint32T numOmClasses;
    ClCorMsoOmTypeT *pMsoDefns;
    ClUint32T numMsoDefns;
    ClBoolT *pBuiltinModuleMap;
    ClUint32T maxBuiltinModules;
    ClOsalMutexT mutex;
} ClCorClassConfigT;

/* STATIC */
static ClCorClassConfigT gClCorClassConfig;
static ClUint64T corAtoI(ClCharT* str);
static ClRcT _corAlarmMsoValueSet(
        DMContObjHandle_t dmContObjHdl, 
        VDECL_VER(ClCorAlarmProfileDataIDLT, 4, 1, 0)* pAlarmProfile);
ClRcT _corAlarmMsoValueSwap(VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData);
void _clCorAlarmMsoConfigureCB(void* pMoId, ClBufferHandleT buffer);
ClRcT _clCorAlarmMsoProfilesConfig(ClCorMOIdPtrT pMoId, DMContObjHandle_t dmContObjHdl, void* pData);
ClRcT _clCorConfigureMso(VDECL_VER(ClCorMsoConfigIDLT, 4, 1, 0)* pMsoConfig);
static void _corMsoConfigReplyCB(ClRcT retCode,
        ClPtrT pCookie,
        ClBufferHandleT inMsgHdl,
        ClBufferHandleT outMsgHdl);

static ClInt32T msoClassDefnCompare(const void *data, const void *element)
{
    const ClCorMsoOmTypePtrT key1 = (const ClCorMsoOmTypePtrT)data;
    const ClCorMsoOmTypePtrT key2 = (const ClCorMsoOmTypePtrT)element;
    ClInt32T cmp = 0;
    if(!(cmp = (key1->moClassId - key2->moClassId)))
    {
        return key1->svcId - key2->svcId;
    }
    return cmp;
}

static ClRcT _clCorOmClassNameFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT srvcId, 
                                               ClOmClassTypeT *pOmClass, ClCharT *pOmClassName, 
                                               ClUint32T maxClassSize)
{
    ClCorMsoOmTypePtrT pCorMsoOmType = NULL;
    ClCorMsoOmTypeT data = {.moClassId = moClass, .svcId = srvcId };

    CL_FUNC_ENTER();

    pCorMsoOmType = (ClCorMsoOmTypePtrT)bsearch(&data, gClCorClassConfig.pMsoDefns,
                                                gClCorClassConfig.numMsoDefns,
                                                (ClUint32T)sizeof(*gClCorClassConfig.pMsoDefns),
                                                msoClassDefnCompare);
    if(!pCorMsoOmType)
        return CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);

    if(pOmClass)
        *pOmClass = pCorMsoOmType->omClassId;

    if(pOmClassName && maxClassSize)
        return clCorOmClassNameFromIdGet(pCorMsoOmType->omClassId, pOmClassName, maxClassSize);

    return CL_OK;
}

ClRcT _clCorOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT srvcId, ClOmClassTypeT *pOmClass)
{
    ClCorMsoOmTypePtrT pCorMsoOmType = NULL;
    ClCorMsoOmTypeT data = { .moClassId = moClass, .svcId = srvcId };

    pCorMsoOmType = (ClCorMsoOmTypePtrT)bsearch(&data, gClCorClassConfig.pMsoDefns, gClCorClassConfig.numMsoDefns,
                                                (ClUint32T)sizeof(*gClCorClassConfig.pMsoDefns),
                                                msoClassDefnCompare);
    if(!pCorMsoOmType)
        return CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);

    if(pOmClass)
        *pOmClass = pCorMsoOmType->omClassId;

    return CL_OK;
}

ClRcT VDECL(_CORUtilOps) (ClUint32T cData, ClBufferHandleT  inMsgHandle,
                        ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classType = 0;
    ClCorServiceIdT svcId = CL_COR_INVALID_SVC_ID;
    ClOmClassTypeT OmClass = 0;
	ClVersionT		version = {0};
    
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("UTL", "ATB", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

	if((rc = clXdrUnmarshallClVersionT(inMsgHandle, &version)) != CL_OK)
		return rc;

    clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);


    if((clXdrUnmarshallClInt32T(inMsgHandle, (ClUint8T*)&classType)) != CL_OK)
    	return rc;


    if((VDECL_VER(clXdrUnmarshallClCorServiceIdT, 4, 0, 0)(inMsgHandle, (ClUint8T*)&svcId )) != CL_OK)
    	return rc;

   rc =  _clCorOmClassFromInfoModelGet(classType, svcId, &OmClass);

   if(CL_OK != rc)
    {
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCould not get OM class. Rc  = 0x%x", rc));
    return rc;
    }

  rc =  clXdrMarshallClInt32T(&OmClass, outMsgHandle, 0 );

  return rc;

}

ClRcT VDECL(_CORUtilExtendedOps) (ClUint32T cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classType;
    ClCorServiceIdT svcId;
    ClOmClassTypeT omClass = 0;
	ClVersionT		version;
    ClCharT omClassName[CL_MAX_NAME_LENGTH] = {0};
    ClCorUtilExtendedOpT opCode = 0;

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("UTL", "ATB", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

	if((rc = clXdrUnmarshallClVersionT(inMsgHandle, &version)) != CL_OK)
		return rc;

    clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);

    if((rc = clXdrUnmarshallClInt32T(inMsgHandle, &opCode)) != CL_OK)
        return rc;

    if(opCode >= COR_UTIL_MAX_OP)
        return rc;
    
    switch(opCode)
    {
    case COR_MO_TO_OM_CLASS_UTIL_OP:
        {
            SaNameT className = {0};

            if((rc = clXdrUnmarshallClInt32T(inMsgHandle, (ClUint8T*)&classType)) != CL_OK)
                return rc;

            if((VDECL_VER(clXdrUnmarshallClCorServiceIdT, 4, 0, 0)(inMsgHandle, (ClUint8T*)&svcId )) != CL_OK)
                return rc;

            rc =  _clCorOmClassNameFromInfoModelGet(classType, svcId, &omClass, omClassName,
                                                    (ClUint32T)sizeof(omClassName)-1);

            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCould not get OM class. Rc  = 0x%x", rc));
                return rc;
            }

            saNameSet(&className, omClassName);
            rc =  clXdrMarshallClInt32T(&omClass, outMsgHandle, 0 );
            rc |= clXdrMarshallSaNameT(&className, outMsgHandle, 0);
        }
        break;

    case COR_CONFIG_LOAD_UTIL_OP:
        {
            SaNameT configFile = {0};
            SaNameT routeFile =  {0};
            if((rc = clXdrUnmarshallSaNameT(inMsgHandle, &configFile)) != CL_OK)
                return rc;
            if((rc = clXdrUnmarshallSaNameT(inMsgHandle, &routeFile)) != CL_OK)
                return rc;

            configFile.value[CL_MIN((ClUint32T)sizeof(configFile.value),
                                    configFile.length)] = 0;
            routeFile.value[CL_MIN((ClUint32T)sizeof(routeFile.value),
                                   routeFile.length)] = 0;
            clLogNotice("CONFIG", "RELOAD", "Loading COR configuration from file [%s], route from file [%s]",
                        (ClCharT*)configFile.value, (ClCharT*)routeFile.value);
            if( (rc = clCorInformationModelLoad((const ClCharT*)configFile.value,
                                                (const ClCharT*)routeFile.value)) != CL_OK)
            {
                clLogError("CONFIG", "RELOAD", "COR configuration load returned [%#x]", rc);
            }
            
        }
        break;

    default:
        rc = CL_COR_SET_RC(CL_ERR_NOT_SUPPORTED);
        break;
    }

    return rc;
}

ClRcT VDECL(_corMOIdOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                        ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCorMoIdOpT op = 0;
	ClVersionT version = {0};

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("MID", "ATB", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    if((rc = clXdrUnmarshallClVersionT(inMsgHandle, (ClUint8T*)&version)) != CL_OK)
    	return (rc);
    /* Client to Server version Check */ 
    clCorClientToServerVersionValidate(version, rc); 
    if(rc != CL_OK)
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
   
    if((rc = VDECL_VER(clXdrUnmarshallClCorMoIdOpT, 4, 0, 0)(inMsgHandle, (ClUint8T*)&op)) != CL_OK)
    	return (rc);
   switch(op)
   {
       case CL_COR_MOID_TO_CLASS_GET:
       {
           ClCorMOClassPathT moClsPath;
           ClCorMOIdT        moId;
           ClCorClassTypeT   classId;

           if((rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(inMsgHandle,&moId)) != CL_OK)
          		 return (rc);
           clCorMoIdToMoClassPathGet(&moId, &moClsPath);
           if ((rc = corMOTreeClassGet (&moClsPath, moId.svcId, &classId)) != CL_OK)
                        {
                                CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Can not get the classId \
                                   for the supplied moId/svcId combination\n"));
                                return (rc);
                        }
           rc =  clXdrMarshallClInt32T(&classId, outMsgHandle, 0);
       }
       break;

       case CL_COR_NAME_TO_MOID_GET:
       {
           SaNameT  moIdName = {0};
           ClCorMOIdT        moId = {{{0}}};

           if((rc = clXdrUnmarshallSaNameT(inMsgHandle, (ClUint8T*)&moIdName)) != CL_OK)
            return (rc);

           clCorMoIdInitialize(&moId);
    
           rc = corXlateMOPath(moIdName.value, &moId);
           if(CL_OK != rc)
           {
                clLog(CL_LOG_SEV_ERROR,"MUT", "MDG","Failed while getting the \
                       Moid name [%s] to MoId. rc[0x%x]", moIdName.value, rc);
                return (rc);
           }
           /* rc =  clBufferNBytesWrite(outMsgHandle, (ClUint8T *)&moId, sizeof(ClCorMOIdT)); */
           rc =  VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)((ClUint8T *)&moId, outMsgHandle, 0);
       }
       break;

       case CL_COR_MOID_TO_NAME_GET:
       {
           SaNameT  moIdName;
           ClCorMOIdT        moId;

           if((rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(inMsgHandle, (ClUint8T*)&moId)) != CL_OK)
            return (rc);
 
           memset(&moIdName, 0, sizeof(moIdName));
           if ((rc = _clCorMoIdToMoIdNameGet(&moId, &moIdName)) != CL_OK)
             return (rc);

           /* rc =  clBufferNBytesWrite(outMsgHandle, (ClUint8T *)&moIdName, sizeof(SaNameT)); */
           rc =  clXdrMarshallSaNameT(&moIdName, outMsgHandle, 0);
       }
       break;

       default:
          CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "Invalid Operation !!!\n"));
          return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
     }

  return (rc);
}




static ClUint64T corAtoI(ClCharT* str)
{
    ClUint64T val = 0;
    ClInt32T len = 0, hexlen = 0;

    len = strlen(str);
    
    if (len <= 2)
    {
        sscanf(str, "%llu", &val);
        return val;      
    }

    hexlen = strlen("0x");
    
    if (!strncmp(str, "0x", hexlen) || !strncmp(str, "0X", hexlen))
    {
        sscanf(str, "%llx", &val);
        return val;
    }
    else
    {   
        sscanf(str, "%llu", &val);
        return val;
    }
}

ClRcT clCorMoClassPathXlate(ClCharT* moClassPathName, ClCorMOClassPathPtrT moClassPath)
{
    ClRcT rc = CL_OK;
    ClCharT *buff = NULL, *tmpBuff =NULL, *tmpPath = NULL;
    ClCorClassTypeT classId = 0;

    CL_FUNC_ENTER();
    
    if (moClassPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory is not allocated for moClassPath"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (moClassPathName == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("moClassPathName is NULL"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    clCorMoClassPathInitialize(moClassPath);
    
    buff = (ClCharT *) clHeapAllocate(strlen(moClassPathName));
    if (buff == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset(buff, 0, strlen(moClassPathName));
    
    tmpBuff = buff;

    tmpPath = (ClCharT *) strtok_r(moClassPathName, "\\", &buff);

    while (tmpPath != NULL)
    {        
        rc = _corNiNameToKeyGet(tmpPath, &classId);
        if (rc != CL_OK)
        {
            /* Invalid class type specified */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the classId. Invalid class name specified. rc [0x%x]", rc));
            clHeapFree(tmpBuff); 
            CL_FUNC_EXIT();
            return rc;
        }
        
        rc = clCorMoClassPathAppend(moClassPath, classId);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to append the class name with the MO-Path. rc [0x%x]. classId %d className %s", rc, classId, tmpPath)); 
            clHeapFree(tmpBuff);
            CL_FUNC_EXIT();
            return rc;
        }
        
        tmpPath = strtok_r(NULL, "\\", &buff);
    }
    
    clHeapFree(tmpBuff);
    CL_FUNC_EXIT();    
    return CL_OK;
}

ClRcT clCorAttrDataTypeIdGet(ClCharT* dataTypeName, ClInt32T* dataTypeId)
{
    ClRcT rc = CL_OK;

    if (strcmp(dataTypeName, "CL_COR_INVALID_DATA_TYPE") == 0)
        *dataTypeId = CL_COR_INVALID_DATA_TYPE;
    else if (strcmp(dataTypeName, "CL_COR_VOID") == 0)
        *dataTypeId = CL_COR_VOID;
    else if (strcmp(dataTypeName, "CL_COR_INT8") == 0)
        *dataTypeId = CL_COR_INT8;
    else if (strcmp(dataTypeName, "CL_COR_UINT8") == 0)
        *dataTypeId = CL_COR_UINT8;
    else if (strcmp(dataTypeName, "CL_COR_INT16") == 0)
        *dataTypeId = CL_COR_INT16;
    else if (strcmp(dataTypeName, "CL_COR_UINT16") == 0)
        *dataTypeId = CL_COR_UINT16;
    else if (strcmp(dataTypeName, "CL_COR_INT32") == 0)
        *dataTypeId = CL_COR_INT32;
    else if (strcmp(dataTypeName, "CL_COR_UINT32") == 0)
        *dataTypeId = CL_COR_UINT32;
    else if (strcmp(dataTypeName, "CL_COR_INT64") == 0)
        *dataTypeId = CL_COR_INT64;
    else if (strcmp(dataTypeName, "CL_COR_UINT64") == 0)
        *dataTypeId = CL_COR_UINT64;
    else if (strcmp(dataTypeName, "CL_COR_FLOAT") == 0)
        *dataTypeId = CL_COR_FLOAT;
    else if (strcmp(dataTypeName, "CL_COR_DOUBLE") == 0)
        *dataTypeId = CL_COR_DOUBLE;
    else if (strcmp(dataTypeName, "CL_COR_COUNTER32") == 0)
        *dataTypeId = CL_COR_COUNTER32;
    else if (strcmp(dataTypeName, "CL_COR_COUNTER64") == 0)
        *dataTypeId = CL_COR_COUNTER64;
    else if (strcmp(dataTypeName, "CL_COR_SEQUENCE32") == 0)
        *dataTypeId = CL_COR_SEQUENCE32;
    else
        rc = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);

    return rc;
}




ClRcT clCorClassAttrFormUserFlag(
            ClInt32T *corClassAttrUserFlag, 
            ClCharT* corClassAttrType, 
            ClCharT* corClassAttrPersistency, 
            ClCharT* corClassAttrCaching, 
            ClCharT* corClassAttrInitialized,
            ClCharT* corClassAttrWritable)
{
    ClInt32T flag = 0;

    CL_FUNC_ENTER();

    if (strcmp(corClassAttrType, "CONFIG") == 0)
        flag = flag | CL_COR_ATTR_CONFIG;            
    else if (strcmp(corClassAttrType, "RUNTIME") == 0)
        flag = flag | CL_COR_ATTR_RUNTIME;        
    else if (strcmp(corClassAttrType, "OPERATIONAL") == 0)
        flag = flag | CL_COR_ATTR_OPERATIONAL;
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid configuration flag specified."));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
    }

    if (strcmp(corClassAttrPersistency, "TRUE") == 0)
        flag = flag | CL_COR_ATTR_PERSISTENT;

    if (strcmp(corClassAttrCaching, "TRUE") == 0)
        flag = flag | CL_COR_ATTR_CACHED;

    /* Set the Initialized flag */
    if (strcmp(corClassAttrInitialized, "TRUE") == 0)
        flag = flag | CL_COR_ATTR_INITIALIZED;

    /* Set the Writable flag */
    if (strcmp(corClassAttrWritable, "TRUE") == 0)
        flag = flag | CL_COR_ATTR_WRITABLE;

    *corClassAttrUserFlag = flag;
    
    CL_FUNC_EXIT();

    return (CL_OK);
}

ClRcT clCorParseSimpleAttributes(ClCorClassTypeT corClassId, 
                                 ClParserPtrT corClassSimpleAttrDef,
                                 ClListHeadT *pAttrList)
{
    ClRcT rc = CL_OK;
    ClParserPtrT corClassSimpleAttr = NULL;
    ClCharT* corClassAttrName = NULL;
    ClCorAttrIdT corClassAttrId = 0;
    ClInt32T corClassAttrDataTypeVal = 0;
    ClCharT* corClassAttrDataTypeName = NULL;
    ClInt64T corClassAttrInitVal = 0;
    ClInt64T corClassAttrMinVal = 0;
    ClInt64T corClassAttrMaxVal = 0;
    ClInt32T corClassAttrUserFlag = 0;
    ClCharT* corClassAttrType = NULL;
    ClCharT* corClassAttrPersistency = NULL;
    ClCharT* corClassAttrCaching = NULL;
    ClCharT* corClassAttrInitialized = NULL;
    ClCharT* corClassAttrWritable = NULL;
    ClCharT* pTemp = NULL;
    corAttrIntf_t *pAttr = NULL;
    CORClass_h classHdl = NULL;
    
    CL_FUNC_ENTER();

    if (corClassSimpleAttrDef == NULL)
    {
        /* No simple attributes present */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Simple attributes found"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
   

    corClassSimpleAttr = clParserChild(corClassSimpleAttrDef, "corClassSimpleAttrT");
    if (corClassSimpleAttr != NULL)
    {
        /* Create all the attributes */
        for (; corClassSimpleAttr != NULL; corClassSimpleAttr = corClassSimpleAttr->next)
        {
            /* Parse the attribute information */
            corClassAttrName = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrName");
            if (corClassAttrName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attribute name for class [0x%x].", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
           
            pTemp = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrId");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attribute Id for class [0x%x].", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            corClassAttrId = (ClCorAttrIdT) corAtoI(pTemp);

            corClassAttrDataTypeName = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrDataType");
            if (corClassAttrDataTypeName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attribute data type for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            pTemp = (ClCharT*) clParserAttr(corClassSimpleAttr, "attrInitVal");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attr default value for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrInitVal =  (ClInt64T) corAtoI(pTemp);
           
            pTemp = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrMinVal");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attr min value for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrMinVal = (ClInt64T) corAtoI(pTemp);
           
            pTemp = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrMaxVal");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attr max value for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrMaxVal = (ClInt64T) corAtoI(pTemp);
            
            corClassAttrType = ((ClCharT*) clParserAttr(corClassSimpleAttr, "attrType"));
            if (corClassAttrType == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attr type for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            /* Based on the attrType, skip or parse the below values */
            /* attrType==CONFIG, 
               persistency=true, 
               cached=true
               writable = true/false
               Initialized = true/false

               attrType = RUNTIME,
               cached=true/false,
               persistency = true/false,
               Initialized=false,
               Writable=false
            */
            if (strcmp(corClassAttrType, "CONFIG") == 0)
            {
                corClassAttrPersistency = (ClCharT*) "TRUE"; 
                corClassAttrCaching = (ClCharT *) "TRUE"; 
                
                corClassAttrInitialized = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrInitialized");
                if (corClassAttrInitialized == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrInitialized flag type for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                corClassAttrWritable = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrWritable");
                if (corClassAttrWritable == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrWritable flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
            }
            else if (strcmp(corClassAttrType, "RUNTIME") == 0)
            {
                corClassAttrCaching = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrCaching");
                if (corClassAttrCaching == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrCaching flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                corClassAttrPersistency = (ClCharT*) clParserAttr(corClassSimpleAttr, "attrPersistency");
                if (corClassAttrPersistency == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrPersistency flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                corClassAttrInitialized = (ClCharT *) "FALSE";
                corClassAttrWritable = (ClCharT *) "FALSE";

                if ((strcmp(corClassAttrCaching, "FALSE") == 0) && (strcmp(corClassAttrPersistency, "TRUE") == 0))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid flags specified for the attribute [0x%x].", corClassAttrId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_ATTR_FLAGS_INVALID));
                }
            }
            else if (strcmp(corClassAttrType, "OPERATIONAL") == 0)
            {
                corClassAttrCaching = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrCaching");
                if (corClassAttrCaching == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrCaching flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                corClassAttrPersistency = (ClCharT*) clParserAttr(corClassSimpleAttr, "attrPersistency");
                if (corClassAttrPersistency == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrPersistency flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                if( !strcmp(corClassAttrCaching, "TRUE") || !strcmp(corClassAttrPersistency, "TRUE"))
                {
                    clLogError("INT", "PRS", 
                               "Operational attribute [%#x] cannot be cached or persistent. Mark attrCaching and/or attrPersistency as FALSE for attr [%#x] in clCorMOClass.xml", 
                               corClassAttrId, corClassAttrId);
                    return (CL_COR_SET_RC(CL_COR_ERR_ATTR_FLAGS_INVALID));
                }

                corClassAttrInitialized = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrInitialized");
                if (corClassAttrInitialized == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrInitialized flag type for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                
                corClassAttrWritable = (ClCharT *) clParserAttr(corClassSimpleAttr, "attrWritable");
                if (corClassAttrWritable == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the attrWritable flag for attr Id [0x%x] in class [0x%x].", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }

                if( !strcmp(corClassAttrWritable, "FALSE") )
                {
                    clLogError("INT", "PRS", "Operational attribute [%#x] should be writable. Mark attrWritable as TRUE for attr [%#x] in clCorMOClass.xml", 
                               corClassAttrId, corClassAttrId);
                    return (CL_COR_SET_RC(CL_COR_ERR_ATTR_FLAGS_INVALID));
                }
            }
            else
            {
                clLogCritical("INT", "PRS", 
                              "Failure while parsing attrid [0x%x] with attr name [%s], \
                        The attribute type should be either CONFIG or RUNTIME or OPERATIONAL",
                              corClassAttrId, corClassAttrName);
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_STATE); 
            }

            clLogDebug("SER", "INT", "Successfully parsed attr Id [%#x] in class [%d].", corClassAttrId, corClassId);
            rc = clCorAttrDataTypeIdGet(corClassAttrDataTypeName, &corClassAttrDataTypeVal);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid data type specified. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }
           
            rc = clCorClassAttrFormUserFlag(&corClassAttrUserFlag, corClassAttrType, corClassAttrPersistency, corClassAttrCaching, corClassAttrInitialized, corClassAttrWritable);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to form the user flag for the given information. rc [0x%x]", rc))
                    CL_FUNC_EXIT();
                return rc;
            }
            
            pAttr = clHeapAllocate(sizeof(corAttrIntf_t)); /* Free this memory in all the places */
            if (pAttr == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory."));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }
            memset(pAttr, 0, sizeof(corAttrIntf_t));
               
            pAttr->classId = corClassId;
            pAttr->attrId = corClassAttrId;
            pAttr->attrType = corClassAttrDataTypeVal;
            pAttr->init = corClassAttrInitVal;
            pAttr->min = corClassAttrMinVal;
            pAttr->max = corClassAttrMaxVal;
            pAttr->flags = corClassAttrUserFlag;
            
            classHdl = dmClassGet(pAttr->classId);
            if (classHdl == NULL)
            {
                /* Unable to get the class handle */
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to get the DM handle for Class Id [0x%x].", pAttr->classId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
            }
            
            rc = dmClassAttrCreate(classHdl, pAttr);
            if ((rc != CL_OK))
            {
                /* Failed to create the class attribute */
                clHeapFree(pAttr);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                           CL_LOG_MESSAGE_1_ATTR_CREATE, rc);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the class attribute. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }

            rc = _corNIAttrAdd(corClassId, corClassAttrId, corClassAttrName);
            if ((rc != CL_OK))
            {
                /* Failed to set the attribute name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the name for the attribute. rc [0x%x]", rc));
                clHeapFree(pAttr);
                CL_FUNC_EXIT();
                return rc;
            }
            
            rc = _corClassAttrValueSet(classHdl, pAttr->attrId, pAttr->init, pAttr->min, pAttr->max);            
            if ((rc != CL_OK))
            {
                /* Failed to set the attribute value */
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the attribute value. rc [0x%x]", rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                           CL_LOG_MESSAGE_1_ATTR_VALUE_SET, rc);
                CL_FUNC_EXIT();
                return rc;
            }
            
            rc = _corClassAttrUserFlagsSet(classHdl, pAttr->attrId, pAttr->flags);
            if (rc != CL_OK)
            {
                /* Unable to set the user flags */                                
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the user flags for the attribute. rc [0x%x]", rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                           CL_LOG_MESSAGE_1_ATTR_FLAG_SET, rc);
                CL_FUNC_EXIT();
                return rc;
            }
            if(pAttrList)
            {
                clListAddTail(&pAttr->list, pAttrList);
            }
            else
            {
                clHeapFree(pAttr);
            }
        }
    }
   
    CL_FUNC_EXIT();
    return rc; 
}

ClRcT clCorParseArrayAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassArrAttrDef,
                                ClListHeadT *pAttrList)
{
    ClRcT rc = CL_OK;
    ClParserPtrT corClassArrAttr = NULL;
    ClCharT* corClassAttrName = NULL;
    ClCorAttrIdT corClassAttrId = 0;
    ClInt32T corClassAttrDataTypeVal = 0;
    ClInt32T corClassArrAttrSize = 0;
    ClInt32T corClassAttrUserFlag = 0;
    ClCharT* corClassAttrDataTypeName = NULL;
    ClCharT* corClassAttrType = NULL;
    ClCharT* corClassAttrPersistency = NULL;
    ClCharT* corClassAttrCaching = NULL;
    ClCharT* corClassAttrInitialized = NULL;
    ClCharT* corClassAttrWritable = NULL;
    ClCharT* pTemp = NULL;
    corAttrIntf_t *pAttr = NULL;
    CORClass_h classHdl = NULL;

    CL_FUNC_ENTER();

    if (corClassArrAttrDef == NULL)
    {
        /* No simple attributes present */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Array attributes found"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    corClassArrAttr = clParserChild(corClassArrAttrDef, "corClassArrAttrT");
                    
    if (corClassArrAttr != NULL)
    {
        /* Create all the attributes */
        for (; corClassArrAttr != NULL; corClassArrAttr = corClassArrAttr->next)
        {
            corClassAttrName = (ClCharT *) clParserAttr(corClassArrAttr, "attrName");
            if (corClassAttrName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attr name in class [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            pTemp = (ClCharT *) clParserAttr(corClassArrAttr, "attrId");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attr Id in class [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrId = (ClCorAttrIdT) corAtoI(pTemp);
            
            corClassAttrDataTypeName = (ClCharT *) clParserAttr(corClassArrAttr, "attrDataType");         
            if (corClassAttrDataTypeName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrDataType for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            pTemp =  (ClCharT *) clParserAttr(corClassArrAttr, "attrArraySize");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrArraySize for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassArrAttrSize = (ClInt32T) corAtoI(pTemp);
            
            corClassAttrType = ((ClCharT*) clParserAttr(corClassArrAttr, "attrType"));
            if (corClassAttrType == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrType for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            /* Based on the attrType, skip or parse the below values */
            /* attrType==CONFIG, 
                persistency=true, 
                cached=true
                writable = true/false
                Initialized = true/false

               attrType = RUNTIME,
                cached=true/false,
                persistency = true/false,
                Initialized=false,
                Writable=false
            */
            
            if (strcmp(corClassAttrType, "CONFIG") == 0)
            {
                corClassAttrPersistency = (ClCharT*) "TRUE";
                corClassAttrCaching = (ClCharT *) "TRUE";
                
                corClassAttrInitialized = (ClCharT *) clParserAttr(corClassArrAttr, "attrInitialized");
                if (corClassAttrInitialized == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrInitialized flag for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                corClassAttrWritable = (ClCharT *) clParserAttr(corClassArrAttr, "attrWritable");
                if (corClassAttrWritable == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrWritable flag for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
            }
            else if (strcmp(corClassAttrType, "RUNTIME") == 0)
            {
                corClassAttrCaching = (ClCharT *) clParserAttr(corClassArrAttr, "attrCaching");
                if (corClassAttrCaching == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrCaching flag for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }

                corClassAttrPersistency = (ClCharT*) clParserAttr(corClassArrAttr, "attrPersistency");
                if (corClassAttrPersistency == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrPersistency flag for attrId [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }

                corClassAttrInitialized = (ClCharT *) "FALSE";
                corClassAttrWritable = (ClCharT *) "FALSE";

                if ((strcmp(corClassAttrCaching, "FALSE") == 0) && (strcmp(corClassAttrPersistency, "TRUE") == 0))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid flags specified for the attribute [0x%x].", corClassAttrId));
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            else
            {
                clLogCritical("INT", "PRS", 
                        "Failure while parsing attrid [0x%x] with attr name [%s], \
                        The attribute type should be either CONFIG or RUNTIME",
                        corClassAttrId, corClassAttrName);
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_STATE); 
            }

            rc = clCorClassAttrFormUserFlag(&corClassAttrUserFlag, corClassAttrType, corClassAttrPersistency, corClassAttrCaching, corClassAttrInitialized, corClassAttrWritable);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to form the user flag for the given information. rc [0x%x]", rc))
                CL_FUNC_EXIT();
                return rc;
            }

            rc = clCorAttrDataTypeIdGet(corClassAttrDataTypeName, &corClassAttrDataTypeVal);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid data type specified. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }
    
            pAttr = clHeapAllocate(sizeof(corAttrIntf_t)); /* Free this memory in all the places */
            if (pAttr == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory."));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }
            memset(pAttr, 0, sizeof(corAttrIntf_t));
           
            pAttr->classId    = corClassId;
            pAttr->attrId     = corClassAttrId;
            pAttr->attrType   = CL_COR_ARRAY_ATTR;
            pAttr->subAttrType= corClassAttrDataTypeVal;
            pAttr->min        = corClassArrAttrSize;
            pAttr->max        = -1;
            pAttr->flags      = corClassAttrUserFlag;

            classHdl = dmClassGet(pAttr->classId);
            if (classHdl == NULL)
            {
                /* Unable to get the class handle */
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the DM handle for Class Id [0x%x].", pAttr->classId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
            }
 
            rc = dmClassAttrCreate(classHdl, pAttr);
            if ((rc != CL_OK))
            {
                /* Failed to create the class attribute */
                clHeapFree(pAttr);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_CREATE, rc);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the class attribute. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }

            rc = _corNIAttrAdd(corClassId, corClassAttrId, corClassAttrName);
            if ((rc != CL_OK))
            {
                /* Failed to set the attribute name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the name for the attribute. rc [0x%x]", rc));
                clHeapFree(pAttr);
                CL_FUNC_EXIT();
                return rc;
            }

            rc = _corClassAttrUserFlagsSet(classHdl, pAttr->attrId, pAttr->flags);
            if (rc != CL_OK)
            {
                /* Unable to set the user flags */                                
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the user flags for the attribute. rc [0x%x]", rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_FLAG_SET, rc);
                CL_FUNC_EXIT();
                return rc;
            }
            if(pAttrList)
            {
                clListAddTail(&pAttr->list, pAttrList);
            }
            else
            {
                clHeapFree(pAttr);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clCorParseAssocAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassAssocAttrDef,
                                ClListHeadT *pAttrList)
{
    ClRcT rc = CL_OK;
    ClCharT* corClassAttrName = NULL;
    ClCorAttrIdT corClassAttrId = 0;
    ClParserPtrT corClassAssocAttr = NULL;
    ClCharT* corClassAttrAssocClassName = NULL;
    ClCharT* corClassAttrInitialized = NULL;
    ClCharT* corClassAttrWritable = NULL;
    ClCorClassTypeT corClassAttrAssocClassId = 0;
    ClInt32T corClassAttrMaxAssoc = 0;
    corAttrIntf_t *pAttr = NULL;
    CORClass_h classHdl = NULL;
    ClCharT* pTemp = NULL;

    if (corClassAssocAttrDef == NULL)
    {
        /* No association attributes present */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Association attributes found"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
   
   /* Assoc attributes present */
    corClassAssocAttr = clParserChild(corClassAssocAttrDef, "corClassAssocAttrT");
                    
    if (corClassAssocAttr != NULL)
    {
        /* Create all the attributes */
        for (; corClassAssocAttr != NULL; corClassAssocAttr = corClassAssocAttr->next)
        {
            corClassAttrName = (ClCharT *) clParserAttr(corClassAssocAttr, "attrName");
            if(corClassAttrName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrName for classId [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            pTemp = (ClCharT *) clParserAttr(corClassAssocAttr, "attrId");
            if(pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrId for classId [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrId = (ClCorAttrIdT) corAtoI(pTemp);
            
            corClassAttrAssocClassName = (ClCharT *) clParserAttr(corClassAssocAttr, "associatedClassName");
            if(corClassAttrAssocClassName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse associatedClassName for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            pTemp = (ClCharT *) clParserAttr(corClassAssocAttr, "maxAssoc");
            if(pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse maxAssoc for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrMaxAssoc = (ClInt32T) corAtoI(pTemp);

            corClassAttrInitialized = (ClCharT *) clParserAttr(corClassAssocAttr, "attrInitialized");
            if(corClassAttrInitialized == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrInitialized flag for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            corClassAttrWritable = (ClCharT *) clParserAttr(corClassAssocAttr, "attrWritable");
            if(corClassAttrWritable == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrWritable flag for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            if (corClassAttrMaxAssoc < 1)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Invalid max associations specified for the attribute [0x%x] in class [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
            }
            
            rc = _corNiNameToKeyGet(corClassAttrAssocClassName, &corClassAttrAssocClassId);
            if (rc != CL_OK)
            {
                /* Unable to get the class name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the Class Id from Class Name. rc [0x%x]\n", rc));
                CL_FUNC_EXIT();
                return rc;
            }
            
            pAttr = clHeapAllocate(sizeof(corAttrIntf_t)); /* Free this memory in all the places */
            if (pAttr == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory."));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }
            memset(pAttr, 0, sizeof(corAttrIntf_t));
   
 
            pAttr->classId = corClassId;
            pAttr->attrId = corClassAttrId;
            pAttr->attrType = CL_COR_ASSOCIATION_ATTR;
            pAttr->subClassId = corClassAttrAssocClassId;
            pAttr->max = corClassAttrMaxAssoc;

            classHdl = dmClassGet(pAttr->classId);
            if (classHdl == NULL)
            {
                /* Unable to get the class handle */
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the DM handle for Class Id [0x%x].", pAttr->classId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
            }
            
            rc = dmClassAttrCreate(classHdl, pAttr);
            if ((rc != CL_OK))
            {
                /* Failed to create the class attribute */
                clHeapFree(pAttr);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_CREATE, rc);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the class attribute. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }

            rc = _corNIAttrAdd(corClassId, corClassAttrId, corClassAttrName);
            if ((rc != CL_OK))
            {
                /* Failed to set the attribute name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the name for the attribute. rc [0x%x]", rc));
                clHeapFree(pAttr);
                CL_FUNC_EXIT();
                return rc;
            }

            /* Set the default user flags */
            pAttr->flags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT;
            
            /* Set the Initialized flag */
            if (strcmp(corClassAttrInitialized, "TRUE") == 0)
                pAttr->flags = pAttr->flags | CL_COR_ATTR_INITIALIZED;
            
            /* Set the Writable flag */
            if (strcmp(corClassAttrWritable, "TRUE") == 0)
                pAttr->flags = pAttr->flags | CL_COR_ATTR_WRITABLE;

            rc = _corClassAttrUserFlagsSet(classHdl, pAttr->attrId, pAttr->flags);
            if (rc != CL_OK)
            {
                /* Unable to set the user flags */                                
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the user flags for the attribute. rc [0x%x]", rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_FLAG_SET, rc);
                CL_FUNC_EXIT();
                return rc;
            }
            if(pAttrList)
            {
                clListAddTail(&pAttr->list, pAttrList);
            }
            else
            {
                clHeapFree(pAttr);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clCorParseContAttributes(ClCorClassTypeT corClassId, ClParserPtrT corClassContAttrDef,
                               ClListHeadT *pAttrList)
{
    ClRcT rc = CL_OK;
    ClParserPtrT corClassContAttr = NULL;
    ClCharT* corClassAttrName = NULL;
    ClCorAttrIdT corClassAttrId = 0;
    ClInt32T corClassAttrContClassId = 0;
    ClInt32T corClassAttrContMinInst = 0;
    ClInt32T corClassAttrContMaxInst = 0;
    ClCharT* corClassAttrContClassName = NULL;
    corAttrIntf_t *pAttr = NULL;
    CORClass_h classHdl = NULL;
    ClCharT* pTemp = NULL;

    if (corClassContAttrDef == NULL)
    {
        /* No simple attributes present */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No Simple attributes found"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    corClassContAttr = clParserChild(corClassContAttrDef, "corClassContAttrT");
    if (corClassContAttr != NULL)
    {
        /* Create all the attributes */
        for (; corClassContAttr != NULL; corClassContAttr = corClassContAttr->next)
        {
            corClassAttrName = (ClCharT *) clParserAttr(corClassContAttr, "attrName");
            if (corClassAttrName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrName in classId [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            
            pTemp =  (ClCharT *) clParserAttr(corClassContAttr, "attrId");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrId in classId [0x%x]", corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrId = (ClCorAttrIdT) corAtoI(pTemp);
           
            pTemp =(ClCharT *) clParserAttr(corClassContAttr, "maxInst");
            if (pTemp == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse MaxInst for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
            corClassAttrContMaxInst = (ClInt32T) corAtoI(pTemp);
            
            corClassAttrContClassName = (ClCharT *) clParserAttr(corClassContAttr, "containedClassName");
            if (corClassAttrContClassName == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse attrContClassName for attr [0x%x] in classId [0x%x]", corClassAttrId, corClassId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            rc = _corNiNameToKeyGet(corClassAttrContClassName, &corClassAttrContClassId);
            if (rc != CL_OK)
            {
                /* Unable to get the class name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the Class Id from Class Name. rc [0x%x]\n", rc));
                CL_FUNC_EXIT();
                return rc;
            }
   
            pAttr = clHeapAllocate(sizeof(corAttrIntf_t)); /* Free this memory in all the places */
            if (pAttr == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory."));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }
            memset(pAttr, 0, sizeof(corAttrIntf_t));

            pAttr->classId = corClassId;
            pAttr->attrId = corClassAttrId;
            pAttr->attrType = CL_COR_CONTAINMENT_ATTR;
            pAttr->subClassId = corClassAttrContClassId;
            pAttr->min = corClassAttrContMinInst;
            pAttr->max = corClassAttrContMaxInst;

            classHdl = dmClassGet(pAttr->classId);
            if (classHdl == NULL)
            {
                /* Unable to get the class handle */
                clHeapFree(pAttr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to get the DM handle for Class Id [0x%x].", pAttr->classId));
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
            }
            
            rc = dmClassAttrCreate(classHdl, pAttr);
            if ((rc != CL_OK))
            {
                /* Failed to create the class attribute */
                clHeapFree(pAttr);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_CREATE, rc);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the class attribute. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }

            rc = _corNIAttrAdd(corClassId, corClassAttrId, corClassAttrName);
            if ((rc != CL_OK))
            {
                /* Failed to set the attribute name */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the name for the attribute. rc [0x%x]", rc));
                clHeapFree(pAttr);
                CL_FUNC_EXIT();
                return rc;
            }
            if(pAttrList)
            {
                clListAddTail(&pAttr->list, pAttrList);
            }
            else
            {
                clHeapFree(pAttr);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clCorMsoSvcIdGet(ClCharT* msoType, ClCorServiceIdT* svcId)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    if (strcmp(msoType, "CL_COR_INVALID_SRVC_ID") == 0)
        *svcId = CL_COR_INVALID_SRVC_ID;
    else if (strcmp(msoType, "CL_COR_SVC_ID_DEFAULT") == 0)
        *svcId = CL_COR_SVC_ID_DEFAULT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_FAULT_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_FAULT_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_ALARM_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_ALARM_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_PROVISIONING_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_PROVISIONING_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_DUMMY_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_DUMMY_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_PM_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_PM_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_AMF_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_AMF_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_CHM_MANAGEMENT") == 0)
        *svcId = CL_COR_SVC_ID_CHM_MANAGEMENT;
    else if (strcmp(msoType, "CL_COR_SVC_ID_MAX") == 0)
        *svcId = CL_COR_SVC_ID_MAX;
    else
        rc = CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID);

    CL_FUNC_EXIT();
    return rc;
}

static ClInt32T omClassDataCompare(const void *data, const void *element)
{
    return ( (ClCorOmClassDataT*)data )->classId - ( (ClCorOmClassDataT*)element)->classId;
}

ClRcT clCorOmClassNameFromIdGet(ClOmClassTypeT classId, ClCharT *pClassName, ClInt32T maxClassSize)
{
    ClCorOmClassDataT *pOmClassData = NULL;
    ClCorOmClassDataT classData = {0};
    if(!pClassName || !maxClassSize)
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    classData.classId = classId;
    clOsalMutexLock(&gClCorClassConfig.mutex);
    pOmClassData = (ClCorOmClassDataT*)bsearch(&classData, gClCorClassConfig.pOmClassData,
                                               gClCorClassConfig.numOmClasses, 
                                               sizeof(*gClCorClassConfig.pOmClassData), omClassDataCompare);
    if(pOmClassData && pOmClassData->className)
    {
        strncpy(pClassName, pOmClassData->className, maxClassSize);
        clOsalMutexUnlock(&gClCorClassConfig.mutex);
        return CL_OK;
    }
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
    return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
}

ClRcT clCorOmClassIdFromNameGet(ClCorOmClassDataT* pOmClassData, ClUint32T numOmClasses, ClCharT* omClassName, ClOmClassTypeT* omClassId)
{
    ClUint32T idx = 0;
    
    CL_FUNC_ENTER();
   
    while (idx < numOmClasses)
    {
        if (strcmp(pOmClassData->className, omClassName) == 0)
        {
            *omClassId = pOmClassData->classId;
            CL_FUNC_EXIT();
            return (CL_OK);
        }
        
        pOmClassData++;
        idx++;
    }
    
    CL_FUNC_EXIT();
    return (CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST));
}

static ClRcT corInformationModelBuild(const ClCharT *pConfigFile, ClBoolT *pBuiltinModuleMap, 
                                      ClUint32T maxBuiltinModules)
{
    ClRcT rc = CL_OK;
    ClParserPtrT top = NULL;
    ClCharT* corClassName = NULL;
    ClInt32T corClassId = 0;
    ClInt32T corSuperClassId = 0;
    ClCharT* corSuperClassName = 0;
    ClCharT* configPath = NULL;
    ClParserPtrT corClassTreeDef = NULL;
    ClParserPtrT corClassDef = NULL;
    ClParserPtrT corClassSimpleAttrDef = NULL;
    ClParserPtrT corClassArrAttrDef = NULL;
    ClParserPtrT corClassContAttrDef = NULL;
    ClParserPtrT corClassAssocAttrDef = NULL;
    ClParserPtrT corMoMsoClassTreeDef = NULL;
    ClParserPtrT corMoClassDef = NULL;
    ClParserPtrT corMsoClassDef = NULL;
    ClCharT* corMoClassPathName = NULL;
    ClInt32T corMoClassMaxInst = 0;
    ClCharT* corMsoClassName = NULL;
    ClCharT* corMsoType = NULL;
    ClInt32T corMsoClassId = 0;
    ClCorServiceIdT  corMsoSvcId = 0;
    CORMOClass_h corMoClassHandle = NULL;
    CORMSOClass_h corMsoClassHandle = NULL;
    ClCorMOClassPathPtrT corMoClassPath = NULL;
    ClCorMsoOmTypeT* pMsoDefns = gClCorClassConfig.pMsoDefns;
    ClCorMsoOmTypeT *pTempMsoDefns = NULL;
    ClUint32T numMsoDefns = gClCorClassConfig.numMsoDefns;
    ClCharT* corOmClassName = NULL;
    ClOmClassTypeT corOmClassId = 0;
    ClCorClassTypeT corMoClassId = 0;
    CORMOClass_h hCorMoHandle = {0};
    ClParserPtrT omClasses = NULL;
    ClUint32T numOmClasses = gClCorClassConfig.numOmClasses;
    ClCorOmClassDataT* pOmClassData = gClCorClassConfig.pOmClassData;
    ClCorOmClassDataT* pTempOmClassData = NULL;
    ClParserPtrT omClassData = NULL;
    ClCharT* omClassName = NULL;
    ClOmClassTypeT omClassId = 0;
    ClCharT* pTemp = NULL;
    ClCharT configDir[CL_MAX_NAME_LENGTH];
    ClOmClassTypeT omClassIdBase = gClCorClassConfig.omClassIdBase;
    ClCorClassTypeT corClassIdBase = gClCorClassConfig.corClassIdBase;
    CL_FUNC_ENTER();

    if(!pConfigFile || !pConfigFile[0])
    {
        pConfigFile = CL_COR_META_DATA_FILE;
    }
    if(pConfigFile[0] != '/')
    {
        configPath = getenv("ASP_CONFIG");

        if (configPath == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the configuration path for the MetaStruct xml file"));
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }
    }
    else
    {
        ClUint32T dirLen = 0;
        configPath = strrchr(pConfigFile, '/');
        CL_ASSERT(configPath != NULL);
        strncpy(configDir, pConfigFile, 
                (dirLen = CL_MIN(configPath - pConfigFile, (ClUint32T)sizeof(configDir)-1)));
        configDir[dirLen] = 0;
        pConfigFile = configPath+1;
        configPath = configDir;
    }
    top = clParserOpenFile(configPath, pConfigFile);
    if(!top)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to open COR config file [%s] at [%s]",
                                        pConfigFile, configPath));
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    }

    /* Parse the OM class functionality and store the classid and classname mapping */
    omClasses = clParserChild(top, "OmClassesT");
    if (omClasses != NULL)
    {
        /* Parse the xml file and get the no. of om classes */
        omClassData = clParserChild(omClasses, "OmClassDataT");
        if (omClassData != NULL)
        {
            for (; omClassData != NULL; omClassData = omClassData->next)
            {
                numOmClasses++;
            }
        }

        pOmClassData = (ClCorOmClassDataT *) clHeapRealloc(pOmClassData, numOmClasses * sizeof(ClCorOmClassDataT));
        if (pOmClassData == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
            clParserFree(top);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        }
        memset(pOmClassData + gClCorClassConfig.numOmClasses, 0, 
               sizeof(*pOmClassData) * (numOmClasses - gClCorClassConfig.numOmClasses));
        pTempOmClassData = pOmClassData + gClCorClassConfig.numOmClasses;
        
        omClassData = clParserChild(omClasses, "OmClassDataT");
        if (omClassData != NULL)
        {
            for (; omClassData != NULL; omClassData = omClassData->next)
            {
                omClassName = (ClCharT*) clParserAttr(omClassData, "className");
                if (omClassName == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("OM class name is not specified"));
                    clParserFree(top);
                    if(!gClCorClassConfig.pOmClassData)
                        clHeapFree(pOmClassData);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                pTemp =(ClCharT *) clParserAttr(omClassData, "classId");
                if (pTemp == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("OM class id is not specified"));
                    clParserFree(top);
                    if(!gClCorClassConfig.pOmClassData)
                        clHeapFree(pOmClassData);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                if(!gClCorClassConfig.omClassIdBase)
                {
                    omClassId = (ClOmClassTypeT) corAtoI(pTemp);
                    if(omClassIdBase < omClassId)
                        omClassIdBase = omClassId;
                }
                else
                {
                    /*
                     * Check for the existing presence of the OM Class
                     */
                    if(pOmClassData && 
                       clCorOmClassIdFromNameGet(pOmClassData, gClCorClassConfig.numOmClasses,
                                                 omClassName, &omClassId) == CL_OK)
                    {
                        --numOmClasses;
                        continue;
                    }
                    omClassId = ++omClassIdBase;
                }
                pTempOmClassData->className = clStrdup(omClassName);
                pTempOmClassData->classId = omClassId;
                pTempOmClassData++;
            }
        }        
    }
    /*
     * Update the OM class id base.
     */
    gClCorClassConfig.omClassIdBase = omClassIdBase;
    gClCorClassConfig.pOmClassData = pOmClassData;
    gClCorClassConfig.numOmClasses = numOmClasses;
    qsort(gClCorClassConfig.pOmClassData, gClCorClassConfig.numOmClasses, 
          sizeof(*gClCorClassConfig.pOmClassData), omClassDataCompare);

    /* Parse the file and create the DM classes */
    corClassTreeDef = clParserChild(top, "corClassTreeDefT");
    if (corClassTreeDef != NULL)
    {
        corClassDef = clParserChild(corClassTreeDef, "corClassDefT");
        if (corClassDef != NULL)
        {
            /* Parse all the class definitions and create cor classes */
            for (; corClassDef != NULL; corClassDef = corClassDef->next)
            {
                ClCorModuleInfoT builtinModule = {0};
                ClInt32T moduleId = 0;
                CL_LIST_HEAD_INIT(&builtinModule.simpleAttrList);
                CL_LIST_HEAD_INIT(&builtinModule.arrayAttrList);
                CL_LIST_HEAD_INIT(&builtinModule.contAttrList);
                CL_LIST_HEAD_INIT(&builtinModule.assocAttrList);

                corSuperClassId = 0;

                /* Class Defintion is found. Parse it and create the classes. */
                corClassName = (ClCharT *) clParserAttr(corClassDef, "className");
                if (corClassName == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the class name"));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                
                pTemp = (ClCharT *) clParserAttr(corClassDef, "classId");
                if (pTemp == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the class id"));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }
                if(!gClCorClassConfig.corClassIdBase)
                {
                    corClassId = (ClCorClassTypeT) corAtoI(pTemp);
                    if(corClassIdBase < corClassId)
                        corClassIdBase = corClassId;
                }
                else
                {
                    if(_corNiNameToKeyGet(corClassName, &corClassId) == CL_OK)
                    {
                        /*
                         * The class name already exists. in the table. 
                         * Skip the entry.
                         */
                        continue;
                    }
                    corClassId = ++corClassIdBase;
                }
                corSuperClassName = (ClCharT *) clParserAttr(corClassDef, "superClassName");
                if (corSuperClassName == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the Super Class Name for classId [0x%x]", corClassId));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }

                if (strlen(corSuperClassName) > 0)
                {
                    rc = _corNiNameToKeyGet(corSuperClassName, &corSuperClassId);
                    if (rc != CL_OK)
                    {
                        /* Unable to get the class name */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the Class Id from Class Name. rc [0x%x]\n", rc));
                        clParserFree(top);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                
                clLogDebug("INT", "XML", "Creating dmclass with classId [%d]", corClassId);
                rc = dmClassCreate(corClassId, corSuperClassId);
                if ((rc != CL_OK) && (rc != CL_COR_SET_RC(CL_COR_ERR_CLASS_PRESENT)))
                {
                    /* Failed to create the class */
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the class [0x%x]. rc [0x%x]\n", corClassId, rc));
                    clParserFree(top);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                               CL_LOG_MESSAGE_1_CLASS_CREATE, rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
                
                clLogDebug("INT", "XML", "Adding classId [%d] to NI", corClassId);
                rc = _corNIClassAdd(corClassName, corClassId);
                if ((rc != CL_OK))
                {
                    /* Failed to set the name */
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the class name for class Id [0x%x]. rc [0x%x]", corClassId, rc));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return rc;
                }
                
                corClassSimpleAttrDef = clParserChild(corClassDef, "corClassSimpleAttrDefT");
                if (corClassSimpleAttrDef != NULL)
                {
                    /* Simple Attributes present. */
                    rc = clCorParseSimpleAttributes(corClassId, corClassSimpleAttrDef,
                                                    &builtinModule.simpleAttrList);
                    if (rc != CL_OK)
                    {
                        /* Unable to parse the simple attributes */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the simple attributes from the XML file. rc [0x%x]", rc));
                        clParserFree(top);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }

                corClassArrAttrDef = clParserChild(corClassDef, "corClassArrAttrDefT");
                
                if (corClassArrAttrDef != NULL)
                {
                    /* Array attributes present */                    
                    rc = clCorParseArrayAttributes(corClassId, corClassArrAttrDef,
                                                   &builtinModule.arrayAttrList);
                    if (rc != CL_OK)
                    {
                        /* Unable to parse array attributes */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the array attributes. rc [0x%x]", rc));
                        clParserFree(top);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                  
                /* Association Attributes */

                corClassAssocAttrDef = clParserChild(corClassDef, "corClassAssocAttrDefT");
                if (corClassAssocAttrDef != NULL)
                {
                    /* Association attributes present */
                    rc = clCorParseAssocAttributes(corClassId, corClassAssocAttrDef,
                                                   &builtinModule.assocAttrList);
                    if (rc != CL_OK)
                    {
                        /* Unable to parse Association attributes */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the association attributes. rc [0x%x]", rc));
                        clParserFree(top);
                        CL_FUNC_EXIT();
                        return rc;
                    }                    
                }

                /* Containment Attributes */
                corClassContAttrDef = clParserChild(corClassDef, "corClassContAttrDefT");
                if (corClassContAttrDef != NULL)
                {
                    /* Containment attributes present */
                    rc = clCorParseContAttributes(corClassId, corClassContAttrDef,
                                                  &builtinModule.contAttrList);
                    if (rc != CL_OK)
                    {
                        /* Unable to create containment attributes */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the containment attributes from XML file. rc [0x%x]", rc));
                        clParserFree(top);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                /*
                 * Check if a builtin module is supposed to implement this classdef
                 */
                builtinModule.classId = corClassId;
                builtinModule.superClassId = corSuperClassId;
                strncpy(builtinModule.className, corClassName, sizeof(builtinModule.className)-1);
                strncpy(builtinModule.superClassName, corSuperClassName, sizeof(builtinModule.superClassName)-1);
                if(clCorModuleClassHandler(&builtinModule, &moduleId) == CL_OK)
                {
                    if(pBuiltinModuleMap 
                       && 
                       moduleId < maxBuiltinModules 
                       &&
                       pBuiltinModuleMap[moduleId] != CL_TRUE)
                    {
                        pBuiltinModuleMap[moduleId] = CL_TRUE;
                    }
                }
                clCorModuleClassAttrFree(&builtinModule);
            } /* End of all the class definitions */
        }
    }

    gClCorClassConfig.corClassIdBase = corClassIdBase;

    /* Parse the file again and create the MO Tree */
    corMoMsoClassTreeDef = clParserChild(top, "corMoMsoClassTreeDefT");
    if (corMoMsoClassTreeDef != NULL)
    {
        /* Parse the MoMso class tree and get the OM class count */
        corMoClassDef = clParserChild(corMoMsoClassTreeDef, "corMoClassDefT");
        if (corMoClassDef != NULL)
        {
            /* Walk through each Mo and find any Mso's are there */
            for (; corMoClassDef != NULL; corMoClassDef = corMoClassDef->next)
            {
                corMsoClassDef = clParserChild(corMoClassDef, "corMsoClassDefT");
                if (corMsoClassDef != NULL)
                {
                    for (; corMsoClassDef != NULL; corMsoClassDef = corMsoClassDef->next)
                    {
                        ++numMsoDefns;
                    }
                }
            }
        }
        
        pMsoDefns = clHeapRealloc(pMsoDefns,
                                  sizeof(*pMsoDefns) * numMsoDefns);
        CL_ASSERT(pMsoDefns != NULL);
        memset(pMsoDefns + gClCorClassConfig.numMsoDefns, 0,
               sizeof(*pMsoDefns) * (numMsoDefns - gClCorClassConfig.numMsoDefns));
        pTempMsoDefns = pMsoDefns + gClCorClassConfig.numMsoDefns;
        /* Parse the MoMso class tree and process the MO (MSO) classes */
        corMoClassDef = clParserChild(corMoMsoClassTreeDef, "corMoClassDefT");
        if (corMoClassDef != NULL)
        {
            /* Process all the MOs */
            for (; corMoClassDef != NULL; corMoClassDef = corMoClassDef->next)
            
            {
                ClCharT classPathName[CL_MAX_NAME_LENGTH];
                corMoClassPathName = (ClCharT *) clParserAttr(corMoClassDef, "moPath");
                if (corMoClassPathName == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the MO class path"));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)); 
                }
                strncpy(classPathName, corMoClassPathName, sizeof(classPathName)-1);
                pTemp = (ClCharT *) clParserAttr(corMoClassDef, "maxInst");
                if (pTemp == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the MO class max instance"));
                    clParserFree(top);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)); 
                }
                corMoClassMaxInst = (ClInt32T) corAtoI(pTemp);
                
                clCorMoClassPathAlloc(&corMoClassPath); /* Free this memory appropriately */

                rc = clCorMoClassPathXlate(corMoClassPathName, corMoClassPath);
                if (rc != CL_OK)
                {
                    /* Invalid mo-path specified */
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the MO Class Path from the xml file. rc [0x%x]", rc));
                    clParserFree(top);
                    clCorMoClassPathFree(corMoClassPath);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH));
                }
                if(gClCorClassConfig.numMsoDefns > 0)
                {
                    /*
                     * Check if the MO class is already present.
                     */
                    rc = corMOClassHandleGet(corMoClassPath, &hCorMoHandle);
                    if(rc == CL_OK)
                    {
                        --numMsoDefns;
                        continue;
                    }
                }
                rc = _corMOClassCreate(corMoClassPath, corMoClassMaxInst, &corMoClassHandle);
                if (rc != CL_OK)
                {
                    /* Unable to create mo class */
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the MO class. rc [0x%x]", rc));
                    clParserFree(top);
                    clCorMoClassPathFree(corMoClassPath);
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                               CL_LOG_MESSAGE_1_MOCLASS_CREATE, rc);
                    CL_FUNC_EXIT();
                    return rc;
                }

                rc = corMOClassHandleGet(corMoClassPath, &hCorMoHandle); 
                if (rc != CL_OK)
                {
                    /* Failed to get the mo class from mo path */
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the class Id from Mo Path. rc [0x%x]", rc)); 
                    clParserFree(top);
                    clCorMoClassPathFree(corMoClassPath);
                    CL_FUNC_EXIT();
                    return rc;
                }
                corMoClassId = hCorMoHandle->id;
                clLogDebug("INT", "XML", "MO Class Id [%d]", corMoClassId);

                corMsoClassDef = clParserChild(corMoClassDef, "corMsoClassDefT");
                if (corMsoClassDef != NULL)
                {
                    /* MSO's present for this MO-Class */
                    /* Run through all the mso's and create them */
                    for (; corMsoClassDef; corMsoClassDef = corMsoClassDef->next)
                    {
                        corMsoClassName = (ClCharT *) clParserAttr(corMsoClassDef, "className");
                        if (corMsoClassName == NULL)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the MSo class name")); 
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }
                        
                        corMsoType = (ClCharT *) clParserAttr(corMsoClassDef, "msoType");
                        if (corMsoType == NULL)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the MSo type")); 
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }

                        corOmClassName = (ClCharT *) clParserAttr(corMsoClassDef, "omClassType");
                        if (corOmClassName == NULL)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the OM Class Type in the MSO class definition"));
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }

                        clLogDebug("INT", "XML", "Name [%s] to Id [%d] get", corMsoClassName, corMsoClassId);
                        rc = _corNiNameToKeyGet(corMsoClassName, &corMsoClassId);
                        if (rc != CL_OK)
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the MSO class Id for classname [%s]. rc [0x%x]", corMsoClassName, rc));
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }
                        clLogDebug("INT", "XML", "MsoClassName [%s], Mso class ID [%d]", corMsoClassName, corMsoClassId);
                        
                        rc = clCorMsoSvcIdGet(corMsoType, &corMsoSvcId);
                        if (rc != CL_OK)
                        {
                            /* ServiceId is not valid */
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid MSO type specified. rc [0x%x]", rc));
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return (CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
                        }
                        
                        clLogDebug("INT", "XML", "Creating MSO class with id[%d]", corMsoClassId);
                        rc = _corMSOClassCreate(corMoClassHandle, corMsoSvcId, corMsoClassId, &corMsoClassHandle);
                        if (rc != CL_OK )
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the MSO. rc [0x%x]", rc));
                            clParserFree(top);
                            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                                       CL_LOG_MESSAGE_1_MSOCLASS_CREATE, rc);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }

                        /* Store the OM class types */
                        /* Get the Om class Id from om class name */

                        rc = clCorOmClassIdFromNameGet(pOmClassData, numOmClasses, corOmClassName, &corOmClassId);
                        if (rc != CL_OK)
                        {
                            clParserFree(top);
                            clCorMoClassPathFree(corMoClassPath);
                            CL_FUNC_EXIT();
                            return rc;
                        }

                        pTempMsoDefns->moClassId = corMoClassId;
                        pTempMsoDefns->svcId = corMsoSvcId;
                        pTempMsoDefns->omClassId = corOmClassId;
                        ++pTempMsoDefns;
                    }
                } /* End of all the MSO class definitions */

                clCorMoClassPathFree(corMoClassPath);
            }
        } /* End of all the MO class definitions */
    } /* End of MoMsoClassTreeDef */
     
    if(numMsoDefns < gClCorClassConfig.numMsoDefns)
        numMsoDefns = gClCorClassConfig.numMsoDefns;
    gClCorClassConfig.pMsoDefns = pMsoDefns;
    gClCorClassConfig.numMsoDefns = numMsoDefns;
    qsort(gClCorClassConfig.pMsoDefns, gClCorClassConfig.numMsoDefns, 
          (ClUint32T)sizeof(*gClCorClassConfig.pMsoDefns), msoClassDefnCompare);

    /* Free the parser memory */
    if(top)
        clParserFree(top);

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clCorInformationModelBuild(const ClCharT *pConfigFile)
{
    ClRcT rc = CL_OK;
    ClBoolT *pBuiltinModuleMap = gClCorClassConfig.pBuiltinModuleMap;
    ClUint32T maxBuiltinModules = gClCorClassConfig.maxBuiltinModules;
    clOsalMutexLock(&gClCorClassConfig.mutex);
    if(!pBuiltinModuleMap)
    {
        rc = clCorModuleClassCount(&maxBuiltinModules);
        if(rc == CL_OK)
        {
            pBuiltinModuleMap = clHeapCalloc(maxBuiltinModules, sizeof(*pBuiltinModuleMap));
            CL_ASSERT(pBuiltinModuleMap != NULL);
        }
        gClCorClassConfig.pBuiltinModuleMap = pBuiltinModuleMap;
        gClCorClassConfig.maxBuiltinModules = maxBuiltinModules;
    }
    rc = corInformationModelBuild(pConfigFile, pBuiltinModuleMap, maxBuiltinModules);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&gClCorClassConfig.mutex);
        return rc;
    }
    _clCorDataSave();
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
    return rc;
}

/*
 * Invoked on the master.
 */
ClRcT clCorInformationModelLoad(const ClCharT *pConfigFile, const ClCharT *pRouteFile)
{
    ClRcT rc = CL_OK;
    ClBoolT *pBuiltinModuleMap = gClCorClassConfig.pBuiltinModuleMap;
    ClUint32T maxBuiltinModules = gClCorClassConfig.maxBuiltinModules;
    ClBoolT *pCurrentBuiltinModuleMap = NULL;
    if(!pConfigFile || !pRouteFile)
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    clOsalMutexLock(&gClCorClassConfig.mutex);
    if(!pBuiltinModuleMap)
    {
        rc = clCorModuleClassCount(&maxBuiltinModules);
        if(rc == CL_OK)
        {
            pBuiltinModuleMap = clHeapCalloc(maxBuiltinModules, sizeof(*pBuiltinModuleMap));
            CL_ASSERT(pBuiltinModuleMap != NULL);
        }
        gClCorClassConfig.pBuiltinModuleMap = pBuiltinModuleMap;
        gClCorClassConfig.maxBuiltinModules = maxBuiltinModules;
    }
    pCurrentBuiltinModuleMap = clHeapCalloc(maxBuiltinModules, sizeof(*pCurrentBuiltinModuleMap));
    CL_ASSERT(pCurrentBuiltinModuleMap !=NULL);
    rc = corInformationModelBuild(pConfigFile, pCurrentBuiltinModuleMap, maxBuiltinModules);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&gClCorClassConfig.mutex);
        if(pCurrentBuiltinModuleMap)
            clHeapFree(pCurrentBuiltinModuleMap);
        return rc;
    }
    rc = clCorModuleClassRun(pBuiltinModuleMap, pCurrentBuiltinModuleMap, maxBuiltinModules);
    _clCorDataSave();
    if(rc == CL_OK)
    {
        ClCharT dirName[CL_MAX_NAME_LENGTH] = {0};
        const ClCharT *pSuffixName = NULL;
        const ClCharT *pDirName = NULL;
        ClInt32T suffixLen = 0;
        if( (pSuffixName = strrchr(pRouteFile, '/') ) )
        {
            strncpy(dirName, pRouteFile,
                    CL_MIN((ClUint32T)sizeof(dirName)-1,
                           pSuffixName - pRouteFile));
            pDirName = (const ClCharT*)dirName;
            ++pSuffixName;
        }
        else
        {
            pSuffixName = pRouteFile;
        }
        suffixLen = strlen(pSuffixName);
        rc = clCorCreateResources(pDirName, pSuffixName, suffixLen);
    }
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
    if(pCurrentBuiltinModuleMap)
        clHeapFree(pCurrentBuiltinModuleMap);
    return rc;
}

ClRcT clCorBuiltinModuleRun(void)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gClCorClassConfig.mutex);
    rc = clCorModuleClassRun(NULL, 
                             gClCorClassConfig.pBuiltinModuleMap, 
                             gClCorClassConfig.maxBuiltinModules);
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
    return rc;
}

void clCorClassConfigInitialize(void)
{
    clOsalMutexInit(&gClCorClassConfig.mutex);
}

void clCorClassConfigOmClassIdUpdate(ClOmClassTypeT classId)
{
    clOsalMutexLock(&gClCorClassConfig.mutex);
    if(gClCorClassConfig.omClassIdBase < classId)
        gClCorClassConfig.omClassIdBase = classId;
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
}

void clCorClassConfigCorClassIdUpdate(ClCorClassTypeT classId)
{
    clOsalMutexLock(&gClCorClassConfig.mutex);
    if(gClCorClassConfig.corClassIdBase < classId)
        gClCorClassConfig.corClassIdBase = classId;
    clOsalMutexUnlock(&gClCorClassConfig.mutex);
}

ClRcT clCorConfigGet(ClUint32T* pCorSaveType)
{
    ClRcT rc = CL_OK;
    ClCharT* configPath = NULL;
    ClParserPtrT top = NULL;
    ClParserPtrT corConfig = NULL;
    ClCharT* corSaveType = NULL;

    configPath = getenv("ASP_CONFIG");

    if (configPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the configuration path for corConfig.xml file"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    top = clParserOpenFile(configPath, CL_COR_CONFIG_FILE);
    if (top != NULL)
    {
        corConfig = clParserChild(top, "corConfigT");        
        if (corConfig != NULL)
        {
            corSaveType = (ClCharT *) clParserAttr(corConfig, "corSaveOption");
            if (corSaveType == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse corSaveType flag value."));
                clParserFree(top);
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            rc = clCorSaveTypeValueGet(corSaveType, pCorSaveType);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the COR save type value. rc [0x%x]", rc));
                clParserFree(top);
                return rc;
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse the COR Configuration file."));
            clParserFree(top);
            return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to open the COR Configuration file."));
        clParserFree(top);
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    clParserFree(top);
    return CL_OK;
}

ClRcT clCorOHMaskValueGet(ClCharT* corOHMaskString, ClUint8T** corOHMaskValue)
{
    ClCharT* buff = NULL;
    ClCharT* tmpBuff = NULL;
    ClCharT* tmpStr = NULL;
    ClUint32T idx = 0;
    
    if (strlen(corOHMaskString) == 0) /* If the Mask string is of length 0, return NULL */
    {
        *corOHMaskValue = NULL;
        return (CL_OK);
    }
    
    buff = (ClCharT *) clHeapAllocate(strlen(corOHMaskString));
    if (buff == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset(buff, 0, strlen(corOHMaskString));
    
    tmpBuff = buff;
    tmpStr = (ClCharT*) strtok_r(corOHMaskString, ",", &buff);

    idx = 0;
    while (tmpStr != NULL)
    {
        ClUint8T tmpVal = 0;

        tmpVal = (ClUint8T) corAtoI((ClCharT *) tmpStr);

        (*corOHMaskValue)[idx++] = tmpVal;
        
        tmpStr = strtok_r(NULL, ",", &buff); 
    }
    
    (*corOHMaskValue)[idx] = CL_COR_OH_MASK_END_MARKER;
    
    clHeapFree(tmpBuff);
    return (CL_OK);
}

ClRcT clCorSaveTypeValueGet(ClCharT* corSaveType, ClUint32T* pCorSaveType)
{
    if (strcmp(corSaveType, "CL_COR_NO_SAVE") == 0)
        *pCorSaveType = CL_COR_NO_SAVE;

    else if (strcmp(corSaveType, "CL_COR_SAVE_PER_TXN") == 0)
        *pCorSaveType = CL_COR_SAVE_PER_TXN;
   
    else if (strcmp(corSaveType, "CL_COR_PERIODIC_SAVE") == 0)
        *pCorSaveType = CL_COR_PERIODIC_SAVE;

    else if (strcmp(corSaveType, "CL_COR_DELTA_SAVE") == 0)
        *pCorSaveType = CL_COR_DELTA_SAVE;

    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid COR save option specified."));
        return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
    }

    return (CL_OK);
}



/**
 * Function to build the information of the OM class definitions when 
 * COR read the data from the DB.
 */
ClRcT
clCorOmInfoBuild()
{
    ClRcT               rc = CL_OK;
    ClCharT             *configPath = NULL;
    ClParserPtrT        top = NULL;
    ClParserPtrT        corMoMsoClassTreeDef = NULL;
    ClParserPtrT        omClasses =  NULL;
    ClParserPtrT        omClassData =  NULL;
    ClParserPtrT        corMoClassDef = NULL;
    ClParserPtrT        corMsoClassDef = NULL;
    ClCorOmClassDataT   *pOmClassData = NULL, *pTempOmClassData = NULL;
    ClCorMsoOmTypeT     *pMsoDefns = NULL;
    ClCorMsoOmTypeT     *pTempMsoDefns = NULL;
    ClCharT             *corMoClassPathName = NULL;
    ClCharT             *omClassName = NULL;
    ClCharT             *corMsoType  = NULL;
    ClCharT             *corOmClassName = NULL;
    ClUint32T           numOmClasses    = 0;
    ClUint32T           numMsoDefns = 0;
    ClOmClassTypeT      omClassId  = 0, corOmClassId = 0;
    ClOmClassTypeT      omClassIdBase = 0;
    CORMOClass_h        hCorMoHandle = 0;
    ClCorMOClassPathPtrT corMoClassPath = NULL;
    ClCorServiceIdT     corMsoSvcId = CL_COR_INVALID_SRVC_ID;
    ClCharT* pTemp = NULL;
    
    configPath = getenv("ASP_CONFIG");

    if (configPath == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the configuration path for the MetaStruct xml file"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    top = clParserOpenFile(configPath, CL_COR_META_DATA_FILE);

    if (top != NULL)
    { 
        /* Parse the OM class functionality and store the classid and classname mapping */
        omClasses = clParserChild(top, "OmClassesT");
        if (omClasses != NULL)
        {
            /* Parse the xml file and get the no. of om classes */
            omClassData = clParserChild(omClasses, "OmClassDataT");
            if (omClassData != NULL)
            {
                for (; omClassData != NULL; omClassData = omClassData->next)
                {
                    numOmClasses++;
                }
            }

            /* Allocate memory to store the OM data */
            pOmClassData = (ClCorOmClassDataT *) clHeapCalloc(numOmClasses, sizeof(*pOmClassData));
            if (pOmClassData == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                clParserFree(top);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }
            pTempOmClassData = pOmClassData;

            /* Parse it again and store the OM data */
            omClassData = clParserChild(omClasses, "OmClassDataT");
            if (omClassData != NULL)
            {
                for (; omClassData != NULL; omClassData = omClassData->next)
                {
                    omClassName = (ClCharT*) clParserAttr(omClassData, "className");
                    if (omClassName == NULL)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse OM class name"));
                        clParserFree(top);
                        clHeapFree(pOmClassData);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                    }

                    pTemp = (ClCharT *) clParserAttr(omClassData, "classId");
                    if (pTemp == NULL)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse OM class Id"));
                        clParserFree(top);
                        clHeapFree(pOmClassData);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                    }
                    omClassId = (ClOmClassTypeT) corAtoI(pTemp);
                    
                    pTempOmClassData->className = clStrdup(omClassName);
                    pTempOmClassData->classId = omClassId;
                    pTempOmClassData++;
                    if(omClassIdBase < omClassId)
                        omClassIdBase = omClassId;
                }
            }
        }

        /* Parse the file again and create the MO Tree */
        corMoMsoClassTreeDef = clParserChild(top, "corMoMsoClassTreeDefT");
        if (corMoMsoClassTreeDef != NULL)
        {
            /* Parse the MoMso class tree and get the OM class count */
            corMoClassDef = clParserChild(corMoMsoClassTreeDef, "corMoClassDefT");
            if (corMoClassDef != NULL)
            {
                /* Walk through each Mo and find any Mso's are there */
                for (; corMoClassDef != NULL; corMoClassDef = corMoClassDef->next)
                {
                    corMsoClassDef = clParserChild(corMoClassDef, "corMsoClassDefT");
                    if (corMsoClassDef != NULL)
                    {
                        for (; corMsoClassDef != NULL; corMsoClassDef = corMsoClassDef->next)
                        {
                            ++numMsoDefns;
                        }
                    }
                }
            }

            /* This malloc() needs to be freed when the component is finalized */
            pMsoDefns = clHeapCalloc(numMsoDefns, sizeof(*pMsoDefns));
            CL_ASSERT(pMsoDefns != NULL);
            pTempMsoDefns = pMsoDefns;
            /* Parse the MoMso class tree and process the MO (MSO) classes */
            corMoClassDef = clParserChild(corMoMsoClassTreeDef, "corMoClassDefT");
            if (corMoClassDef != NULL)
            {
                /* Process all the MOs */
                for (; corMoClassDef != NULL; corMoClassDef = corMoClassDef->next)
                {
                    corMoClassPathName = (ClCharT *) clParserAttr(corMoClassDef, "moPath");
                    if (corMoClassPathName == NULL)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse MO class path"));
                        clParserFree(top);
                        clHeapFree(pOmClassData);
                        clHeapFree(pMsoDefns);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                    }
                    
                    clCorMoClassPathAlloc(&corMoClassPath); /* Free this memory appropriately */

                    rc = clCorMoClassPathXlate(corMoClassPathName, corMoClassPath);
                    if (rc != CL_OK)
                    {
                        /* Invalid mo-path specified */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the MO Class Path from the xml file. rc [0x%x]", rc));
                        clParserFree(top);
                        clHeapFree(pOmClassData);
                        clCorMoClassPathFree(corMoClassPath);
                        clHeapFree(pMsoDefns);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH));
                    }

                    rc = corMOClassHandleGet(corMoClassPath, &hCorMoHandle); 
                    if (rc != CL_OK)
                    {
                        /* Failed to get the mo class from mo path */
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the class Id from Mo Path. rc [0x%x]", rc)); 
                        clParserFree(top);
                        clHeapFree(pOmClassData);
                        clCorMoClassPathFree(corMoClassPath);
                        clHeapFree(pMsoDefns);
                        CL_FUNC_EXIT();
                        return rc;
                    }

                    corMsoClassDef = clParserChild(corMoClassDef, "corMsoClassDefT");
                    if (corMsoClassDef != NULL)
                    {
                        /* MSO's present for this MO-Class */
                        /* Run through all the mso's and create them */
                        for (; corMsoClassDef; corMsoClassDef = corMsoClassDef->next)
                        {
                            corMsoType = (ClCharT *) clParserAttr(corMsoClassDef, "msoType");
                            if (corMsoType == NULL)
                            {
                                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse MsoType in MsoClassDefinition"));
                                clParserFree(top);
                                clHeapFree(pOmClassData);
                                clHeapFree(pMsoDefns);
                                clCorMoClassPathFree(corMoClassPath);
                                CL_FUNC_EXIT();
                                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                            }

                            corOmClassName = (ClCharT *) clParserAttr(corMsoClassDef, "omClassType");
                            if (corOmClassName == NULL)
                            {
                                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to parse OM class Name as part of MSO class definition"));
                                clParserFree(top);
                                clHeapFree(pOmClassData);
                                clHeapFree(pMsoDefns);
                                clCorMoClassPathFree(corMoClassPath);
                                CL_FUNC_EXIT();
                                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                            }

                            rc = clCorMsoSvcIdGet(corMsoType, &corMsoSvcId);
                            if (rc != CL_OK)
                            {
                                /* ServiceId is not valid */
                                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid MSO type specified. rc [0x%x]", rc));
                                clParserFree(top);
                                clHeapFree(pOmClassData);
                                clHeapFree(pMsoDefns);
                                clCorMoClassPathFree(corMoClassPath);
                                CL_FUNC_EXIT();
                                return (CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID));
                            }
                           
                            /* Store the OM class types */
                            /* Get the Om class Id from om class name */

                            rc = clCorOmClassIdFromNameGet(pOmClassData, numOmClasses, corOmClassName, &corOmClassId);
                            if (rc != CL_OK)
                            {
                                clParserFree(top);
                                clHeapFree(pOmClassData);
                                clHeapFree(pMsoDefns);
                                clCorMoClassPathFree(corMoClassPath);
                                CL_FUNC_EXIT();
                                return rc;
                            }

                            pTempMsoDefns->moClassId = hCorMoHandle->id;
                            pTempMsoDefns->svcId = corMsoSvcId;
                            pTempMsoDefns->omClassId = corOmClassId;
                            ++pTempMsoDefns;
                        }
                    } /* End of all the MSO class definitions */

                    /* Freeing the Mo class path */
                    clCorMoClassPathFree(corMoClassPath);
                }
            } /* End of all the MO class definitions */
        } /* End of MoMsoClassTreeDef */
    }

    clParserFree(top);

    gClCorClassConfig.omClassIdBase = omClassIdBase;
    gClCorClassConfig.pOmClassData = pOmClassData;
    gClCorClassConfig.numOmClasses = numOmClasses;
    gClCorClassConfig.pMsoDefns = pMsoDefns;
    gClCorClassConfig.numMsoDefns = numMsoDefns;
    qsort(gClCorClassConfig.pOmClassData, gClCorClassConfig.numOmClasses, 
          sizeof(*gClCorClassConfig.pOmClassData), omClassDataCompare);
    qsort(gClCorClassConfig.pMsoDefns, gClCorClassConfig.numMsoDefns,
          (ClUint32T)sizeof(*gClCorClassConfig.pMsoDefns), msoClassDefnCompare);

    CL_FUNC_EXIT();

    return rc;
}


/**
 * This is the IPI to be used for testing.
 */ 

ClRcT VDECL(_clCorDelaysRequestProcess) (CL_IN ClEoDataT eoData, ClBufferHandleT inMsgH, 
                            ClBufferHandleT outMsgH)
{
    ClRcT  rc = CL_OK;
#ifdef CL_COR_MEASURE_DELAYS
    ClTimeT time = 0;
    ClTimeT *pTime = NULL;
    ClCorDelayRequestOptT   opt = CL_COR_DELAY_INVALID_REQ;
	ClIocNodeAddressT	    corSlaveNodeAddressId = 0; 
    ClUint32T               rmdFlags = 0;
    ClRmdOptionsT           rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClIocAddressT           destAddr = {{0}};

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("UTL", "DLY", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    rc = clXdrUnmarshallClInt32T(inMsgH, &opt);
    if(CL_OK != rc)
    {
        clLogError("UTL", "DLY", "Failed while unmarshalling the delay request option. rc[0x%x]", rc);
        return rc;
    }

    if(opt < CL_COR_DELAY_DM_UNPACK_REQ || opt > CL_COR_DELAY_MAX_REQ) 
    {
        clLogError("UTL", "DLY", "Invalid operation request type supplied.");
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

    clLogTrace("UTL", "DLY", "Got the request for getting the processing time for opt[%d]", opt);
    switch (opt)
    {

        case CL_COR_DELAY_DM_UNPACK_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.dmDataUnpack, time, pTime);
        break;
        case CL_COR_DELAY_MOCLASSTREE_UNPACK_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.moClassTreeUnpack, time, pTime);
        break;
        case CL_COR_DELAY_OBJTREE_UNPACK_REQ: 
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.objTreeUnpack, time, pTime);
        break;
        case CL_COR_DELAY_ROUTELIST_UNPACK_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.routeListUnpack, time, pTime);
        break;
        case CL_COR_DELAY_NITABLE_UNPACK_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.niTableUnpack, time, pTime);
        break;
        case CL_COR_DELAY_SYNCUP_STANDBY_REQ: 
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.corStandbySyncup, time, pTime);
        break;
        case CL_COR_DELAY_OBJ_DB_RECREATE_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.objDbRecreate, time, pTime);
        break;
        case CL_COR_DELAY_ROUTE_DB_RECREATE_REQ:
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.routeDbRecreate, time, pTime);
        break;
        case CL_COR_DELAY_DB_RECREATE_REQ:    
            CL_COR_OP_DELAY_PROCESS(gCorOpDelay.corDbRecreate, time, pTime);
        break;
        case CL_COR_DELAY_DM_PACK_REQ:
                time = gCorOpDelay.dmDataPack;
        break;
        case CL_COR_DELAY_MOCLASSTREE_PACK_REQ:
                time = gCorOpDelay.moClassTreePack;
        break;
        case CL_COR_DELAY_OBJTREE_PACK_REQ:
                time = gCorOpDelay.objTreePack;
        break;
        case CL_COR_DELAY_ROUTELIST_PACK_REQ:
                time = gCorOpDelay.routeListPack;
        break;
        case CL_COR_DELAY_NITABLE_PACK_REQ:
                time = gCorOpDelay.niTablePack;
        break;
        case CL_COR_DELAY_SYNCUP_ACTIVE_REQ:
                time = gCorOpDelay.dmDataPack + gCorOpDelay.moClassTreePack + gCorOpDelay.objTreePack
                        + gCorOpDelay.routeListPack + gCorOpDelay.niTablePack;
        break;
        case CL_COR_DELAY_OBJTREE_DB_RESTORE_REQ:
                time = gCorOpDelay.objTreeRestoreDb;
        break;
        case CL_COR_DELAY_ROUTELIST_DB_RESTORE_REQ:
                time = gCorOpDelay.routeListRestoreDb;
        break;
        case CL_COR_DELAY_DB_RESTORE_REQ:
                time = gCorOpDelay.objTreeRestoreDb + gCorOpDelay.routeListRestoreDb;
        break;
        default:
            clLogError("UTL", "DLY", "Invalid optype specifed.");
    } 

sendData:
    rc = clXdrMarshallClUint64T (&time, outMsgH, 0);
    if(CL_OK != rc)
    {
        clLogError("UTL", "DLY", "Failed while packing the time information. rc[0x%x]", rc);
        return rc;
    }

    return CL_OK;

getFromStandby:

    clLogNotice("UTL", "DLY", "Data is not at active COR for operatoin [%d] \
            so routing the request to standby COR. ", opt);
    rmdFlags |= CL_RMD_CALL_NEED_REPLY;

    /* Sending a RMD to the Peer CORs to get the information not available on active COR. */
    rc = clCorSlaveIdGet(&corSlaveNodeAddressId);
    if (CL_OK != rc)
    {
        clLogError("UTL", "DLY", "Failed while getting the standby COR address, \
                so returning value unavailable. rc[0x%x]", rc);
        return rc;
    }

    /**
     * Sending the RMD only:
     * 1. When there is a standby COR present in the cor list.
     * 2. Taking care for the standby if it is reaching this part of code.
     */ 
    if ((corSlaveNodeAddressId != 0) && ( corSlaveNodeAddressId != clIocLocalAddressGet()))
    {
        destAddr.iocPhyAddress.nodeAddress = corSlaveNodeAddressId;
        destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;
        rc = clRmdWithMsg(destAddr, CL_COR_PROCESSING_DELAY_OP, inMsgH, outMsgH, 
                            rmdFlags&(~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
        if(rc != CL_OK)
        {
             clLogError("UTL", "DLY", "Failed while getting the information from \
                     the standby COR server for op[%d]. rc [0x%x]", opt, rc);
             return rc;
        }

        clLogNotice("UTL", "DLY", "Got the response for the operation type [%d] from standby COR.", opt);

        if(pTime == NULL)
        {
            clLogNotice("UTL", "DLY", "The pointer obtained is null, so assigning it properly.");
            pTime = &time;
        }

        rc = clXdrUnmarshallClUint64T(outMsgH, pTime);
        if(CL_OK != rc)
        {
            clLogError("UTL", "DLY", "Failed while unmarshalling the time \
                    information for [%d] from slave. rc[0x%x]", opt, rc);
        }
    }
    else
    {
        clLogTrace("UTL", "DLY", "No stand-by entity present, so couldn't get the unpack times");
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    }

#endif 
    return rc;
}

/*
 * Mso configuration function to minimize the interaction between the applications and COR server.
 * Currently this function is very specific to alarm, should be made generic if other Mso services
 * require the same functionality.
 */
ClRcT VDECL(_clCorMsoConfigOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                        ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    VDECL_VER(ClCorMsoConfigIDLT, 4, 1, 0) msoConfig = {{0}};
    ClIocNodeAddressT corSlaveNodeAddr = 0;
    ClUint32T resIdx = 0;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData = NULL;

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("UTL", "MSOCONFIG", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    /* 
     * Need to take care of sync up data consistency.
     */
    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);

    if(pCorSyncState == CL_COR_SYNC_STATE_INPROGRESS)
    {    
        clLogNotice("TXN", "PRE", "COR slave syncup in progress .... retry again ..");
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }    

    gCorTxnRequestCount++;
    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

    rc = VDECL_VER(clXdrUnmarshallClCorMsoConfigIDLT, 4, 1, 0)(inMsgHandle, (void *)&msoConfig);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to unmarshall ClCorMsoConfigIDLT. rc [0x%x]", rc);

        /* Decrement the transaction count */ 
        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
        --gCorTxnRequestCount;
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

        return rc;
    }

    clCorClientToServerVersionValidate(msoConfig.version, rc);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to validate client to server version.");
        rc = CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);

        goto unlock_and_free;
    }

    clCorSlaveIdGet(&corSlaveNodeAddr);

    if (msoConfig.msoConfigOp == COR_MSOCONFIG_CLIENT_TO_SERVER && 
            corSlaveNodeAddr != 0) 
    {
        /* Include both the CORs */
        ClRmdResponseContextHandleT syncRespHandle = 0;
        ClCorCommInfoT* pSyncList = NULL;
        corMsoConfigT* pAsyncConfig = NULL;
        ClRmdAsyncOptionsT asyncOpt = {0};
        ClUint32T size = 0;
        ClIocAddressT destAddr = {{0}};
        ClBufferHandleT asyncInMsgHandle = 0;
        ClBufferHandleT asyncOutMsgHandle = 0;
        ClUint32T       rmdFlags = CL_RMD_CALL_ASYNC | CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_ATMOST_ONCE;
        ClRmdOptionsT   rmdOptions = {  COR_RMD_DFLT_TIME_OUT,
                                        COR_RMD_DFLT_RETRIES,
                                        CL_RMD_DEFAULT_PRIORITY,
                                        CL_RMD_DEFAULT_TRANSPORT_HANDLE
                                        };

        rc = clRmdResponseDefer(&syncRespHandle);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to defer RMD sync response. rc [0x%x]", rc);
            goto unlock_and_free;
        }

        rc = corListIdsGet(&pSyncList, &size);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to get list of COR ids. rc [0x%x]", rc);
            clRmdSyncResponseSend(syncRespHandle, 0, rc);
            goto unlock_and_free;
        }

        msoConfig.msoConfigOp = COR_MSOCONFIG_SERVER_TO_SERVER;

        rc = clBufferCreate(&asyncInMsgHandle);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to create the buffer. rc [0x%x]", rc);
            clHeapFree(pSyncList);
            clRmdSyncResponseSend(syncRespHandle, 0, rc);
            goto unlock_and_free; 
        }

        rc = clBufferCreate(&asyncOutMsgHandle);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to create the buffer. rc [0x%x]", rc);
            clHeapFree(pSyncList);
            clRmdSyncResponseSend(syncRespHandle, 0, rc);
            clBufferDelete(&asyncInMsgHandle);
            goto unlock_and_free; 
        }

        rc = VDECL_VER(clXdrMarshallClCorMsoConfigIDLT, 4, 1, 0) (&msoConfig, asyncInMsgHandle, 0);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to marshall ClCorMsoConfigIDLT. rc [0x%x]", rc);
            clHeapFree(pSyncList);
            clRmdSyncResponseSend(syncRespHandle, 0, rc);
            clBufferDelete(&asyncInMsgHandle);
            clBufferDelete(&asyncOutMsgHandle);
            goto unlock_and_free; 
        }

        pAsyncConfig = clHeapAllocate(sizeof(corMsoConfigT));
        if (!pAsyncConfig)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to allocate memory.");
            clHeapFree(pSyncList);
            clRmdSyncResponseSend(syncRespHandle, 0, rc);
            clBufferDelete(&asyncInMsgHandle);
            clBufferDelete(&asyncOutMsgHandle);
            rc = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            goto unlock_and_free; 
        }

        pAsyncConfig->count = size;
        pAsyncConfig->syncRespHandle = syncRespHandle;
        pAsyncConfig->outMsgHandle = outMsgHandle;

        clOsalMutexInit(&pAsyncConfig->mutex);

        asyncOpt.pCookie = (void *) pAsyncConfig;
        asyncOpt.fpCallback = _corMsoConfigReplyCB;

        clLogTrace("UTL", "MSOCONFIG", "Syching Alarm configuration information with standby COR..");

        clOsalMutexLock(&pAsyncConfig->mutex);

        for (i=0; i<size; i++)
        {
            destAddr.iocPhyAddress = pSyncList[i].addr;

            rc = clRmdWithMsg(destAddr, COR_EO_MSO_CONFIG, asyncInMsgHandle, asyncOutMsgHandle, rmdFlags, &rmdOptions, &asyncOpt);
            if (rc != CL_OK)
            {
                clLogError("UTL", "MSOCONFIG", "Failed to send async rmd to cor server running on node [0x%x]. rc [0x%x]",
                        destAddr.iocPhyAddress.nodeAddress, rc);
                clOsalMutexUnlock(&pAsyncConfig->mutex);
                clRmdSyncResponseSend(syncRespHandle, 0, rc);
                clBufferDelete(&asyncInMsgHandle);
                clBufferDelete(&asyncOutMsgHandle);
                clHeapFree(pSyncList);
                clHeapFree(pAsyncConfig);
                goto unlock_and_free; 
            }
        }

        clOsalMutexUnlock(&pAsyncConfig->mutex);

        clHeapFree(pSyncList);

        goto free_and_exit;
    }
    else
    {
        rc = _clCorConfigureMso(&msoConfig);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to configure the Mso. rc [0x%x]", rc);
            goto unlock_and_free;
        }

        goto unlock_and_free;
    }

unlock_and_free:

    /* Decrement the transaction count */ 
    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
    --gCorTxnRequestCount;
    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
    
free_and_exit:

    for (resIdx=0; resIdx<msoConfig.alarmInfo.noOfRes; resIdx++)
    {
        pResData = (msoConfig.alarmInfo.pResourceData + resIdx);

        if (pResData)
        {
            if (pResData->noOfAlarms && pResData->pAlarmProfile)
                clHeapFree(pResData->pAlarmProfile);
        }
    }

    if (msoConfig.alarmInfo.pResourceData)
        clHeapFree(msoConfig.alarmInfo.pResourceData);

    return rc;
}

void _corMsoConfigReplyCB(ClRcT retCode,
        ClPtrT pCookie,
        ClBufferHandleT inMsgHdl,
        ClBufferHandleT outMsgHdl)
{
    ClRcT rc = CL_OK;
    corMsoConfigT* pAsyncConfig = NULL;

    if (!pCookie)
    {
        clLogError("UTL", "MSOCONFIG", "NULL pointer passed.");
        return;
    }

    pAsyncConfig = (corMsoConfigT *) pCookie;

    clOsalMutexLock(&pAsyncConfig->mutex);
    --pAsyncConfig->count;

    if (pAsyncConfig->count == 0)
    {
        /* Decrement the transaction count */ 
        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
        if (gCorTxnRequestCount > 0)
            --gCorTxnRequestCount;
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

        /* Send the response to the client */
        rc = clRmdSyncResponseSend(pAsyncConfig->syncRespHandle, pAsyncConfig->outMsgHandle, retCode);
        if (rc != CL_OK)
        {
            clLogError("UTL", "MSOCONFIG", "Failed to send deferred response to the client. rc [0x%x]", rc);
        }

        clBufferDelete(&outMsgHdl);
        clOsalMutexUnlock(&pAsyncConfig->mutex);

        clHeapFree(pAsyncConfig);
        return;
    }

    clOsalMutexUnlock(&pAsyncConfig->mutex);

    return;
}

ClRcT _clCorConfigureMso(VDECL_VER(ClCorMsoConfigIDLT, 4, 1, 0)* pMsoConfig)
{
    ClRcT rc = CL_OK;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData = NULL;
    ClUint32T resIdx = 0;
    ClUint32T moIdIdx = 0;
    ClIocPhysicalAddressT alarmOwner = {0};
    DMContObjHandle_t   dmContObjHdl = {{0}};

    for (resIdx = 0; resIdx < pMsoConfig->alarmInfo.noOfRes; resIdx++)
    {
        pResData = (pMsoConfig->alarmInfo.pResourceData + resIdx);
        CL_ASSERT(pResData);

        /* Alarm profile values are in host format,
         * check the host format and convert the values 
         * to network format if required.
         */
        if (CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
        {
            rc = _corAlarmMsoValueSwap(pResData);
            if (rc != CL_OK)
            {
                clLogWarning("UTL", "MSOCONFIG", "Failed to convert the alarm profile values into network format. rc [0x%x]",
                        rc);
                rc = CL_OK;
                continue;
            }
        }

        clLogTrace("UTL", "MSOCONFIG", "No. of resources to be configured for class [%d]: is : [%u]", 
                pResData->classId, pResData->noOfInst);

        for (moIdIdx = 0; moIdIdx < pResData->noOfInst; moIdIdx++)
        {
            ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};
            ClCorMOIdPtrT pMoId = (pResData->pAlarmInstList + moIdIdx);  
            CL_ASSERT(pMoId);

            _clCorMoIdStrGet(pMoId, moIdStr);

            /*
             * Check if the current component is the alarm owner.
             */
            clOsalMutexLock(gCorRouteInfoMutex);
            rc = _clCorMoIdToComponentAddressGet(pMoId, &alarmOwner); 
            if (rc != CL_OK)
            {
                clLogError("UTL", "MSOCONFIG", "Failed to get alarm owner for MO [%s]. rc [0x%x]", 
                        moIdStr, rc);
                clOsalMutexUnlock(gCorRouteInfoMutex);
                return rc;
            }
            clOsalMutexUnlock(gCorRouteInfoMutex);

            if ( !(pMsoConfig->compAddr.nodeAddress == alarmOwner.nodeAddress && 
                    pMsoConfig->compAddr.portId == alarmOwner.portId))
            {
                /* 
                 * It is not the alarm owner. Need not do alarm configuration.
                 */
                continue;
            }

            rc = moId2DMHandle(pMoId, &dmContObjHdl.dmObjHandle);
            if (rc != CL_OK)
            {
                /* It can be a wild card resource, in that case, walk all the instances 
                 * and configure the alarm profiles. 
                 */
                if (clCorMoIdIsWildCard(pMoId) == CL_TRUE)
                {
                    ClCorMOIdT rootMoId = {{{0}}};

                    memcpy(&rootMoId, pMoId, sizeof(ClCorMOIdT));
                    rootMoId.depth--;

                    rc = _clCorObjectWalk(&rootMoId, pMoId, _clCorAlarmMsoConfigureCB, CL_COR_MSO_WALK,
                            (void *) pResData);
                    if (rc != CL_OK)
                    {
                        clLogWarning("UTL", "MSOCONFIG", "Failed to do object walk. rc [0x%x]", rc);
                        rc = CL_OK;
                        continue;
                    }
                }
                else
                {
                    clLogTrace("UTL", "MSOCONFIG", "MO object [%s] obtained for alarm configuration "
                            "doesn't exist in COR. rc [0x%x]", moIdStr, rc);
                    rc = CL_OK;
                }
            }
            else
            {
                clLogTrace("UTL", "MSOCONFIG", "MO object [%s] obtained for alarm configuration "
                        "exists in COR. rc [0x%x]", moIdStr, rc);

                rc = _clCorAlarmMsoProfilesConfig(pMoId, dmContObjHdl, (void *) pResData);
                if (rc != CL_OK)
                {
                    clLogError("UTL", "MSOCONFIG", "Failed to configure alarm profiles for the MO [%s]. rc [0x%x]", 
                            moIdStr, rc);
                    return rc;
                }
            }
        }
    }

    return CL_OK; 
}

void _clCorAlarmMsoConfigureCB(void* pMoId, ClBufferHandleT pData)
{
    ClRcT rc = CL_OK;
    DMContObjHandle_t dmContObjHdl = {{0}};

    if (!pMoId || !pData)
    {
        clLogError("UTL", "MSOCONFIG", "NULL pointer passed.");
        return;
    }

    rc = moId2DMHandle((ClCorMOIdPtrT) pMoId, &dmContObjHdl.dmObjHandle);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to get DM handle from MoId. rc [0x%x]", rc);
        goto error_exit;
    }

    rc = _clCorAlarmMsoProfilesConfig((ClCorMOIdPtrT) pMoId, dmContObjHdl, (void *) pData);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to configure the alarm profiles. rc [0x%x]", rc);
        goto error_exit;
    }

error_exit:
    return;
}

ClRcT _clCorAlarmMsoProfilesConfig(ClCorMOIdPtrT pMoId, DMContObjHandle_t dmContObjHdl, void* pData)
{
    ClRcT rc = CL_OK;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData = NULL;
    VDECL_VER(ClCorAlarmProfileDataIDLT, 4, 1, 0)* pAlarmProfile = NULL;
    ClCorAttrPathT      attrPath = {{{0}}};
    ClUint32T almIdx = 0;
    ClUint32T size = 0;
    ClUint32T probCause = 0;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};
    ClCorAttrIdT attrId = 0;

    if (!pMoId || !pData)
    {
        clLogError("UTL", "MSOCONFIG", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pResData = (VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)*) pData;

    clCorAttrPathInitialize(&attrPath);
    clCorAttrPathAppend(&attrPath, 0x3, 0);
    
    _clCorMoIdStrGet(pMoId, moIdStr);
    clLogTrace("UTL", "MSOCONFIG", "Configuring alarm profiles for the moId : [%s]",
            moIdStr);
    /* 
     * This piece of code is just a hack to find whether the mso is already configured 
     * in cor, this has to be replaced by marking an attribute.
     */
    attrId = 14; /* Attribute Id of probable cause */
    size = sizeof(ClUint32T);
    dmContObjHdl.pAttrPath = NULL;
    rc = dmObjectAttrGet(&dmContObjHdl, attrId, 0, &probCause, &size);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to get prob cause attribute of the mso. rc [0x%x]", rc);
        clLogError("UTL", "MSOCONFIG", "Class ID : [%u], instance : [%u]", dmContObjHdl.dmObjHandle.classId,
                dmContObjHdl.dmObjHandle.instanceId);
        return rc;
    }

    if (probCause != 0)
    {
        /* Alarm MSO is already configured. */
        clLogNotice("UTL", "MSOCONFIG", "Alarm resource [%s] is already configured.", moIdStr);
        return CL_OK;
    }

    /* Configure the alarm profiles */
    for (almIdx=0; almIdx < pResData->noOfAlarms; almIdx++)
    {
        pAlarmProfile = pResData->pAlarmProfile + almIdx;
        CL_ASSERT(pAlarmProfile);

        clCorAttrPathIndexSet(&attrPath, 1, almIdx);
        dmContObjHdl.pAttrPath = &attrPath;

        rc = _corAlarmMsoValueSet(dmContObjHdl, pAlarmProfile);
        if (rc != CL_OK)
            continue;

        clCorDeltaDbStore(*pMoId, &attrPath, CL_COR_DELTA_SET);
    }

    /* Store the Mso level attributes */
    clCorDeltaDbStore(*pMoId, NULL, CL_COR_DELTA_SET);

    return CL_OK;
}

ClRcT _corAlarmMsoValueSwap(VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData)
{
    ClRcT rc = CL_OK;
    VDECL_VER(ClCorAlarmProfileDataIDLT, 4, 1, 0)* pAlarmProfile = NULL;
    ClUint32T i = 0;

    if (! pResData)
    {
        clLogError("UTL", "MSOCONFIG", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    for (i=0; i<pResData->noOfAlarms; i++)
    {
        pAlarmProfile = (pResData->pAlarmProfile + i);
        CL_ASSERT(pAlarmProfile);
    
        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->probCause,
                sizeof(pAlarmProfile->probCause),
                CL_COR_UINT32,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->specProb,
                sizeof(pAlarmProfile->specProb),
                CL_COR_UINT32,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->affectedAlarms,
                sizeof(pAlarmProfile->affectedAlarms),
                CL_COR_UINT64,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->genRule,
                sizeof(pAlarmProfile->genRule),
                CL_COR_UINT64,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->supRule,
                sizeof(pAlarmProfile->supRule),
                CL_COR_UINT64,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->genRuleRel,
                sizeof(pAlarmProfile->genRuleRel),
                CL_COR_UINT32,
                0);
        if (rc != CL_OK)
            goto error_exit;

        rc = clCorObjAttrValSwap((ClUint8T *) &pAlarmProfile->supRuleRel,
                sizeof(pAlarmProfile->supRuleRel),
                CL_COR_UINT32,
                0);
        if (rc != CL_OK)
            goto error_exit;
    }

    return CL_OK;

error_exit:
    clLogError("UTL", "MSOCONFIG", "Failed to convert alarm profile values into network format. rc [0x%x]", rc);
    return rc;
}

ClRcT _corAlarmMsoValueSet(DMContObjHandle_t dmContObjHdl, VDECL_VER(ClCorAlarmProfileDataIDLT, 4, 1, 0)* pAlarmProfile)
{
    ClUint32T profileIndex = 0;
    ClCorAttrIdT attrId = 0;
    ClRcT rc = CL_OK;
    CORClass_h classH = NULL;
    void* instBuf = NULL;

    if (!pAlarmProfile)
    {
        clLogError("UTL", "MSOCONFIG", "NULL pointer passed.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    profileIndex = clCorAttrPathIndexGet(dmContObjHdl.pAttrPath);
   
    clOsalMutexLock(gCorMutexes.gCorServerMutex);

    rc = dmObjectAttrHandleGet(&dmContObjHdl, &classH, &instBuf);
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to get object buffer. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 17;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->probCause),
            sizeof(pAlarmProfile->probCause));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 19;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->specProb),
            sizeof(pAlarmProfile->specProb));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    /* CL_COR_UINT8 */
    attrId = 18;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->category),
            sizeof(pAlarmProfile->category));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    /* CL_COR_UINT8 */
    attrId = 20;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->severity),
            sizeof(pAlarmProfile->severity));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    /* CL_COR_UINT8 */
    attrId = 1;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->contValidEntry),
            sizeof(pAlarmProfile->contValidEntry));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 21;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->affectedAlarms),
            sizeof(pAlarmProfile->affectedAlarms));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 22;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->genRule),
            sizeof(pAlarmProfile->genRule));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 23;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->supRule),
            sizeof(pAlarmProfile->supRule));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }


    attrId = 24;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->genRuleRel),
            sizeof(pAlarmProfile->genRuleRel));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 25;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->supRuleRel),
            sizeof(pAlarmProfile->supRuleRel));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }


    /* 
     * Set the Mso level attributes.
     */
    dmContObjHdl.pAttrPath = NULL;
    rc = dmObjectAttrHandleGet(&dmContObjHdl, &classH, &instBuf);
    if (rc != CL_OK)
        goto error_exit;

    /* CL_COR_UINT8 */
    attrId = 9;
    rc = dmObjectBufferAttrSet(classH, 
            instBuf,
            attrId,
            -1,
            &(pAlarmProfile->pollAlarm),
            sizeof(pAlarmProfile->pollAlarm));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

    attrId = 14;
    rc = dmObjectBufferAttrSet(classH,
            instBuf,
            attrId,
            profileIndex,
            &(pAlarmProfile->probCause),
            sizeof(pAlarmProfile->probCause));
    if (rc != CL_OK)
    {
        clLogError("UTL", "MSOCONFIG", "Failed to set the attribute value in COR. rc [0x%x]", rc);
        goto error_exit;
    }

error_exit:    
    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);

    clLogTrace("UTL", "MSOCONFIG", "Configured alarm profile [%d]", profileIndex);

    clLogTrace("UTL", "MSOCONFIG", "Prob Cause : [%u]", pAlarmProfile->probCause);
    clLogTrace("UTL", "MSOCONFIG", "Spec Problem : [%u]", pAlarmProfile->specProb);
    clLogTrace("UTL", "MSOCONFIG", "Category : [%u]", pAlarmProfile->category);
    clLogTrace("UTL", "MSOCONFIG", "Severity : [%u]", pAlarmProfile->severity);
    clLogTrace("UTL", "MSOCONFIG", "Cont valid entry : [%u]", pAlarmProfile->contValidEntry);
    clLogTrace("UTL", "MSOCONFIG", "Poll Alarm : [%u]", pAlarmProfile->pollAlarm);
    clLogTrace("UTL", "MSOCONFIG", "Affected Alams BM : [%llu]", pAlarmProfile->affectedAlarms);
    clLogTrace("UTL", "MSOCONFIG", "GenRule : [%llu]", pAlarmProfile->genRule);
    clLogTrace("UTL", "MSOCONFIG", "SupRule : [%llu]", pAlarmProfile->supRule);
    clLogTrace("UTL", "MSOCONFIG", "GenRuleRel : [%u]", pAlarmProfile->genRuleRel);
    clLogTrace("UTL", "MSOCONFIG", "SupRuleRel : [%u]", pAlarmProfile->supRuleRel);
 
    return rc; 
}

ClRcT clCorSyncDataWithSlave(ClInt32T funcId, ClBufferHandleT inMsgHandle)
{
    ClIocNodeAddressT corSlaveNodeAddr = 0;
    ClIocAddressT destAddr = {{0}};
    ClUint32T       rmdFlags = CL_RMD_CALL_ATMOST_ONCE;
    ClRmdOptionsT   rmdOptions = {0};
    ClUint32T noOfRetries = 0;
    ClBoolT bRetry = CL_FALSE;
    ClRcT rc = CL_OK;

    rmdOptions.timeout = COR_RMD_DFLT_TIME_OUT;
    rmdOptions.retries = COR_RMD_DFLT_RETRIES;
    rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;

    do 
    {
        rc = clCorSlaveIdGet(&corSlaveNodeAddr);
        if (rc != CL_OK)
        {
            clLogError("CLS", "SYN", "Failed to get the slave cor node address. rc [0x%x]", rc);
            return rc;
        }

        if (!corSlaveNodeAddr)
            break;

        destAddr.iocPhyAddress.nodeAddress = corSlaveNodeAddr;
        destAddr.iocPhyAddress.portId = CL_IOC_COR_PORT;

        rc = clRmdWithMsg(destAddr, funcId, inMsgHandle, 0, rmdFlags & (~CL_RMD_CALL_ASYNC), 
                &rmdOptions, NULL);
        bRetry = clCorHandleRetryErrors(rc);

        if (bRetry)
        {
            clLogNotice("COR", CL_LOG_CONTEXT_UNSPECIFIED, "Cor Server has returned try again error. So retrying..");
        }
    } while (bRetry && noOfRetries++ < CL_COR_TRY_AGAIN_MAX_RETRIES);

    return rc;
}
