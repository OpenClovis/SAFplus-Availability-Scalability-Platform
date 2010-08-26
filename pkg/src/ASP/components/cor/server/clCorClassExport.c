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
 * File        : clCorClassExport.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains COR component's  Class definition module.  
 *****************************************************************************/

/* INCLUDES */

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clBufferApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>

/*Internal Headers*/
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorPvt.h"
#include "clCorClient.h"
#include "clCorNiIpi.h"
#include "clCorLog.h"
#include "clCorRMDWrap.h"
#include "clCorDmData.h"

#include <xdrCorClsIntf_t.h>
#include <xdrCorAttrIntf_t.h>
#include <xdrClCorAttrDefT.h>
#include <xdrClCorAttrDefIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


/***********************************GLOBALS*********************************/

extern ClCorInitStageT gCorInitStage;
extern ClUint32T gCorSlaveSyncUpDone;

/***************************************************************************/
/*
 *   Server stub for corClass create/delete operations
 */

ClRcT 
VDECL(_corClassOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
	                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT          rc = CL_OK;
    corClsIntf_t    *pCls = NULL;
    
    CL_FUNC_ENTER();
 
   if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
   {
       clLogError("CLS", "EOF", "The COR server Initialization is in progress....");
       return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
   }

    pCls = clHeapAllocate(sizeof(corClsIntf_t));
    if (pCls == NULL)
    {
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("malloc failed"));
             return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorClsIntf_t, 4, 0, 0)(inMsgHandle, (void *)pCls)) != CL_OK)
	{
		clHeapFree(pCls);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corClsIntf_t "));	
		return rc;
	}

	clCorClientToServerVersionValidate(pCls->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pCls);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    if (pCls->op == COR_CLASS_OP_CREATE)
    {
        rc = dmClassCreate(pCls->classId, pCls->superClassId);
        if(CL_OK != rc )
        	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_CLASS_CREATE, rc);
    }
    else if (pCls->op == COR_CLASS_OP_DELETE)
    {
        rc =  dmClassDelete(pCls->classId);
        if(CL_OK != rc )
        {
        	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_CLASS_DELETE, rc);
        }
        else
        {
            _corNIClassDel(pCls->classId);
        }
    }

    if (gCorSlaveSyncUpDone == CL_TRUE)
    {
        ClRcT retCode = CL_OK;

        retCode = clCorSyncDataWithSlave(COR_EO_CLASS_OP, inMsgHandle);
        if (retCode != CL_OK)
        {
            clLogError("SYNC", "", "Failed to sync data with slave COR. rc [0x%x]", rc);
            /* Ignore the error code. */
        }
    }

    CL_FUNC_EXIT();
	clHeapFree(pCls);
    return rc;
}

/*
 *   Server stub for all corAttr operations 
 */

