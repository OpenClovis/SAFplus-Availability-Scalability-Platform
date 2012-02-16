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
 * File        : clCorDbgCmdUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * Implements debug command utilities. 
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clCorApi.h>
#include <clEoApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <ctype.h>
#include <clCorClient.h>
#include <clBitApi.h>
#include <clCorObj.h>

/* cor internal files  */
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorPvt.h"
#include "clCorNiIpi.h"
#include "clCorClientDebug.h"

#define CL_CLI_STR_LEN 1024*65
#define CL_COR_CLI_MAX_STR_LEN 256

extern ClCharT* clCorServiceIdString[];

static ClRcT clCorDbgGetCallback( CL_IN ClCorBundleHandleT sessionHandle, 
                                  CL_IN ClPtrT userArg);
static ClRcT 
clCorDbgBundleJobAdd ( CL_IN ClCorBundleHandleT sessionHandle, 
                            CL_IN ClCorMOIdT* pMoId,
                            CL_IN ClCorDbgBundleDataPtrT pTempData,
                            CL_IN ClBufferHandleT * pMsg);

extern ClRcT corObjTreeShowDetails(ClCorMOIdPtrT from, ClBufferHandleT *pMsgHdl);
extern ClRcT  corMOTreeClassGet(ClCorMOClassPathPtrT path, 
                                 ClCorMOServiceIdT srvcId,
                                  ClCorClassTypeT* classId);

extern ClRcT objHandle2DMHandle(ClCorObjectHandleT oh, DMObjHandle_h dmh);
extern ClInt64T corAtoI(ClCharT *str);
extern void clCorMoIdGetInMsg(ClCorMOIdPtrT pAttrPath, ClBufferHandleT* pMsg );
extern void * dmObjectAttrConvert(void *buf, ClInt64T value, CORAttrInfo_t attrType, ClUint32T size);

extern ClCntHandleT corDbgBundleCnt;
extern ClCorInitStageT   gCorInitStage;

static ClInt32T 
getWord(ClCharT *name, ClCharT *word, ClInt32T *idx)
{
    ClInt32T  i = 0, j = 0;

    clDbgIfNullReturn(name, CL_CID_COR);

    clDbgIfNullReturn(word, CL_CID_COR);

    clDbgIfNullReturn(idx, CL_CID_COR);

    j = *idx ;
    clLogTrace( "CLI", "GWD", "The index is [%d]", *idx);

    /**
     *  Function which reject all the entries untill specified string and give
     *  the lenth of string rejected from the start.
     */
    i = strcspn(&name[j], "\\"); 

    clLogTrace( "CLI", "GWD", "Got the length of subsection is [%d]", i);

    /* if it is the end of the string */
    if(0 == i)
        return 0;

    /* Copy the class and instance into the word. */
    memcpy(word, &name[j], i);

    /* End the string with '\0' character */
    word[i]='\0';

    /* Update the value of idx */
    *idx += i;

    /* If there are still some more entries of class and instance then ..*/
    if(name[*idx] == '\\')
        (*idx)++;

    clLogTrace( "CLI", "GWD", "Got the word as [%s] and index is [%d]", word, *idx);

    /* Success */
    return (1);
}

static ClInt32T 
getInstance(ClCharT *word, ClInt32T *inst)
{
    ClInt32T j = 0;

    clDbgIfNullReturn(word, CL_CID_COR);
    
    clDbgIfNullReturn(inst, CL_CID_COR);
   
    for(j=0; j < strlen(word); j++)
    {
        if(word[j] == ':')
        {
            if (word[j+1] && word[j+1] == '*')
            {
                *inst = CL_COR_INSTANCE_WILD_CARD;
                word[j]= '\0';
                return 0;
            }
            else 
                *inst = corAtoI(&word[j+1]);
                word[j]= '\0';
            return 0;
        }
    }
    
    return -1;
}

/*************************************************************************/

ClRcT
corXlatePath(ClCharT *path, ClCorMOClassPathPtrT cAddr)
{
    ClInt32T  widx;
    ClCharT wrd[CL_COR_CLI_MAX_STR_LEN];
    ClInt32T  aidx;
    ClCorClassTypeT tkey;
    ClRcT rc = CL_OK;
    /* int  inst; */

    if( NULL == path )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    widx = 0;
    aidx = 0;
    clCorMoClassPathInitialize (cAddr);
    if (path[widx] == '\\')
        widx = 1;
    while(getWord(path, wrd, &widx) != 0)
    {
        if(wrd[0] && !(isdigit(wrd[0])))
        {
            if(CL_OK == (rc = _corNiNameToKeyGet(wrd, &tkey)))
            {
                   cAddr->node[aidx] = tkey;
                   cAddr->depth++;
                   aidx++;
            }
            else{
                clLogTrace("DBG", "CLI", "Invalid Class Name : [%s]", wrd);
                return rc;
            }
        }
        else
        {
           tkey = corAtoI(wrd);
           cAddr->node[aidx] = tkey;
           cAddr->depth++;
           aidx++;
        }
    }
    if (aidx)
        return(CL_OK);
    else
        return(!(CL_OK));
}