ClRcT 
VDECL(_corAttrOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
		                                  ClBufferHandleT  outMsgHandle)
{
    corAttrIntf_t   *pAttr = NULL;
    CORClass_h      classHdl = NULL; 
    ClRcT          rc = CL_OK;

    CL_FUNC_ENTER();
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
       clLogError("ATR", "EOF", "The COR server Initialization is in progress....");
       return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pAttr = clHeapAllocate(sizeof(corAttrIntf_t));
    if (pAttr == NULL)
    {
        clLogError("ATR", "EOF", "Failed while allocating the memory for Attribute information from client.");
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

	if((rc = VDECL_VER(clXdrUnmarshallcorAttrIntf_t, 4, 0, 0)(inMsgHandle, (void *)pAttr)) != CL_OK)
	{
		clHeapFree(pAttr);
        CL_FUNC_EXIT();
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall the data"));
		return rc;
	}

	clCorClientToServerVersionValidate(pAttr->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pAttr);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    /* Check the existence of class */
    if (NULL == (classHdl = dmClassGet(pAttr->classId)))
    {
		clHeapFree(pAttr);
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Class Id 0x%x is not present ", pAttr->classId));
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_NOT_PRESENT);
    }

    switch (pAttr->op)
    {
        case COR_ATTR_OP_CREATE:
        {
            rc = dmClassAttrCreate(classHdl, pAttr);
            if(CL_OK != rc )
        	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_CREATE, rc);
        }
        break;

        case COR_ATTR_OP_DELETE:
        {
            rc = dmClassAttrDel(classHdl, pAttr->attrId);
            if(CL_OK != rc )
            	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
				CL_LOG_MESSAGE_1_ATTR_DELETE, rc);
             _corNIAttrDel(pAttr->classId, pAttr->attrId); 
        }
        break;

        case COR_ATTR_OP_VALS_SET:
        {
            rc =  _corClassAttrValueSet(classHdl, pAttr->attrId, pAttr->init, pAttr->min, pAttr->max);
            if(CL_OK != rc )
            	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_VALUE_SET, rc);
        }
        break;

        case COR_ATTR_OP_FLAGS_SET:
        {
            rc = _corClassAttrUserFlagsSet(classHdl, pAttr->attrId, pAttr->flags);
            if(CL_OK != rc )
            	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
					CL_LOG_MESSAGE_1_ATTR_FLAG_SET, rc);
        }
        break;

        case COR_ATTR_OP_FLAGS_GET:
        case COR_ATTR_OP_TYPE_GET:
        case COR_ATTR_OP_VALS_GET:
        {
            CORAttr_h   attrHdl = NULL;

            HASH_GET(classHdl->attrList, pAttr->attrId, attrHdl);
            if (attrHdl != NULL)
            {
                pAttr->flags = attrHdl->userFlags;
                pAttr->min  = attrHdl->attrValue.min;
                pAttr->max  = attrHdl->attrValue.max;
                pAttr->init = attrHdl->attrValue.init;
                pAttr->attrType = attrHdl->attrType.type;

				rc = VDECL_VER(clXdrMarshallcorAttrIntf_t, 4, 0, 0)(pAttr, outMsgHandle, 0);
                if (CL_OK != rc)
                    clLogError("AOP", "EXP",
                            "Failed while marshalling the attribute info. rc[0x%x]", rc);

            }
            else rc = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
            if(CL_OK != rc )
                clLogError("AOP", "EXP", CL_LOG_MESSAGE_1_ATTR_GET, rc);
        }
        break;

        case COR_ATTR_OP_ID_GET_NEXT:
        {
            ClCorAttrDefT attrDef;
            ClCorAttrIdT  attrId = pAttr->attrId;
            CORAttr_h     attrH;
  
            memset(&attrDef, 0, sizeof(ClCorAttrDefT));

            if (pAttr->attrId == CL_COR_UNKNOWN_ATTRIB)
            {
                HASH_GET_FIRST_KEY(classHdl->attrList, attrId, rc) ;
            }
            else
            {
                HASH_GET_NEXT_KEY(classHdl->attrList, attrId, rc) ;
            }
            
            if(rc == CL_OK)
             {
               HASH_GET(classHdl->attrList, attrId, attrH);
            
                 attrDef.attrId = attrH->attrId;
                 attrDef.attrType = attrH->attrType.type;

               switch(attrH->attrType.type)
                    {
                     case CL_COR_CONTAINMENT_ATTR:                         
                     case CL_COR_ASSOCIATION_ATTR:
                            attrDef.u.attrInfo.arrDataType = CL_COR_INVALID_DATA_TYPE;
                            attrDef.u.attrInfo.maxElement = attrH->attrValue.max;
                            attrDef.u.attrInfo.classId = attrH->attrType.u.remClassId;
                          break;
                     case CL_COR_ARRAY_ATTR:
                            attrDef.u.attrInfo.arrDataType = attrH->attrType.u.arrType;
                            attrDef.u.attrInfo.maxElement = attrH->attrValue.min;
                          break;
                     default:
                             attrDef.u.attrInfo.arrDataType = attrH->attrType.type;
                             attrDef.attrType = CL_COR_SIMPLE_ATTR;
                             attrDef.u.simpleAttrVals.init = attrH->attrValue.init;
                             attrDef.u.simpleAttrVals.min = attrH->attrValue.min;
                             attrDef.u.simpleAttrVals.max = attrH->attrValue.max;
                    }
             }
   
			VDECL_VER(clXdrMarshallClCorAttrDefT, 4, 0, 0)(&attrDef, outMsgHandle, 0);
            if (CL_OK != rc) rc = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED);
        }
        break;

        case COR_ATTR_OP_ID_LIST_GET:
        {
            VDECL_VER(ClCorAttrDefIDLT, 4, 1, 0) attrDefIDL = {0};

            attrDefIDL.flags = pAttr->flags;

            rc = HASH_ITR_ARG(classHdl->attrList, dmClassAttrListGet, (void *) &attrDefIDL, 0);
            if (rc != CL_OK)
            {
                clLogError("AOP", "EXP", "Failed to walk the attribute list of class : [%d]", classHdl->classId);
                break;
            }

            VDECL_VER(clXdrMarshallClCorAttrDefIDLT, 4, 1, 0) (&attrDefIDL, outMsgHandle, 0);
        }
        break;

        default :
            rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
            break;
    }

    /*
     * Sync up only the changes with standby cor.
     */
    if (pAttr->op == COR_ATTR_OP_CREATE ||
            pAttr->op == COR_ATTR_OP_DELETE ||
            pAttr->op == COR_ATTR_OP_VALS_SET ||
            pAttr->op == COR_ATTR_OP_FLAGS_SET)
    {
        if (gCorSlaveSyncUpDone == CL_TRUE)
        {
            ClRcT retCode = CL_OK;

            retCode = clCorSyncDataWithSlave(COR_EO_CLASS_ATTR_OP, inMsgHandle);
            if (retCode != CL_OK)
            {
                clLogError("SYNC", "", "Failed to sync data with slave COR. rc [0x%x]", rc);
                /* Ignore the error code. */
            }
        }
    }

    CL_FUNC_EXIT();
	clHeapFree(pAttr);
    return rc;
}