ClRcT
corXlateMOPath(ClCharT *path, ClCorMOIdPtrT cAddr)
{
    ClInt32T  widx;
    ClCharT wrd[CL_COR_CLI_MAX_STR_LEN] = {0};
    ClInt32T  aidx;
    ClCorClassTypeT tkey;
    ClInt32T  inst;
    ClInt32T k = 0;
    ClRcT rc = CL_OK;

    clLogTrace( "CLI", "XMO", "Entered function [%s]", __FUNCTION__);

    clDbgIfNullReturn(path, CL_CID_COR);
    
    clDbgIfNullReturn(cAddr, CL_CID_COR);

    widx = 0;
    aidx = 0;
    clCorMoIdInitialize (cAddr);
    if(path[0] != '\\')
    {
        cAddr->qualifier = CL_COR_MO_PATH_RELATIVE;
    }

    clLogTrace( "CLI", "XMO", "The MO path to translate is [%s]", path);

    if (path[widx] == '\\')
        widx = 1;

    while(getWord(path, wrd, &widx) != 0)
    {
        inst = 0;
        k = getInstance(wrd, &inst);
        if(k == -1)
        {
            clLogError( "CLI", "XMO", "Failed to get the instance value for MO name[%s] ", wrd);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        clLogTrace( "CLI", "XMO", "For the MO [%s] the instance is [%d], widx[%d] ", wrd, inst, widx);

        /* Take care of wildcard class type */
        if( (wrd[0] != '*'))
        {
            if(wrd[0] != '\0')
            {
                if(!(isdigit(wrd[0])))
                {
                    clLogTrace( "CLI", "XMO", "Contains string in the MoId");
                    rc = _corNiNameToKeyGet(wrd, &tkey);
                    if (rc == CL_OK)
                    {
                        clLogTrace( "CLI", "XMO", "Got the Class id [0x%x] for class name [%s]", tkey, wrd);

                        cAddr->node[aidx].type     = tkey;
                        cAddr->node[aidx].instance = inst;
                        cAddr->depth++;
                        aidx++;
                    }
                    else
                    {
                        clLogTrace( "CLI", "XMO", "Failed to get the classId for \
                                classname [%s]. rc[0x%x]", wrd, rc);
                        return rc;
                    }
                }
                else
                {
                    tkey = corAtoI(wrd);
                    cAddr->node[aidx].type     = tkey;
                    cAddr->node[aidx].instance = inst;
                    cAddr->depth++;
                    aidx++;

                    clLogTrace( "CLI", "XMO", "The MoId contains digits,  \
                            ClassId [0x%x], for class Name [%s] ", tkey, wrd);
                }
            }
            else
            {
                clLogTrace("CLI","XMO", "The class name string is empty [%s]", wrd);
                return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
            }

        }
        else
        {
            clLogTrace( "CLI", "XMO", "The MoId contains WildCarded entries");

            /* This is a wildcard class type. Do not get it from NI table */
            cAddr->node[aidx].type = CL_COR_INSTANCE_WILD_CARD;
            cAddr->node[aidx].instance = inst;
            cAddr->depth++;
            aidx++;
        }
        memset(wrd, 0, sizeof(wrd));
    }

    clLogTrace( "CLI", "XMO","Leaving the function [%s]", __FUNCTION__);
    if (aidx)
        return(CL_OK);
    else
        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
}

ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh,  ClNameT *moIdName)
{
    ClRcT rc = CL_OK;
    ClCorInstanceIdT inst = -1;
    ClCorClassTypeT  clsId = -1;
    ClCharT className[CL_MAX_NAME_LENGTH] = {'\0'};
    ClCharT objInstance[CL_MAX_NAME_LENGTH] = {'\0'};
    ClInt16T depth = -1;
    char* nameBuf = moIdName->value;
    ClUint32T i = 0, level = 0;
    ClCorMOIdPtrT pMoId = NULL;

    rc = clCorMoIdAlloc(&pMoId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for the MoId. rc [0x%x]", rc));
        return rc;
    }

    memcpy(pMoId, moIdh, sizeof(ClCorMOIdT));
    
    clCorMoIdServiceSet(pMoId, CL_COR_INVALID_SVC_ID);
    depth = clCorMoIdDepthGet(pMoId);
    if (depth < 0)
    {
        clLogError("CLI", "MNG", 
                "The moId depth obtained is negative ");
        clCorMoIdFree(pMoId);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
    
    level = depth;

    memset(nameBuf, 0, CL_MAX_NAME_LENGTH);
    for(i = 0; i < level; i++ )
    {
        /* get classId*/
        clsId = pMoId->node[i].type;
        if(clsId == CL_COR_CLASS_WILD_CARD)
           strcpy(className, "*");  
        else if((rc = _corNiKeyToNameGet(clsId, className)) != CL_OK)
        {
            clCorMoIdFree(pMoId);
            return (rc);
        }

        /* get instanceId */
        inst = pMoId->node[i].instance;
        if(inst == CL_COR_INSTANCE_WILD_CARD)
           strcpy(objInstance, "*");  
        else
           sprintf(objInstance,"%d",inst);
        
        /* put the level seperator */
        if(strlen(nameBuf) + strlen("\\") < sizeof(moIdName->value))
            strncat(nameBuf, "\\", (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "MNG", "Adding the class name [%s] would exceed the \
                    array length of MoId Name [%s]. So breaking here...", className, nameBuf);
            break;
        }

        /* append class name */
        if(strlen(nameBuf) + strlen(className) < sizeof(moIdName->value))
            strncat(nameBuf, className, (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "MNG", "Adding the class name [%s] would exceed the \
                    array length of MoId Name [%s]. So breaking here...", className, nameBuf);
            break;
        }

        /* put the delimiter */
        if(strlen(nameBuf) + 1 < sizeof(moIdName->value))
            strncat(nameBuf, ":", (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "MNG", "Adding the delimiter(:) in the moid name would exceed the \
                    moId name array [%s]. So breaking here. ", nameBuf);
            break;
        }
        /* append instance  */
        if(strlen(nameBuf) + strlen(objInstance) < sizeof(moIdName->value))
            strncat(nameBuf, objInstance, (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "MNG", "Adding the object instance [%s] would exceed the MOId name \
                    array length for name[%s]", objInstance, moIdName->value);
            break;
        }

        /* set to 0 again */
        memset(objInstance, 0, CL_MAX_NAME_LENGTH);
        memset(className, 0, CL_MAX_NAME_LENGTH);
    }

    /* update the length in moIdName */
    moIdName->length = strlen(nameBuf);

    clLogDebug("CLI", "MNG", "The MOId name [%s] obtained is of length [%d]", moIdName->value, moIdName->length);

    clCorMoIdFree(pMoId);
    return CL_OK;
}
 
                                  // coverity[pass_by_value]
ClRcT _clCorAttrPathToAttrNameGet(ClCorMOIdT moIdh, 
                                  ClCorAttrPathT attrPath,  
                                  ClNameT *moIdName)

{
    ClRcT rc = CL_OK;
    ClCorInstanceIdT inst = -1;
	ClCorAttrPathPtrT pAttrPath = NULL;
    ClCorClassTypeT  clsId = -1, contClassId = -1;
    ClCharT className[CL_MAX_NAME_LENGTH] = {'\0'};
    ClCharT objInstance[CL_MAX_NAME_LENGTH] = {'\0'};
    ClInt16T depth = -1;
    char* nameBuf = moIdName->value;
    ClUint32T i = 0, level = 0;
	CORClass_h pClass;
	CORAttr_h  pContAttrHdl;


	clCorAttrPathClone(&attrPath, &pAttrPath);

    depth = clCorAttrPathDepthGet(pAttrPath);
    if (depth < 0)
    {
        clLogError("CLI", "APG", "The depth obtained is negtive");
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }
    level = depth;
    
	/*     clsId = moIdh->node[i].type; */

	if(moIdh.svcId != CL_COR_INVALID_SVC_ID)
    {
		rc = clCorMoIdToClassGet(&moIdh, CL_COR_MSO_CLASS_GET, &clsId);
        if (CL_OK != rc)
        {
            clLogError("CLI", "APG", 
                    "Failed to get the class for the MO. rc[0x%x]", rc);
            return rc;
        }
    }
	else
		clsId = moIdh.node[moIdh.depth-1].type;	

    memset(nameBuf, 0, CL_MAX_NAME_LENGTH);
    for(i = 0; i < level; i++ )
    {
        /* get classId*/
        if(clsId == CL_COR_CLASS_WILD_CARD)
           strcpy(className, "*");  
        else if((rc = _corNIAttrNameGet(clsId, pAttrPath->node[i].attrId, className)) != CL_OK)
           return (rc);

		pAttrPath->depth = i;
		dmAttrPathToClassIdGet(clsId, pAttrPath, &contClassId); 
		pClass = dmClassGet(clsId);
		pContAttrHdl = dmClassAttrGet(pClass, pAttrPath->node[i].attrId);
		clsId = pContAttrHdl->attrType.u.remClassId;

        /* get instanceId */
        inst = pAttrPath->node[i].index;
        if(inst == CL_COR_INSTANCE_WILD_CARD)
           strcpy(objInstance, "*");  
        else
           sprintf(objInstance,"%d",inst);
        
        /* put the level seperator */
        if(strlen(nameBuf) + strlen("\\") < sizeof(moIdName->value))
            strncat(nameBuf, "\\", (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "ANG", "Adding the level saperator would exceed the \
                    array length of MoId Name [%s]. So breaking here...", nameBuf);
            break;
        }


        /* append class name */
        if(strlen(nameBuf) + strlen(className) < sizeof(moIdName->value))
            strncat(nameBuf, className, (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "ANG", "Adding the attribute name [%s] would exceed the \
                    array length of MoId Name [%s]. So breaking here...", className, nameBuf);
            break;
        }

        /* put the delimiter */
        if(strlen(nameBuf) + 1 < sizeof(moIdName->value))
            strncat(nameBuf, ":", (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "ANG", "Adding the delimiter(:) in the moid name would exceed the \
                    moId name array [%s]. So breaking here. ", nameBuf);
            break;
        }

        /* append instance  */
        if(strlen(nameBuf) + strlen(objInstance) < sizeof(moIdName->value))
            strncat(nameBuf, objInstance, (CL_MAX_NAME_LENGTH-strlen(nameBuf)-1));
        else
        {
            clLogDebug("CLI", "ANG", "Adding the object instance [%s] would exceed the MOId name \
                    array length for name[%s]", objInstance, moIdName->value);
            break;
        }

        /* set to 0 again */
        memset(objInstance, 0, CL_MAX_NAME_LENGTH);
        memset(className, 0, CL_MAX_NAME_LENGTH);
    }
    /* update the length in moIdName */
       moIdName->length = strlen(nameBuf);
    return CL_OK;
}
ClRcT
corXlateAttrPath(ClCorClassTypeT cls, ClCharT *path, ClCorAttrPathPtrT cAddr)
{
    ClInt32T  widx = 0;
    /*   int  i = 0; */
    ClCharT wrd[CL_COR_CLI_MAX_STR_LEN] = {0};
    ClInt32T  aidx = 0;
    ClCorClassTypeT tkey = 0;
    ClInt32T  inst = 0;
    ClCorClassTypeT clsId = cls;
    CORClass_h      clsHdl = NULL;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    widx = 0;
    aidx = 1;
    clCorAttrPathInitialize (cAddr);

    if (path[widx] == '\\')
        widx = 1;
    
    while(getWord(path, wrd, &widx))
    {
        ClInt32T k = 0;
        inst = 0;
        k = getInstance(wrd, &inst);

        if(k == -1)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        }

        if(wrd[0] && !(isdigit(wrd[0])))
        {
        /* The attribute might be a inherited attribute, so try getting name from parent class */
           clsHdl = dmClassGet(clsId);       
           while((rc = _corNIAttrKeyGet(clsHdl->classId, wrd, &tkey))!= CL_OK &&
                       clsHdl->superClassId != 0)
           {
                 clsHdl = dmClassGet(clsHdl->superClassId);       
           }
        }
        else
        tkey = corAtoI(wrd);
      
        if (rc != CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY)) /*CL_COR_SET_RC(CL_COR_UTILS_ERR_MEMBER_NOT_FOUND))*/
        { 
            if(aidx)
            {
              cAddr->node[aidx-1].attrId     = tkey;
              cAddr->node[aidx-1].index      = inst;
              cAddr->depth++;
              /* Get next class Id for cAddr */
              dmAttrPathToClassIdGet(clsId, cAddr, &clsId);
            }
            aidx++;
        }
    }
    if (aidx)
        return(CL_OK);
    else
        return(!(CL_OK));
}