/*
 *   Internal routine 
 */
ClRcT 
_corClassAttrValueSet(CORClass_h classHandle, 
                     ClCorAttrIdT attrId,
                     ClInt64T  init,
                     ClInt64T  min, 
                     ClInt64T  max)
{
    ClRcT ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    CORAttr_h ah = NULL ;

    CL_FUNC_ENTER();
    ah = dmClassAttrGet(classHandle, attrId);

    if(ah && (ah->attrType.type < CL_COR_MAX_TYPE) && (ah->attrType.type > 0))
    {
        ret = dmClassAttrInit(ah, init, min, max);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "ClassAttrInit Invalid Attribute Type"));
        ret = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
              ( "ClassAttrInit Class:%04x  Attr:%x, <Init:%lld,min:%lld,max:%lld>=> RET [%04x]",
               classHandle->classId,
               attrId,
               init,
               min,
               max,
               ret));


    CL_FUNC_EXIT();
    return (ret);
}


/**
 * Function to create a string which will contain 
 * all the user flags information.
 */

void
_clCorAttrFlagStringCreate ( ClCorAttrFlagT attrFlag, ClCharT *flagStr)
{
    if(attrFlag & CL_COR_ATTR_CONFIG)
        sprintf(flagStr, "   CF");
    else if(attrFlag & CL_COR_ATTR_RUNTIME)
        sprintf(flagStr, "   RT");
    else if(attrFlag & CL_COR_ATTR_OPERATIONAL)
        sprintf(flagStr, "   OP");
    else
        sprintf(flagStr, "     ");
    
     if(attrFlag & CL_COR_ATTR_INITIALIZED)
        strcat(flagStr," / INITED");
     else
        strcat(flagStr," / N-INITED");
    
    if(attrFlag & CL_COR_ATTR_WRITABLE)
        strcat(flagStr," / WRBLE");
    else
        strcat(flagStr," / N-WRBLE");
   
    if(attrFlag & CL_COR_ATTR_CACHED)
        strcat(flagStr," /   C$");
    else
        strcat(flagStr," / N-C$");

    if(attrFlag & CL_COR_ATTR_PERSISTENT)
        strcat(flagStr," / PERS");
    else
        strcat(flagStr," / N-PERS");

    return ;
}


/**
 *  Set user defined flags
 *
 *  
 *  Set the user defined flags for a given attribute in a class
 *                                                                        
 *  @param classHandle  Handle to class type
 *  @param attrId       Attribute Identifier
 *  @param flags         the value of flags to be set
 *
 *  @returns 
 *    CL_OK on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT)  Attribute not present <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM)  Invalid values passed to set <br/>
 *
 */

ClRcT _corClassAttrUserFlagsSet(CORClass_h classHandle, ClCorAttrIdT attrId, ClUint32T flags)
{
	CORAttr_h ah = NULL;
    ClCharT flagStr[CL_COR_MAX_NAME_SZ] = {0};
    ClCorClassTypeT     classId = 0;

    if(NULL == classHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Class info is NULL pointer."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    

	if (NULL == (ah = dmClassAttrGet(classHandle, attrId)))
    {
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get the attribute"));
		return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_PRESENT);
    }
    else
    {
        /* 
         * Mandatory validations done before setting the flags:
         * 1. If the attribute is configuration :
         *  a. Always cached.
         *  b. Always persistent.
         *  c. Can be initialized/non-initialized.
         *  d. Can be writable/non-writable.
         * 
         * 2. If the attribute is Runtime :
         *   a. Always read-Only
         *   b. Non-cached/Cached. In case of cached set is allowed only form OI.
         *   c. Persistent only when cached.
         * 
         * 3. If the attribute is Operational :
         *   a. Always Non-cached.
         *   b. Always Non-persistent.
         *   c. Always Read-Write.
         */

        if(flags & CL_COR_ATTR_CONFIG)
        {
            if( !(flags & CL_COR_ATTR_CACHED) || 
                !(flags & CL_COR_ATTR_PERSISTENT))
            {
                _clCorAttrFlagStringCreate(flags, flagStr);

                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                        CL_LOG_MESSAGE_3_CONFIG_ATTR_FLAG_INVALID, classId, attrId, flagStr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid flags specified for the Config attribute [0x%x] in class [0x%x]", attrId, classId));
                return CL_COR_SET_RC(CL_COR_ERR_CONFIG_ATTR_FLAG);
            }
        }
        else if(flags & CL_COR_ATTR_RUNTIME)
        {
            if((flags & CL_COR_ATTR_WRITABLE)) 
            {
                _clCorAttrFlagStringCreate(flags, flagStr);

        	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                    CL_LOG_MESSAGE_3_RUNTIME_ATTR_FLAG_INVALID, classId, attrId, flagStr);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The runtime attributes are read-only."));
                return CL_COR_SET_RC(CL_COR_ERR_RUNTIME_ATTR_WRITE);
            }
            else if(!(flags & CL_COR_ATTR_CACHED))
            {
                if(flags & CL_COR_ATTR_PERSISTENT)
                {
                    _clCorAttrFlagStringCreate(flags, flagStr);

        	        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_3_RUNTIME_ATTR_FLAG_INVALID, classId, attrId, flagStr);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Runtime attribute cannot be persistent without being cached."));
                    return CL_COR_SET_RC(CL_COR_ERR_ATTR_PERS_WITHOUT_CACHE);
                }
            }
        }
        else if ( flags & CL_COR_ATTR_OPERATIONAL )
        {
             if( !(flags & CL_COR_ATTR_WRITABLE) || (flags & CL_COR_ATTR_CACHED) || (flags & CL_COR_ATTR_PERSISTENT))
            {
                _clCorAttrFlagStringCreate(flags, flagStr);    

                clLogError("INT", "XML", " Invalid flag assigned to opearational attribute [%#x:%#x], flagStr [%s]", 
                        classId, attrId, flagStr);
                return CL_COR_SET_RC(CL_COR_ERR_OP_ATTR_TYPE_INVALID);
            }
        }
        else
        {
            _clCorAttrFlagStringCreate(flags, flagStr);    

            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, 
                        CL_LOG_MESSAGE_3_ATTR_FLAG_INVALID, classId, attrId, flagStr);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Attribute type is invalid. attrId [0x%x], flag [0x%x]", ah->attrId, flags));
            return CL_COR_SET_RC(CL_COR_ERR_ATTR_FLAGS_INVALID); 
        }
        
        /* Now assign the flag */
		ah->userFlags = flags;
    }

	return CL_OK;
}