ClRcT
clCorMOClassCreateByName(ClCharT *path, ClInt32T maxInstances)
{
    ClCorMOClassPathT cAddr;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = corXlatePath(path, &cAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MOC", "Invalid MO path specified. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMOClassCreate(&cAddr, maxInstances);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MOC", "Failed to create the MO class. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT
clCorMSOClassCreateByName(ClCharT *path, ClInt32T srvcId, ClInt32T classType)
{
    ClCorMOClassPathT cAddr;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        clLogError("CLI", "MSC", "NULL pointer specified. rc [0x%x]", rc);
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    rc = corXlatePath(path, &cAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MSC", "Failed to translate the MO path. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMSOClassCreate(&cAddr, srvcId, classType);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MSC", "Failed to create the MSO class. rc [0x%x]", rc);
        return rc;
    }
    
    return CL_OK;
}

ClRcT
clCorMOClassDeleteNodeByName(ClCharT *path)
{
    ClCorMOClassPathT cAddr;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        clLogError("CLI", "MOD", "NULL pointer specified.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); 
    }
    
    rc = corXlatePath(path, &cAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MOD", "Failed to translate the MO class path. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMOClassDelete(&cAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MOD", "Failed to delete the MO class. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT
clCorMSOClassDeleteByName(ClCharT *path, ClInt32T serviceId)
{
    ClCorMOClassPathT cAddr;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        clLogError("CLI", "MSD", "NULL pointer specified.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = corXlatePath(path, &cAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MSD", "Failed to translate the MSO class path. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMSOClassDelete(&cAddr, serviceId);
    if (rc != CL_OK)
    {
        clLogError("CLI", "MSD", "Failed to delete the MSO class. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT
corObjCreateByName(ClCharT *path, ClInt32T srvcId)
{
    ClRcT           rc = CL_OK;
    ClCorMOIdT         mAddr;
    ClCorObjectHandleT objHandle;

    if( NULL == path )
    {
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
    clCorMoIdInitialize(&mAddr);
    rc = corXlateMOPath(path, &mAddr);
    if (CL_OK == rc)
    {
        rc = clCorMoIdServiceSet(&mAddr, srvcId);
        if (CL_OK != rc)
        {
            clLogError("CLI", "CRE", "Failed while setting the service Id of the MOID. rc[0x%x]", rc);
            return rc;
        }

        rc = clCorObjectCreate(CL_COR_SIMPLE_TXN, &mAddr, &objHandle);
        if(CL_OK != rc)
        {
            clLogError("CLI", "CRE", "Failed while creating the object [%s: [%d]. rc[0x%x]", path, srvcId, rc);
            return rc;
        }
    }
    else
        clLogError("CLI", "CRE", "Failed while converting the string[%s] to MOID. rc[0x%x]", path, rc);

    return rc;
}

ClRcT
corObjDeleteByName(ClCharT *path, ClInt32T srvcId)
{
    ClCorMOIdT         mAddr;
    ClCorObjectHandleT objHandle;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        clLogError("CLI", "DEL", "NULL pointer passed");
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clCorMoIdInitialize(&mAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "DEL", "Failed to initialize the MoId. rc [0x%x]", rc);
        return rc;
    }

    rc = corXlateMOPath(path, &mAddr);
    if (rc != CL_OK)
    {
        clLogError("CLI", "DEL", "Failed to Xlate the MO Path. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMoIdServiceSet(&mAddr, srvcId);
    if (rc != CL_OK)
    {
        clLogError("CLI", "DEL", "Failed to set the service-id in the MoId. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorObjectHandleGet(&mAddr, &objHandle);
    if (rc != CL_OK)
    {
        clLogError("CLI", "DEL", "Failed to get the object handle. rc [0x%x]", rc);
        return rc;
    }
    
    rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, objHandle);
    if (rc != CL_OK)
    {
        clLogError("CLI", "DEL", "Failed to delete the object. rc [0x%x]", rc);
        return rc;
    }

    return (CL_OK);
}

ClRcT corSubTreeDeleteByName(ClCharT* moPath)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;
    ClUint32T idx = 0;

    CL_FUNC_ENTER();

    if (moPath == NULL)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Input pointer is NULL.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clCorMoIdInitialize(&moId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Failed to initialize the moId. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = corXlateMOPath(moPath, &moId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Failed to Xlate the MoId string to value. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    
    for (idx=0; idx < moId.depth; idx++)
    {
        if ((moId.node[idx].type == CL_COR_CLASS_WILD_CARD) || (moId.node[idx].instance == CL_COR_INSTANCE_WILD_CARD))
        {
            clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Cor Subtree delete with wild card MoId is not supported.");
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
        }
    }
    
    rc = _clCorSubTreeDelete(moId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_DEBUG, "CLI", "OBT", "Failed to delete the Sub Tree. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
corObjAttrSetByName(ClCharT *path, ClInt32T srvcId, ClCharT *attrP , ClInt32T attrId, ClInt32T index, ClInt64T usrValue,
ClBufferHandleT *pMsg)
{
    ClCorMOIdT         mAddr;
    ClCorAttrPathT     attrPath;
    ClCorAttrPathT   *pAttrPath = NULL;
    ClCorObjectHandleT objHandle;
    CORAttr_h attrH = NULL;
    DMContObjHandle_t dmContObjhdl;
    ClUint32T size = sizeof(ClInt64T);
    ClCharT corStr[CL_COR_CLI_STR_LEN]={0};
    void *value = &usrValue;
    ClRcT rc = CL_OK;

    if( NULL == path )
    {
        corStr[0]='\0';
        sprintf(corStr, "Input pointer is NULL");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        clLogError("CLI", "SET", "NULL pointer specified.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorMoIdInitialize(&mAddr);
    if (rc != CL_OK)
    {
        corStr[0] = '\0';
        sprintf(corStr, "Failed to initialize the MoId. rc [0x%x]", rc);
        clBufferNBytesWrite (*pMsg, (ClUint8T *) corStr, strlen(corStr));
        clLogError("CLI", "SET", "Failed to initialize the MoId. rc [0x%x]", rc);
        return rc;
    }
    
    rc = corXlateMOPath(path, &mAddr);
    if ( CL_OK == rc )
    {
        clCorMoIdServiceSet(&mAddr, srvcId);
        
//        rc = _clCorObjectHandleGet(&mAddr,&objHandle);
        rc = _clCorObjectValidate(&mAddr);
        if(CL_OK != rc)
        {
            corStr[0]='\0';
            sprintf(corStr, "\nERROR: Invalid MoId passed. !!!!!");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                   strlen(corStr));
            clLogError("CLI", "SET", "Failed to get the object handle. rc [0x%x]", rc);
            return rc;
        }
    }
    else
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed to translate  moId");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        clLogError("CLI", "SET", "Failed to translate the MoId. rc [0x%x]", rc);
        return rc;
    }
 
    if(attrP)
    {
        ClCorClassTypeT  classId;
        if(srvcId != CL_COR_INVALID_SRVC_ID)
        {
            ClCorMOClassPathT  tmpClassPath;
            
            clCorMoClassPathInitialize(&tmpClassPath);
            clCorMoIdToMoClassPathGet(&mAddr, &tmpClassPath);
            corMOTreeClassGet (&tmpClassPath, srvcId, &classId);
        }
        else
            classId = mAddr.node[mAddr.depth-1].type;
         
        rc = corXlateAttrPath(classId, attrP, &attrPath);
        if(CL_OK == rc)
        {
            pAttrPath = &attrPath;
        }
        else
        {
            corStr[0]='\0';
            sprintf(corStr, "\nFailed to translate Attr Path. rc [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
            clLogError("CLI", "SET", "Failed to translate Attr Path. rc [0x%x]", rc);
            return rc;
        }
    }

   /* Get the size of attribute and pass that only.*/

   /* why? Setting the value at specific index of array, requires exact size of the array data type.
      and if size passed is larger than array data type, cor assumes its a bulk set.
      (we may end up setting few more indexes.)
      Default size of value for cli is 64-bit, which creates problems when array data type
      is not 64-bit.
    */
         
    /* Get the DMHandle */
	rc = moId2DMHandle(&mAddr, &dmContObjhdl.dmObjHandle);
    if (CL_OK != rc)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed to get the DM handle. rc [0x%x]", rc);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        return rc;
    }

    /* Set the cont path.*/ 
    dmContObjhdl.pAttrPath = pAttrPath;

    attrH = dmObjectAttrTypeGet(&dmContObjhdl, attrId);
    if(attrH && attrH->attrType.type == CL_COR_ARRAY_ATTR)
    {
        ClUint32T items = attrH->attrValue.min;
        size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);    
        size = size/items;

        /* Check if its BIG ENDIAN  */
        if(clByteEndian == CL_BIT_BIG_ENDIAN)
        {
            value = (ClUint8T *) value + (sizeof(ClInt64T) - size);
        }
    }

    if(attrH)
    {
        rc = clCorMoIdToObjectHandleGet(&mAddr, &objHandle);
        if (rc != CL_OK)
        {
            corStr[0]='\0';
            sprintf(corStr, "Failed to get the object handle. rc [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        }

        if((rc = clCorObjectAttributeSet(CL_COR_SIMPLE_TXN, objHandle, pAttrPath,
                               attrId, index, value, size)) != CL_OK)
        {
            corStr[0]='\0';
            sprintf(corStr, "Execution Result : Failed [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        }
        else
        {
            corStr[0]='\0';
	        sprintf(corStr, "Execution Result : Passed");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        }

        /*
         * Free the object handle.
         */
        clCorObjectHandleFree(&objHandle);
    }
    else
    {
        corStr[0]='\0';
        sprintf(corStr, "Execution Result : Failed [0x%x]", CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT));
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        rc = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    }

    return rc;
}


/**
 *  Function which return the size of the basic datatype.
 */

ClInt32T
clCorTestBasicSizeGet(ClUint32T type)
{
    switch(type)
    {
        case CL_COR_UINT8:
        case CL_COR_INT8:
                return 1;
        break;
        case CL_COR_UINT16:
        case CL_COR_INT16:
                return 2 ;
        break;
        case CL_COR_UINT32:
        case CL_COR_INT32:
                return 4 ;
        break;
        case CL_COR_UINT64:
        case CL_COR_INT64:
                return 8 ;
        break;
        default:
            clOsalPrintf("\n Invalid type for getting the size");
    } 
    return -1;
}

/**
 *  Function to add display proper value based on the type.
 */

void
_clCorDisplayInfoAppend(ClCorDbgBundleDataPtrT pBundleData, ClBufferHandleT bufH)
{
    ClInt32T    noOfItems = 1;
    ClUint8T    *data     = NULL;
    ClCharT     corStr[CL_COR_CLI_STR_LEN] = {0};
    ClCorTypeT  type    = CL_COR_INVALID_DATA_TYPE; 


    if((pBundleData == NULL) || (pBundleData->data == NULL))
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
    else
    {
        type = pBundleData->attrType;
        data = pBundleData->data;

        sprintf(corStr, "\n AttrId [0x%x], Value : [", pBundleData->attrId);
        clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));

        while((noOfItems--) > 0)
        {
            corStr[0] = '\0';
            switch((ClInt32T)type)
            {
                case CL_COR_UINT8:
                case CL_COR_INT8:
                {
                    sprintf(corStr,"%d", *(ClUint8T *) data);
                    clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
                    data = data + sizeof(ClUint8T);
                }
                break;
                case CL_COR_UINT16:
                case CL_COR_INT16:
                {
                    sprintf(corStr,"%d", *(ClUint16T *) data);
                    clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
                    data = data + sizeof(ClUint16T);
                }
                break;
                case CL_COR_UINT32:
                case CL_COR_INT32:
                {
                    sprintf(corStr,"%d", *(ClUint32T *) data);
                    clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
                    data = data + sizeof(ClUint32T);
                }
                break;
                case CL_COR_UINT64:
                case CL_COR_INT64:
                {
                    sprintf(corStr,"%lld", *(ClUint64T *) data);
                    clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
                    data = data + sizeof(ClUint64T);
                }
                break;
                case CL_COR_ARRAY_ATTR:
                {
                    if(pBundleData->index == CL_COR_INVALID_ATTR_IDX)
                        noOfItems = pBundleData->size / 
                            clCorTestBasicSizeGet(pBundleData->arrType);
                    else
                        noOfItems = 1;

                    type = pBundleData->arrType;

                }
                continue;
                default:
                    sprintf(corStr, "Invalid type for this data value.");
                    clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
            } 

            if (noOfItems > 0)
            {
                sprintf(corStr, ",");
                clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
            }
        }
        
        sprintf(corStr, "]");
        clBufferNBytesWrite(bufH, (ClUint8T *)corStr, strlen(corStr));
    } 

    return;
}

/**
 * Function to get the attribute value.
 */
ClRcT 
corObjAttrGetByName(ClCharT *path, ClInt32T srvcId, ClCharT *attrP, ClInt32T attrId,  ClInt32T Index, ClInt64T *usrValue, ClBufferHandleT *pMsg)
{
    ClRcT              rc   = CL_OK;
    ClCorMOIdT         mAddr;
    ClCorAttrPathT     attrPath;
    ClCorAttrPathT    *pAttrPath = NULL;
    ClCorObjectHandleT objHandle;
    DMContObjHandle_t  dmContObjhdl;
    CORAttr_h          attrH = NULL;
    ClUint32T size = sizeof(ClInt64T);
    ClCharT corStr[CL_COR_CLI_STR_LEN]={0};
    ClCorDbgBundleDataT dbgBundleData = {0};             
    ClCorAttrValueDescriptorT attrDesc = {0};
    ClCorAttrValueDescriptorListT attrList = {0};

    if( NULL == path )
    {
        corStr[0]='\0';
        sprintf(corStr, "Input pointer is NULL");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    clCorMoIdInitialize(&mAddr);

    rc = corXlateMOPath(path, &mAddr);
    if (rc == CL_OK)
    {
        clCorMoIdServiceSet(&mAddr, srvcId);
        if(CL_OK != (rc = _clCorObjectValidate(&mAddr)))
        {
            corStr[0]='\0';
            sprintf(corStr, "\nERROR: Invalid MoId passed. !!!!!. rc [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return rc;
        }
    }
    else
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed to translate the moId. rc [0x%x]", rc);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        return rc;
    }

    if(attrP)
    {
        ClCorClassTypeT  classId;
        
        if(srvcId != CL_COR_INVALID_SRVC_ID)
        {
            ClCorMOClassPathT  tmpClassPath;
            
            clCorMoClassPathInitialize(&tmpClassPath);
            clCorMoIdToMoClassPathGet(&mAddr, &tmpClassPath);
            corMOTreeClassGet (&tmpClassPath, srvcId, &classId);
        }
        else
            classId = mAddr.node[mAddr.depth-1].type;
        
        rc = corXlateAttrPath(classId, attrP, &attrPath);
        if (rc == CL_OK)
        {
            pAttrPath = &attrPath;
        }
        else
        {
            corStr[0]='\0';
            sprintf(corStr, "\nFailed to translate Attr Path. rc [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return rc;
        }
    }

    /* Get the size of attribute and pass that only.*/

    /* Get the DMHandle */

//      objHandle2DMHandle(objHandle, &(dmContObjhdl.dmObjHandle));
      rc = moId2DMHandle(&mAddr, &(dmContObjhdl.dmObjHandle));
      if (rc != CL_OK)
      {
            ClCharT moIdstr[CL_MAX_NAME_LENGTH] = {0};

            corStr[0]='\0';
            clLogError("DM", "ATTRGET", "Failed to get DM handle from MoId [%s]. rc [0x%x]", 
                _clCorMoIdStrGet(&mAddr, moIdstr), rc);
            sprintf(corStr, "Failed to convert moId into DM handle. rc [0x%x]", rc);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return rc;
      }

      /* Set the cont path.*/ 
      dmContObjhdl.pAttrPath = pAttrPath;

      attrH = dmObjectAttrTypeGet(&dmContObjhdl, attrId);
 
      if(attrH)
        size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);    

      if(attrH && attrH->attrType.type == CL_COR_ARRAY_ATTR)
      {
         ClUint32T items = attrH->attrValue.min;
         if(CL_COR_INVALID_ATTR_IDX != Index)
            size = size/items;
      }
 
      if(attrH)
      {
            dbgBundleData.attrType = attrH->attrType.type;
            dbgBundleData.arrType = attrH->attrType.u.arrType;
            dbgBundleData.attrId = attrH->attrId;
            dbgBundleData.size = size;
            dbgBundleData.index = Index;

            dbgBundleData.data = clHeapAllocate(size);
            if(dbgBundleData.data == NULL)
            {
                corStr[0]='\0';
                sprintf(corStr, "\nFailed while allocating the memory for the data. rc [0x%x]", CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
                rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                return rc;
            }

            if(attrH->userFlags & CL_COR_ATTR_CONFIG)
            {
                if((rc = dmObjectAttrGet(&dmContObjhdl, attrId, Index, (void*)dbgBundleData.data, &size)) != CL_OK)
                {
                    corStr[0]='\0';
                    sprintf(corStr, "\nFailed to get attribute. rc [0x%x]", rc);
                    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
                }
                else
                {
                    if(clByteEndian == CL_BIT_LITTLE_ENDIAN)
                    {
                        rc = clCorObjAttrValSwap(dbgBundleData.data, size, 
                                attrH->attrType.type, attrH->attrType.u.arrType);
                        if (CL_OK != rc)
                        {
                            corStr[0]='\0';
                            sprintf(corStr, 
                                    "\nFailed to convert the data into host format . rc [0x%x]", rc);
                            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
                            clHeapFree(dbgBundleData.data);
                            return rc;
                        }
                    }
                    _clCorDisplayInfoAppend(&dbgBundleData, *pMsg);
                }
                clHeapFree(dbgBundleData.data);
            }
            else if((attrH->userFlags & CL_COR_ATTR_RUNTIME) || (attrH->userFlags & CL_COR_ATTR_OPERATIONAL))
            {   
                ClCorBundleHandleT      bundleHandle = 0;
                ClCorDbgGetOpT          *tempGet = NULL;
                ClCorBundleConfigT       bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
                ClTimerTimeOutT          delay = {.tsSec = 0, .tsMilliSec = 0};
                tempGet = clHeapAllocate(sizeof(ClCorDbgGetOpT));
                if(NULL == tempGet)
                {
                    corStr[0]='\0';
                    sprintf(corStr, " ERROR: Failed while allocating memory. rc[0x%x]\n", CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)); 
                    clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
                    return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
                }

                /* Assigning the value */
                memset(tempGet, 0, sizeof(ClCorDbgGetOpT));

                corStr[0]='\0';
                sprintf(corStr, " \n INFO: The attribute get for a non-configuration attribute\n");
                clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));

                rc = clOsalMutexCreate(&tempGet->mutexId);
                if(rc != CL_OK)
                {
                    corStr[0]='\0';
                    sprintf(corStr, " ERROR: Failed while creating mutex. rc[0x%x]\n", rc);
                    clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
                    clHeapFree(tempGet);
                    return rc;
                }
                rc = clOsalCondCreate(&tempGet->condId);
                if(rc != CL_OK)
                {
                    corStr[0] = 0;
                    snprintf(corStr, sizeof(corStr), "ERROR: Failed while creating cond. rc[%#x]\n", rc);
                    clBufferNBytesWrite(*pMsg, (ClUint8T*)corStr, strlen(corStr));
                    clHeapFree(tempGet);
                    return rc;
                }
                tempGet->pMsgHandle = pMsg;

                rc = clCorBundleInitialize(&bundleHandle, &bundleConfig); 
                if(CL_OK != rc) 
                {
                    corStr[0]='\0';
                    sprintf(corStr, " ERROR: Failed while creating bundle. rc[0x%x]\n", rc); 
                    clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
                    goto handleError;
                }

                attrDesc.pAttrPath = pAttrPath;
                attrDesc.attrId = attrId;
                attrDesc.index = Index;
                attrDesc.bufferPtr = dbgBundleData.data;
                attrDesc.bufferSize = size;
                attrDesc.pJobStatus = &tempGet->jobStatus;

                attrList.pAttrDescriptor = &attrDesc;
                attrList.numOfDescriptor = 1;

                rc = clCorMoIdToObjectHandleGet(&mAddr, &objHandle);
                if (rc != CL_OK)
                {
                    corStr[0]='\0';
                    sprintf(corStr, "Failed to get the Object Handle from MoId. rc [0x%x]", rc);
                    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
                    return rc;
                }

                rc = clCorBundleObjectGet(bundleHandle, &objHandle, 
                            &attrList);
                if(CL_OK != rc) 
                {
                    corStr[0]='\0';
                    sprintf(corStr, " ERROR: Failed while queuing the job. rc[0x%x]\n", rc);
                    clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
                    goto handleError;
                }

                clOsalMutexLock(tempGet->mutexId);

                rc = clCorBundleApplyAsync(bundleHandle, 
                            clCorDbgGetCallback, tempGet); 
                if(CL_OK != rc) 
                {
                    corStr[0]='\0';
                    sprintf(corStr, "ERROR: Failed while committing the job. rc[0x%x]\n", rc);
                    clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
                    clOsalMutexUnlock(tempGet->mutexId);
                    goto handleError;
                }

                clOsalCondWait(tempGet->condId, tempGet->mutexId, delay);

                if(tempGet->jobStatus == CL_OK) 
                {
                    _clCorDisplayInfoAppend(&dbgBundleData, *pMsg);
                }
                clOsalMutexUnlock(tempGet->mutexId);
handleError:
                /* Release all the resources */
                clOsalMutexDelete(tempGet->mutexId);
                if(tempGet->condId)
                    clOsalCondDelete(tempGet->condId);
                clHeapFree(tempGet);
                clHeapFree(dbgBundleData.data);
                clCorObjectHandleFree(&objHandle);
            }
            else if(attrH->userFlags & CL_COR_ATTR_OPERATIONAL)
            {
                corStr[0]='\0';
                sprintf(corStr, "\nERROR:  Attribute [0x%x] is an operatinal attribute !! ", attrId);
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
            }
      }
      else
      {
          corStr[0]='\0';
          sprintf(corStr, "\nERROR:  Attribute id 0x%x does not exist !! ", attrId);
          clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                     strlen(corStr));
          rc = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
      }
      
    return rc;
}

/**
 * Callback function for the get operation. 
 *
 */
ClRcT clCorDbgGetCallback( CL_IN ClCorBundleHandleT bundleHandle, 
                           CL_IN ClPtrT userArg)
{
    ClCorDbgGetOpT * tempGet = (ClCorDbgGetOpT *)userArg;
    ClCharT corStr[CL_COR_CLI_STR_LEN]={0};

    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Inside the callback function"));
    clOsalMutexLock(tempGet->mutexId);
    if(tempGet->jobStatus != CL_OK)
    {
        corStr[0] = '\0';
        sprintf(corStr, "Failed to get the attribute information. rc[0x%x]", tempGet->jobStatus); 
        clBufferNBytesWrite (*tempGet->pMsgHandle, (ClUint8T*)corStr, strlen(corStr));
    }
    clOsalCondSignal(tempGet->condId);
    clOsalMutexUnlock(tempGet->mutexId);

    clCorBundleFinalize(bundleHandle);

    return CL_OK;
}


ClRcT
corObjTreeWalkByName(ClCharT *path, ClBufferHandleT * pMsg)
{
    ClCorMOIdT cAddr;
    ClRcT rc = CL_OK;

	if (path == (ClCharT *)NULL)
    {    
        rc = corObjTreeShowDetails(0, pMsg);
    }    
	else
	{
        rc = corXlateMOPath(path, &cAddr);
        if ( rc == CL_OK)
        {
            rc = corObjTreeShowDetails(&cAddr, pMsg);
        }
	}

    return rc;
}

ClRcT
corObjShowByName(ClCorMOIdT* pMoId, ClCorServiceIdT srvcId, ClCharT *cAttrPath, ClBufferHandleT* pMsg)
{
    ClCorAttrPathT attrPath;
    ClCorAttrPathPtrT pAttrPath = &attrPath;
    ClCorObjectHandleT  objHandle;
    DMContObjHandle_t   dmContObjhdl;
    ClCharT corStr[CL_COR_CLI_STR_LEN]={0};
    ClRcT rc = CL_OK;

    if(NULL == pMoId)
    {
         sprintf(corStr, "ERROR: MoId passed is NULL.");
         clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(cAttrPath)
    {
        ClCorClassTypeT  classId;

        if(srvcId != CL_COR_INVALID_SRVC_ID)
        {
            ClCorMOClassPathT  tmpClassPath;

            clCorMoClassPathInitialize(&tmpClassPath);
            clCorMoIdToMoClassPathGet(pMoId, &tmpClassPath);

            rc = corMOTreeClassGet (&tmpClassPath, srvcId, &classId);
            if(CL_OK != rc)
            {
                corStr[0]='\0';
                sprintf(corStr, "\nERROR:  MSO does not exist for service id [ %d ] !!!!", srvcId);
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
                return rc;
            }
        }
        else
            classId = pMoId->node[pMoId->depth-1].type;

        rc = corXlateAttrPath(classId, cAttrPath, pAttrPath);
        if (rc != CL_OK)
        {
            corStr[0]='\0';
            sprintf(corStr, "\n ERROR:  Failed to translate address !!!!");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return rc;
        }
#if 0
			ClNameT name;
			memset(&name, 0 ,sizeof(ClNameT));
			_clCorAttrPathToAttrNameGet(moId, *pAttrPath, &name);
#endif
    }
    else
        pAttrPath = NULL;
 
    /* Get the DMHandle */
//    objHandle2DMHandle(objHandle, &(dmContObjhdl.dmObjHandle));
    rc = moId2DMHandle(pMoId, &(dmContObjhdl.dmObjHandle));
    if (rc != CL_OK)
    {
        corStr[0]='\0';
        sprintf(corStr, "Failed to get DM handle from moId. rc [0x%x]", rc);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        return rc;
    }

    /* Set the cont path.*/ 
    dmContObjhdl.pAttrPath = pAttrPath;

    sprintf(corStr, "\n\n[%s SERVICE OBJECT]\n", clCorServiceIdString[pMoId->svcId]);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));

    rc = clCorMoIdToObjectHandleGet(pMoId, &objHandle);
    if (rc != CL_OK)
    {
        corStr[0]='\0';
        sprintf(corStr, "Failed to get object handle from moId. rc [0x%x]", rc);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        return rc;
    }

    /* Show dm Object */
    rc = dmObjectShow(&dmContObjhdl, *pMsg, objHandle);

    clCorObjectHandleFree(&objHandle);

    return rc;
}

/**
 *  Function to enqueue the attribute get jobs which are part of same bundle.
 */


ClRcT 
_clCorBundleJobEnqueue(ClCorBundleHandleT bundleHandle, ClCharT *path, ClInt32T srvcId, ClCharT *attrP, 
                       ClInt32T attrId, ClInt32T index, ClBufferHandleT *pMsg, 
                       ClCorDbgGetBundleT *pGetBundleInfo, ClCorOpsT opType)
{
    ClRcT                           rc                          = CL_OK;
    ClCorAttrPathT                  *pAttrPath                  = NULL;
    ClCorObjectHandleT              objHandle                   = NULL;
    ClUint32T                       userFlags                   = 0;
    ClCharT                         corStr[CL_COR_CLI_STR_LEN]  = {0};
    ClCorDbgBundleDataPtrT          pTempData                   = NULL;
    ClCorAttrValueDescriptorT       attrDesc                   = {0};
    ClCorAttrValueDescriptorListT   attrList                   = {0};
    ClCorMOIdT                      mAddr;
    ClCorAttrPathT                  attrPath;
    
   
    if(NULL == path) 
    {
        corStr[0]='\0';
        sprintf(corStr, "Moid cannot be NULL. ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    clCorMoIdInitialize(&mAddr);
    clCorAttrPathInitialize(&attrPath);

    if ((rc = corXlateMOPath(path, &mAddr)) == CL_OK)
    {
        clCorMoIdServiceSet(&mAddr, srvcId);
        if(CL_OK != ( rc = _clCorObjectHandleGet(&mAddr,&objHandle)))
        {
            corStr[0]='\0';
            sprintf(corStr, "\nERROR: Invalid MoId passed. !!!!!");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
            return rc;
        }
    }
    else
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed to translate  moId");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
        return rc;
    }

    if(attrP)
    {
        ClCorClassTypeT  classId;

        if(srvcId != CL_COR_INVALID_SRVC_ID)
        {
            ClCorMOClassPathT  tmpClassPath;
            
            clCorMoClassPathInitialize(&tmpClassPath);
            clCorMoIdToMoClassPathGet(&mAddr, &tmpClassPath);
            corMOTreeClassGet (&tmpClassPath, srvcId, &classId);
        }
        else
            classId = mAddr.node[mAddr.depth-1].type;
        
        if ((rc = corXlateAttrPath(classId, attrP, &attrPath)) == CL_OK)
        {
           pAttrPath = &attrPath;
        }
        else
        {
             corStr[0]='\0';
             sprintf(corStr, "\nFailed to translate Attr Path ");
             clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                         strlen(corStr));
             clCorObjectHandleFree(&objHandle);
             return rc;
        }
    }
    
    pTempData = clHeapAllocate(sizeof(ClCorDbgBundleDataT));
    if(NULL == pTempData)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed while allocating the memory ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                     strlen(corStr));
        clCorObjectHandleFree(&objHandle);
        return rc;
    }

    clCorMoIdInitialize(&pTempData->moId);
    clCorAttrPathInitialize(&pTempData->attrPath);    

    pTempData->moId = mAddr;    
    
    pTempData->attrId =  attrId;
    if(pAttrPath != NULL)
        pTempData->attrPath = *pAttrPath;

    /* Get the attribute info */
    rc = clCorObjAttrInfoGet(objHandle, pAttrPath, attrId, &pTempData->attrType, 
                            &pTempData->arrType, &pTempData->size, &userFlags);
    if(rc != CL_OK)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed while getting the attribute information. ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                 strlen(corStr));
        clHeapFree(pTempData);
        clCorObjectHandleFree(&objHandle);
        return rc;
    }
   
    attrDesc.attrId = attrId;
    attrDesc.index = pTempData->index = index;
    attrDesc.pAttrPath = pAttrPath;
    
    
    if((ClInt32T)CL_COR_ARRAY_ATTR == pTempData->attrType)
    { 
        if( CL_COR_INVALID_ATTR_IDX != index)
        {
            pTempData->size = clCorTestBasicSizeGet(pTempData->arrType);
        }
    }
    
    /* Allocate the basic size */
    pTempData->data = clHeapAllocate(pTempData->size);

    if(NULL == pTempData->data)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nFailed while allocating the memory ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                 strlen(corStr));
        clCorObjectHandleFree(&objHandle);
        return rc;
    }

    memset(pTempData->data, 0, pTempData->size);
    
    attrDesc.bufferPtr = pTempData->data;
    attrDesc.bufferSize = pTempData->size;
    attrDesc.pJobStatus = &pTempData->jobStatus;
 
    /* Attribute Descriptor List assignment */
    attrList.numOfDescriptor = 1;
    attrList.pAttrDescriptor = &attrDesc;

    /* If the attribute type is config or runtime , then call the enqueue the jobs. */
    rc = clCorBundleObjectGet(bundleHandle, &objHandle, &attrList);
    if(CL_OK != rc) 
    {
        corStr[0]='\0';
        sprintf(corStr, " ERROR: Failed while queuing the job. rc[0x%x]\n", rc);
        clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
        clCorObjectHandleFree(&objHandle);
        return rc;
    }
   
    /* Add the job in the local container. */
    rc = clCorDbgBundleJobAdd(bundleHandle, &mAddr, pTempData, pMsg); 
    if(CL_OK != rc)
    {
        corStr[0]='\0';
        sprintf(corStr, " ERROR: Failed while queuing the job. rc[0x%x]\n", rc);
        clBufferNBytesWrite(*pMsg, (ClUint8T *)corStr, strlen(corStr));
        clCorObjectHandleFree(&objHandle);
        return rc;
    }
      
    clCorObjectHandleFree(&objHandle);

    return CL_OK;
}

/**
 * Key compare function for all the jobs 
 */
static ClInt32T _clCorDbgBundleJobInfoCmpFn(
    CL_IN       ClCntKeyHandleT         key1,
    CL_IN       ClCntKeyHandleT         key2)
{
    ClCorDbgBundleJobKeyPtrT sessionKey1 = (ClCorDbgBundleJobKeyPtrT )key1;
    ClCorDbgBundleJobKeyPtrT sessionKey2 = (ClCorDbgBundleJobKeyPtrT )key2;

    if(clCorMoIdCompare(&(sessionKey1->moId), &(sessionKey2->moId)) == 0) /*For get/set/ delete */
    {
        if(clCorAttrPathCompare(&(sessionKey1->attrPath), &(sessionKey2->attrPath)) == 0)
        {
            if(sessionKey1->attrId == sessionKey2->attrId)
            {
                return 0;
            }
            else
                return 1;
        }
        else
            return 1;
    }
    else
        return 1;

    return 1;
}

/**
 * Delete callback function
 */
static void _clCorDbgBundleJobInfoCntDelete (
    CL_IN       ClCntKeyHandleT         key,
    CL_IN       ClCntDataHandleT        data)
{
    clHeapFree(((ClCorDbgBundleDataPtrT)data)->data);
    clHeapFree((void*)data);
    clHeapFree((void *)key);

    return ;
}

/**
 *  Function to create the container which will 
 *  store the job related information.
 */

ClRcT
clCorDbgBundleJobCntCreate (
            CL_OUT ClCntHandleT *cntHandle)
{
    ClRcT   rc = CL_OK;

    rc = clCntLlistCreate (_clCorDbgBundleJobInfoCmpFn,
                           _clCorDbgBundleJobInfoCntDelete,
                           _clCorDbgBundleJobInfoCntDelete,
                           CL_CNT_UNIQUE_KEY, 
                           (ClCntHandleT *) cntHandle);
    return rc;
}


/**
 * Function to add the bundle job in the bundle-job container.
 */

ClRcT 
clCorDbgBundleJobAdd ( CL_IN ClCorBundleHandleT bundleHandle, 
                        CL_IN ClCorMOIdT* pMoId, 
                        CL_IN ClCorDbgBundleDataPtrT pTempData,
                        CL_IN ClBufferHandleT * pMsg)
{
    ClRcT                      rc = CL_OK;
    ClCharT                    corStr[CL_COR_CLI_STR_LEN] = {0};
    ClCorDbgGetBundleT         *pCorGetBundleData = NULL;
    ClCorDbgBundleJobKeyPtrT   pBundleJobKey = NULL;

    if(NULL == pTempData)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nERROR: NULL pointer passed.  !! ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        return rc;
    }

    rc = clCntDataForKeyGet(corDbgBundleCnt, (ClCntKeyHandleT)&bundleHandle, (ClCntDataHandleT *)&pCorGetBundleData);
    if(rc != CL_OK)
    {
        corStr[0]='\0';
        sprintf(corStr, "\nERROR: Failed while looking for bundle data !! ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        return rc;
    }

    if(NULL != pCorGetBundleData)
    {
        pBundleJobKey = clHeapAllocate(sizeof(ClCorDbgBundleJobKeyT));
        
        if((NULL == pBundleJobKey))
        {
            corStr[0]='\0';
            sprintf(corStr, "\nERROR:  Failed to Allocate the job/key for storing bundle job.!! ");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pBundleJobKey, 0, sizeof(ClCorDbgBundleJobKeyT));

        pBundleJobKey->moId = (*pMoId);
        pBundleJobKey->attrPath = pTempData->attrPath;
        pBundleJobKey->attrId = pTempData->attrId;

        rc = clCntNodeAdd(pCorGetBundleData->cntHandle, 
                (ClCntKeyHandleT)pBundleJobKey, (ClCntDataHandleT) pTempData, NULL);
        if(CL_OK != rc)
        {
            corStr[0]='\0';
            sprintf(corStr, "\nERROR:  Failed while adding the job in the bundle container. !! ");
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
            return rc;
        }
    }
    else
    {   
        corStr[0]='\0';
        sprintf(corStr, "\nERROR:  NULL pointer for the bundle data for this bundle. !! ");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr, strlen(corStr));
        return rc;
    }

    return rc;
}


/**
 *  Function to work on the attriubute list and get the required information.
 */ 
ClRcT _clCorDbgUtilsCorObjCreateAndSet ( ClCorClientCliOpT cliOp, ClUint32T argc, 
                                        ClCharT * argv[], ClCorCtAndSetInfoPtrT pAttrList )
{
    ClRcT   rc = CL_OK;
    ClUint32T currIndex = 0, index = 0;
    ClInt32T  attrSize = 0;
    ClCorMOIdT moId ;
    CORClass_h classH = NULL;
    ClCorClassTypeT classId = 0;
    CORAttr_h attrH = NULL;
    ClCorServiceIdT serviceId = CL_COR_INVALID_SRVC_ID;
    ClCorAttrPathT attrPath ;
    ClInt64T value = 0;

    clCorAttrPathInitialize(&attrPath);

    clLogInfo("CLI", "EXP", "MoId obtained is [%s]", argv[currIndex]); /* 0 - MoId */

    rc = corXlateMOPath(argv[currIndex], &moId);
    if(CL_OK != rc)
    {
        clLogError( "CLI", "EXP", "Failed to transalte the MO path . rc[0x%x]", rc);
        return rc;
    }

    currIndex++; /* 1 - Service Id */
    serviceId = (ClInt32T ) corAtoI(argv[currIndex]);

    clLogInfo("CLI", "EXP", "Service Id is [%d]", serviceId);

    clCorMoIdServiceSet(&moId, serviceId);
   
    /* Assigning the MoId */
    pAttrList->moId = moId;

    if(cliOp == CL_COR_CLIENT_CLI_ATTR_SET_OP)
    {
        rc = _clCorObjectValidate(&moId);
        if ( CL_OK != rc )
        {
            clLogError ( "CLI", "EXP", "Failed to validate the MO [%s]. rc [0x%x]", argv[0], rc);
            return rc;
        }
    }

#if 0
    rc = corXlateAttrPath(argv[currIndex++], &attrPath);
    if(CL_OK != rc)
    {
        clLog(CL_LOG_ERROR, "CLI", "EXP", "Failed while translating the attrpaht [%s] for MO [%s]. rc[0x%x]",
                argv[2], argv[0], rc);
        return rc;
    }
#endif

   /* Subtracting the moid, service id (and attrPath)  */
   pAttrList->num = (argc - (currIndex + 1 ))/2;
   
   clLogTrace( "CLI", "EXP", "The num of attributes in the list are [%d]", pAttrList->num);

   currIndex++; /*2 - attrId info */

   pAttrList->pAttrInfo =  (_ClCorAttrInfoPtrT) clHeapAllocate(sizeof(_ClCorAttrInfoT) * pAttrList->num); 
   if(NULL == pAttrList->pAttrInfo) 
   {
       clLogTrace( "CLI", "EXP", "Failed to allocate the memory for attrList. ");
       return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
   }

   for ( index = 0; index  < pAttrList->num ; index ++)
   {
        pAttrList->pAttrInfo[index].attrId = (ClUint32T ) corAtoI(argv[currIndex++]);

        clLogTrace("CLI", "EXP", "The attr Id is [0x%x], [%s]", 
                pAttrList->pAttrInfo[index].attrId, argv[currIndex - 1]);

        rc = _corMoIdToClassGet(&moId, &classId);
        if (rc != CL_OK)
        {
            clLogError("CLI", "EXP", "Failed to get the class Id from the MoId");
            return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }

        clLogTrace( "CLI", "EXP", "For MO[%s] , service Id [%d], Class id [0x%x]", 
                argv[0], serviceId, classId);

        /* get the class handle */
        classH = dmClassGet(classId);
        if (classH == NULL)
        {
            clLogError( "CLI", "EXP", "Failed to get the class information for classId [%d]", classId);
            return (CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT));
        }

        /* get the attr handle */
        attrH = dmClassAttrInfoGet(classH, &attrPath, pAttrList->pAttrInfo[index].attrId);
        if (NULL == attrH)
        {
            clLogError( "CLI", "EXP", "Failed to find the attribute [0x%x], \
                    classId [0x%x] for MO [%s], serviceId [%d]", 
                    pAttrList->pAttrInfo[index].attrId, classId, argv[0], serviceId); 
            return (CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_NOT_PRESENT));
        }
        
        attrSize = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);
        if(attrH->attrType.type == CL_COR_ARRAY_ATTR)
            attrSize /= (attrH->attrValue.min * (attrH->attrValue.max > 0 ? attrH->attrValue.max : 1));

        if(attrSize < 0)
        {
            clLogCritical("CLI", "EXP", "The Attribute size for attribute [0x%x] is negative [%d]", 
                    attrH->attrId, attrSize);
            return CL_COR_SET_RC(CL_COR_ERR_INVALID_STATE);
        }

#if 0
        if(attrH->attrType.type != CL_COR_ARRAY_ATTR ||
                attrH->attrType.type != CL_COR_CONTAINMENT_ATTR ||
                attrH->attrType.type != CL_COR_ASSOCIATION_ATTR)
        {
            pAttrList->pAttrInfo[index].attrType = CL_COR_SIMPLE_ATTR;
            pAttrList->pAttrInfo[index].corType  = attrH->attrType.type;
        }
        else
        {
            pAttrList->pAttrInfo[index].attrType = attrH->attrType.type;
            pAttrList->pAttrInfo[index].corType  = attrH->attrType.u.arrType;
        }
#endif

        /**
         *  This COR type assignment can be changed as above. For that the convert 
         *  function on the client side should be changed accordingly. 
         */
        pAttrList->pAttrInfo[index].attrType = attrH->attrType.type;
        pAttrList->pAttrInfo[index].corType  = attrH->attrType.u.arrType;

        if((attrH->attrType.type == CL_COR_ARRAY_ATTR) && 
                (attrH->attrType.u.arrType == (ClInt32T)CL_COR_UINT8 || attrH->attrType.u.arrType == (ClInt32T)CL_COR_INT8))
        {
            attrSize = strlen(argv[currIndex]);

            pAttrList->pAttrInfo[index].pValue = clHeapAllocate(attrSize);
            if(pAttrList->pAttrInfo[index].pValue == NULL)
            {
                clLogError("CLI", "EXP", "Failed while allocating the memory for the \
                        attrId [0x%x], classId[0x%x]", pAttrList->pAttrInfo[index].attrId, classId);
                return rc;
            }

            memset(pAttrList->pAttrInfo[index].pValue, 0, attrSize);
            
            pAttrList->pAttrInfo[index].size = attrSize;
            
            pAttrList->pAttrInfo[index].index = 0;

            memcpy(pAttrList->pAttrInfo[index].pValue, argv[currIndex++], attrSize); 
        }
        else
        {
            if(strncasecmp(argv[currIndex], "0x", 2) == 0)
                value = (ClInt64T) strtoll(argv[currIndex++] , NULL, 16);
            else 
                value = (ClInt64T) strtoll(argv[currIndex++], NULL, 10);

            clLogTrace("CLI", "EXP", "Value of the attrId [%d] is [%lld]", pAttrList->pAttrInfo[index].attrId, value); 

            pAttrList->pAttrInfo[index].pValue = clHeapAllocate(attrSize);
            if(pAttrList->pAttrInfo[index].pValue == NULL)
            {
                clLogError("CLI", "EXP", "Failed while allocating the memory for the attrId [0x%x], classId[0x%x]", 
                        pAttrList->pAttrInfo[index].attrId, classId);
                return rc;
            }

            memset(pAttrList->pAttrInfo[index].pValue, 0, attrSize);
            
            pAttrList->pAttrInfo[index].size = attrSize;
            
            if (CL_COR_ARRAY_ATTR == attrH->attrType.type)
                pAttrList->pAttrInfo[index].index = 0;
            else
                pAttrList->pAttrInfo[index].index = CL_COR_INVALID_ATTR_IDX;

            /* Convert the value of the big endian format.*/
            dmObjectAttrConvert(pAttrList->pAttrInfo[index].pValue, value, attrH->attrType, attrSize); 
        }
    }

    return rc;
}

/**
 * Function to work on the attribute list and set the required information.
 */ 
ClRcT _clCorDbgUtilsCorObjAttrSet(ClUint32T argc, ClCharT *argv[], ClCorCtAndSetInfoPtrT pAttrInfo)
{
    ClRcT rc = CL_OK;

    return rc;
}



/**
 * Function to get the attribute type information.
 */ 
ClRcT _clCorDbgUtilsCorObjAttrTypeGet(ClUint32T argc, 
                                      ClCharT *argv[], 
                                      ClCorAttrTypeT *pAttrType, 
                                      ClCorTypeT *pAttrDataType)
{
    ClRcT rc = CL_OK;

    return rc;
}


/**
 * EO function to execute the commands executed on the COR client's CLI.
 */ 

ClRcT VDECL(_clCorClientDbgCliExport) (ClEoDataT eoData, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle)
{
    ClRcT               rc = CL_OK;
    ClCharT             **argv = NULL;
    ClUint32T           argc = 0, index = 0;
    ClCorClientCliOpT   cliOp = CL_COR_CLIENT_CLI_INVALID_OP;
    ClCorCtAndSetInfoT  attrList = {{{{0}}}};

    /* CL_COR_FUNC_ENTER() ; */

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("CLI", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    rc = _clCorClientDebugDataUnMarshall(inMsgHandle, &cliOp, &argc, &argv);
    if ( CL_OK != rc )
    {
        clLogError( "CLI", "EXP", "Failed while umarshalling the cor \
                client's debug information. rc[0x%x]", rc);
        return rc;
    }

    clLogTrace("CLI", "EXP", "After unmarshalling the number of argument [%d] and opType [%d]", argc, cliOp);

    switch(cliOp)
    {
        case CL_COR_CLIENT_CLI_CREATE_AND_SET_OP:
        case CL_COR_CLIENT_CLI_ATTR_SET_OP:
            {
                rc = _clCorDbgUtilsCorObjCreateAndSet(cliOp, argc, argv, &attrList);
                if(CL_OK != rc)
                {
                    clLogError("CLI", "EXP", "Failed while executing the debug cli \
                            for create and set. rc[0x%x]", rc);
                    clCorClntDbgAttrListFinalize(&attrList);
                    goto error_on_exit;
                }

                clLogTrace("CLI", "EXP","Successfully processed the input. ");
                
                rc = clCorClntDbgCtNStMarshall(&attrList, outMsgHandle);
                if(rc != CL_OK)
                {
                    clLogError("CLI", "EXP", "Failed while marshalling the attrlist information");
                    clCorClntDbgAttrListFinalize(&attrList);
                    goto error_on_exit;
                }
                
                clLogTrace("CLI", "EXP", "Successfully marshalled the data. ");

                clCorClntDbgAttrListFinalize(&attrList);

                clLogTrace("CLI", "EXP", "Finalized the resources successfully. ");
            }
            break;
        case CL_COR_CLIENT_CLI_ATTR_TYPE_GET_OP:
            {
                ClCorTypeT attrDataType = CL_COR_INVALID_DATA_TYPE;
                ClCorAttrTypeT attrType = CL_COR_MAX_TYPE;

                rc = _clCorDbgUtilsCorObjAttrTypeGet(argc, argv, &attrType, &attrDataType);
                if(CL_OK != rc)
                {
                    clLogError( "CLI", "EXP", "Failed while executing the debug cli for \
                        getting the attriubte type. rc[0x%x]", rc);
                    goto error_on_exit;
                }
            }
            break;
        default:
            clLogError("CLI", "EXP", "Invalid type of the client cli issued");
    }

error_on_exit:

    for(index = 0; index < argc; index ++)
        clHeapFree(argv[index]);

    clHeapFree(argv);

    return rc;
}


/**
 * Helper function to put the class information
 * into the buffer message handle. This function is 
 * called from the MO and MSO callback function of the
 * MO class tree walk.
 */
ClRcT 
_clCorClassNamePut(ClPtrT element, ClPtrT *cookie)
{
    ClRcT               rc = CL_OK;
    ClBufferHandleT     bufH = {0};
    MArrayNode_h        pNode = NULL;
    ClCharT             className [CL_COR_MAX_NAME_SZ] = {0};
    ClCharT             tempStr [ CL_COR_CLI_STR_LEN] = {0};

    if (cookie == NULL || element == NULL)
    {
        clLogError("CLI", "CLS", "The pointer to cookie or element is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    bufH = *(ClBufferHandleT *)cookie;

    pNode = (MArrayNode_h) element;

    clLogTrace("CLI", "CLS", "Getting the class id[0x%x]", pNode->id);
    
    if (pNode->id == 0x1 || pNode->id == 0x0)
    {
        clLogTrace("CLI", "CLS", "Ignoring the classId [0x%x]", pNode->id);
        return CL_OK;
    }

    rc = _corNiKeyToNameGet(pNode->id, className);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed to get the class-name for the given class[0x%x]. rc[0x%x]",
                pNode->id, rc);
        return rc;
    }

    snprintf(tempStr, CL_COR_CLI_STR_LEN - 1 , "%#7x \t\t\t %s \n", 
            pNode->id, className);

    rc = clBufferNBytesWrite(bufH, (ClUint8T *)tempStr, strlen(tempStr));
    if ( CL_OK != rc)
    {
        clLogError("CLI", "CLS",
                "Failed while writing the string to the buffer handle. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}
  
/**
 * Function to pack the class information about the MSO.
 */
ClRcT 
_clCorMSOClassTreeWalkCB (ClUint32T idx, ClPtrT element, ClPtrT * cookie)
{
    ClRcT       rc = CL_OK;

    if (idx == (CL_COR_SVC_ID_ALARM_MANAGEMENT - 1))
    {
        clLogInfo("CLI", "CLS", "The alarm information is not needed.. so ignoring");
        return CL_OK;
    }

    rc = _clCorClassNamePut(element, cookie);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS",
                "Failed while putting the class information about the MSO. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}


/**
 * MO Tree callback function. This will get the class information
 * of the MO class and then it will write the information into the
 * buffer handle for a given MO class.
 */

ClRcT 
_clCorMOClassTreeWalkCB (ClUint32T idx, ClPtrT element, ClPtrT * cookie)
{
    ClRcT               rc = CL_OK;
    
    if (idx == COR_FIRST_NODE_IDX || idx == COR_LAST_NODE_IDX)
    {
        clLogInfo("CLI", "CLS", "First and last node is called. ignoring");
        return CL_OK;
    }
    
    rc = _clCorClassNamePut(element, cookie);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed while putting the class information about the MO. rc[0x%x]", 
                rc);
        return rc;
    }
  
    return rc;
}

/**
 * Function to copy the classes information about the MO class tree.
 */

ClRcT 
_clCorMOClassTreeWalk(ClBufferHandleT bufH)
{
    ClRcT           rc = CL_OK;
    MArrayWalk_t    walkInfo = {0};
    ClUint32T       idx = 0;

    walkInfo.pThis = moTree;
    walkInfo.fpNode = _clCorMOClassTreeWalkCB;
    walkInfo.fpData = _clCorMSOClassTreeWalkCB;
    walkInfo.cookie = &bufH;
    walkInfo.internal = 1;
    walkInfo.flags = CL_COR_MO_WALK;

    if ((moTree != NULL) && (moTree->root != NULL))
        idx = moTree->root->id;
    else
    {
        clLogError("CLI", "CLS", 
                "The pointer for MOTree or root of the MO Tree is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = _mArrayNodeWalk( idx, moTree->root, (void **) &walkInfo);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed while doing the MArray Walk. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}


/**
 * Function to walk the Dm classes list and return only the MO classes. It
 * will not return the MSO classes and the base classes.
 */

ClRcT 
_clCorClassShow (ClBufferHandleT bufH)
{
    ClRcT       rc = CL_OK;

    rc = _clCorMOClassTreeWalk(bufH);
    if (CL_OK != rc)
    {
        clLogError("CLI", "CLS", 
                "Failed while walking the MO class tree. rc[0x%x]", rc);
        return rc;
    }

    return rc;
}


/**
 * Function to get the information 
 * about the class given the classId.
 */
ClRcT 
_clCorGetClassInfoFromClassId (ClCorClassTypeT classId, ClBufferHandleT bufH)
{
    ClRcT       rc = CL_OK;
#if 1
    CORClass_h  clsH = NULL;

    clsH = dmClassGet(classId);
    if (clsH == NULL)
    {
        clLogError("CLI", "GCI",
                "The class Id [0x%x] passed is invalid. rc[0x%x]", 
                classId, rc);
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
    }


    
#endif
    return rc;
}

