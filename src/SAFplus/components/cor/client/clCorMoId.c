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
 * File        : clCorMoId.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Utility module that provides moId manipulation utilities.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clEoApi.h>
#include <clCorNotifyApi.h>
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clXdrApi.h>
#include <clBitApi.h>

/*Internal Headers*/
#include <clCorRMDWrap.h>
#include <clCorClient.h>
#include <clCorLog.h>

#include <xdrClCorMoIdOpT.h>
#include <xdrClCorMOIdT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

extern ClRcT  corEOUpdatePwd (ClUint32T eoArg,  ClCorMOIdT* pwd, ClUint32T inLen,
                       char* outBuff, ClUint32T *outLen);

/** 
 *  Constructor for ClCorMOId. Initializes the memory and returns back an
 *  empty ClCorMOId. The "instance" field in all the entries are set to
 *  -1 as default. By default the number of entries is set to 20, and
 *  is grown dynamically when a new entry is added.
 *
 *  @param this      [out] new ClCorMOId handle
 *
 *  @returns 
 *    CL_OK        on success <br/>
 *    CL_COR_ERR_NO_MEM  on out of memory <br/>
 * 
 */
ClRcT
clCorMoIdAlloc(ClCorMOIdPtrT *this)
{
    ClCorMOIdPtrT tmp = 0;
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
  
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    tmp = (ClCorMOIdPtrT) clHeapAllocate(sizeof(ClCorMOIdT));
    *this = tmp;
    if(tmp != 0) 
    {
        /* init stuff here */
        for(i = 0; i < CL_COR_HANDLE_MAX_DEPTH; i++) 
        {
            tmp->node[i].type     = CL_COR_INVALID_MO_ID;
            tmp->node[i].instance = CL_COR_INVALID_MO_INSTANCE;
        }

        tmp->svcId = CL_COR_INVALID_SVC_ID;
        tmp->depth = 0; 
        tmp->qualifier = CL_COR_MO_PATH_ABSOLUTE; 

        /* Initialize the version information */
        tmp->version.releaseCode = CL_RELEASE_VERSION;
        tmp->version.majorVersion = CL_MAJOR_VERSION;
        tmp->version.minorVersion = CL_MINOR_VERSION;
    } 
    else 
    {
	    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
        ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }  
  
    CL_FUNC_EXIT();  
    return (ret);
}

/** 
 *  Initialize ClCorMOId.
 *
 *  Resets the path info (if present) and re-inits the 
 *  path to empty.
 *
 *  @param this      MO Id handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */
ClRcT
clCorMoIdInitialize(ClCorMOIdPtrT this)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* init stuff here */
    for( i = 0; i < CL_COR_HANDLE_MAX_DEPTH; i++) 
    {
        this->node[i].type     = CL_COR_INVALID_MO_ID;
        this->node[i].instance = CL_COR_INVALID_MO_INSTANCE;
    }

    this->svcId = CL_COR_INVALID_SVC_ID;
    this->depth = 0; 
    this->qualifier = CL_COR_MO_PATH_ABSOLUTE; 

    /* Initialize with the current version */
    this->version.releaseCode = CL_RELEASE_VERSION;
    this->version.majorVersion = CL_MAJOR_VERSION;
    this->version.minorVersion = CL_MINOR_VERSION;

    CL_FUNC_EXIT();
    return (ret);
}

/** 
 *  Does byte conversion on moId fields.
 *
 *  @param this      MO Id handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */

ClRcT
clCorMoIdByteSwap(ClCorMOIdPtrT this)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    /* init stuff here */
    for( i = 0; i < CL_COR_HANDLE_MAX_DEPTH; i++) 
    {
        this->node[i].type     = CL_BIT_SWAP32(this->node[i].type);
        this->node[i].instance = CL_BIT_SWAP32(this->node[i].instance);
    }

    this->svcId = CL_BIT_SWAP16(this->svcId);
    this->depth = CL_BIT_SWAP16(this->depth); 
    this->qualifier = CL_BIT_SWAP16(this->qualifier); 
  
    /* No need to swap the version information, since it contains
     * three 8-bit fields.
     */

    CL_FUNC_EXIT();
    return (ret);
}


#if 0
/** 
 *  Normalize ClCorMOId.
 *
 *  This routine normalizes the moId. The changes includes:
 *    1. Converting the ClCorMOId into absolute path.
 *    2. Filling the invalid part of the path with INVALID values. 
 *
 *  @param this      MO Id handle
 *
 *  @returns 
 *    CL_OK       on success <br/>
 */
ClRcT
clCorMoIdNormalize(ClCorMOIdPtrT this)
{
    ClRcT rc = CL_OK;
    ClInt32T i = 0;
    ClCorMOIdPtrT pwd = 0;
    ClCorMOIdT base;
    ClCorMOIdPtrT pMoid;
    ClEoExecutionObjT* pEOObj = NULL;
    ClUint32T outSize = sizeof(ClCorMOIdT);

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_ERR_NULL_PTR);
    }

    /*pwd = (ClCorMOIdPtrT) clHeapAllocate (sizeof(ClCorMOIdT));*/
    pwd = &base;
    clCorMoIdInitialize(pwd);

    switch(this->qualifier)
    {
        case CL_COR_MO_PATH_RELATIVE:
	{
            rc = clEoMyEoObjectGet(&pEOObj);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n clEoMyEoObjectGet failed, rc = %x \n", rc));
                return rc;
            } 

            /* get the current path from the EO specific area */
            if ((rc = clEoPrivateDataGet(pEOObj, CL_COR_WTH_COOKIE_ID,
                                    (void **)&pwd)) != CL_OK) 
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n clCorMoIdNormalize: GET WhereToHang FROM COR \n"));
 
                COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_EO_WHERETOHANG_GET, NULL, NULL, 0, clXdrUnmarshallClCorMOIdT, 
																						(char *)pwd,  &outSize, rc);
                if(rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " Failed to obtain where to hang info"));
                    CL_FUNC_EXIT();
                    return (rc);
                }
                /* update whereToHang info in local cache */
                pMoid = (ClCorMOIdPtrT) clHeapAllocate (sizeof(ClCorMOIdT));
		if(pMoid == NULL)
		{
			clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
						CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to allocate Memory for moId"));
			return CL_COR_ERR_NO_MEM;
		}

                memcpy(pMoid, pwd, sizeof(ClCorMOIdT));
                if ((rc = clEoPrivateDataSet(pEOObj, CL_COR_WTH_COOKIE_ID,
                            (void *)pMoid)) != CL_OK)
                {
                    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "corUpdatePwd: Could not set default corPath\n"));
                }

            }

            
            /* append current path to the 'this' */
            if(pwd->depth != 0)
            {    
                if ((rc = clCorMoIdConcatenate(pwd, this, 1)) != CL_OK)
                {
                    CL_FUNC_EXIT();
                    return (rc);
                }
            }    
             
            break;
        }
        case CL_COR_MO_PATH_RELATIVE_TO_BASE:
        {
		  /* get the base path */
		  /* append the base path to the 'this' */
            if ((rc = clCorMoIdConcatenate(&base, this, 1)) != CL_OK)
            {
                CL_FUNC_EXIT();
                return (rc);
            }

	    break;
        }
        case CL_COR_MO_PATH_ABSOLUTE:
            break;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                      ( "clCorMoIdNormalize: Invalid moId qualifier => [0x%x]",
                      this->qualifier));
            break;
    }

    this->qualifier = CL_COR_MO_PATH_ABSOLUTE;
  /* init stuff here */
    for(i = this->depth; i < CL_COR_HANDLE_MAX_DEPTH; i++) 
    {
        this->node[i].type     = CL_COR_INVALID_MO_ID;
        this->node[i].instance = CL_COR_INVALID_MO_INSTANCE;
    }
    CL_FUNC_EXIT();
    return (rc);
}
#endif

/** 
 *  Free/Delete the ClCorMOId Handle.
 *
 *  Destructor for ClCorMOId.  Removes/Deletes the handle and frees back
 *  any associated memory.
 *
 *  @param this ClCorMOId handle
 *
 *  @returns 
 *    ClRcT  CL_OK on successful deletion.
 * 
 */
ClRcT
clCorMoIdFree(ClCorMOIdPtrT this)
{
    CL_FUNC_ENTER();
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    clHeapFree(this); /* no need to check null (COR_FREE will take care) */
    CL_FUNC_EXIT();

    return (CL_OK);
}

/** 
 *  Remove nodes after specified level.
 *
 *  Remove all nodes and reset the MO Id till the level specified.
 *
 *  @param this  MO Id
 *  @param level level to which the MO Id needs to be truncated
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 * 
 */
ClRcT
clCorMoIdTruncate(ClCorMOIdPtrT this, ClInt16T level)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    
    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if(level  == this->depth)
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }

  /* Check for current depth against the level */
    if(level < this->depth && level >= 0) 
    {
        /* init stuff here */
        for(i = level; i < this->depth; i++) 
        {
            this->node[i].type     = CL_COR_INVALID_MO_ID;
            this->node[i].instance = CL_COR_INVALID_MO_INSTANCE;
        }
        this->depth = (level);
    
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_INVALID_DEPTH));
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
  
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Set the Service Id.
 *
 *  Set Service to the ClCorMOId.
 *
 *  @param this      MOIdPath
 *  @param svc       Service Id
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 * 
 */
ClRcT
clCorMoIdServiceSet(ClCorMOIdPtrT this, 
               ClCorMOServiceIdT svc)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else if ((svc != CL_COR_SVC_WILD_CARD) &&
                ((CL_COR_INVALID_SRVC_ID > svc) || (CL_COR_SVC_ID_MAX < svc)))
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_MSP_ID);
    }
    else 
    { 
        this->svcId = svc; 
    }
    CL_FUNC_EXIT();
    return (ret);
}


/** 
 *  Get the Service Id.
 *
 *  Get Service associated with ClCorMOId.
 *
 *  @param this      MOIdPath
 *
 *  @returns 
 *    ClCorMOServiceIdT     
 *     CL_OK on success <br/>
 * 
 */
ClCorMOServiceIdT
clCorMoIdServiceGet(ClCorMOIdPtrT this)
{
    CL_FUNC_ENTER();
    clDbgCheckNull(this,CL_CID_COR);
    
    return this?this->svcId : -1;
}


/** 
 *  Set the Class Type and instance id at given node.
 *
 *  Set class type and instance id properties at a given node (level).
 *
 *  @param this      MOIdPath
 *  @param level     node level
 *  @param type      Class Type to be set 
 *  @param instance  Instance Id  to be set 
 *
 *  @returns 
 *    ClRcT  
 *     CL_OK on success <br/>
 *     CL_COR_ERR_INVALID_DEPTH  if the level specified is invalid <br/>
 * 
 */
ClRcT
clCorMoIdSet(ClCorMOIdPtrT this, 
        ClUint16T level,
        ClCorClassTypeT type, 
        ClCorInstanceIdT instance)
{
    CL_FUNC_ENTER();
    
    if (this == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "MID", "SET", "NULL pointer passed");
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if ((instance < 0) && (instance != CL_COR_INSTANCE_WILD_CARD) && (instance != CL_COR_INVALID_MO_INSTANCE))
    {
        clLog(CL_LOG_SEV_ERROR, "MID", "SET", "Invalid MO instance passed.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }
    
    if( (type <= 0)  && (CL_COR_CLASS_WILD_CARD != type))
    {
        clLog(CL_LOG_SEV_ERROR, "MID", "SET", "Invalid Class ID passed.");
        CL_FUNC_EXIT();        
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS));
    }
    
    if((level > 0) && (level <= this->depth))
    {
        /* Check for current depth against the level */
        this->node[level-1].type = type;
        this->node[level-1].instance = instance;
    }
    else 
    {
        clLog(CL_LOG_SEV_ERROR, "MID", "SET", "Invalid moId level passed [%d].", level);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH));
    }
  
    CL_FUNC_EXIT();
    return (CL_OK);
}

/** 
 * Add an entry to the MoId.
 * 
 * This API adds an entry to the ClCorMOId. The user explicitly
 * specifies the type and the instance for the entry.
 *
 *  @param this      ClCorMOId
 *  @param type      Node type 
 *  @param instance  Node instance identifier
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br/>
 *       CL_COR_ERR_MAX_DEPTH if max depth exceeded <br/>
 * 
 */
ClRcT 
clCorMoIdAppend(ClCorMOIdPtrT this,
           ClCorClassTypeT type,
           ClCorInstanceIdT instance)
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else if ( ( type != CL_COR_CLASS_WILD_CARD ) && (type <  0) )
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_CLASS);
    }
	/* JC - In case of moidByTypeAppend, we may want to fill up invalid MO Instance */
    else if ( ( CL_COR_INSTANCE_WILD_CARD != instance ) && ( 0 > instance ) && (CL_COR_INVALID_MO_INSTANCE != instance) )
    {
        ret = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
    }

  /* Check for current depth against the default */
  /* Todo: Need to take care of the error scenario later */
    else if(this->depth < CL_COR_HANDLE_MAX_DEPTH) 
    {
        this->node[this->depth].type = type;
        this->node[this->depth++].instance = instance;
    } 
    else 
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MAX_DEPTH));
        ret = CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH);
    }
  
    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Get the ClCorMOId node depth.
 * 
 * Returns the no of nodes in the hierarchy in the MO Id.
 *
 *  @param this ClCorMOId handle
 * 
 *  @returns 
 *    ClInt16T number of elements <br/>
 * 
 */
ClInt16T
clCorMoIdDepthGet(ClCorMOIdPtrT this)
{
    CL_FUNC_ENTER();
    if (this == NULL) return CL_COR_INVALID_SRVC_ID;    
    return(this->depth);
}

/** 
 * Display the ClCorMOId Handle.
 * 
 * This API displays all the entries within the ClCorMOId Handle
 *
 *  @param this ClCorMOId handle
 * 
 *  @returns 
 *    none
 * 
 */
void
clCorMoIdShow(ClCorMOIdPtrT this)
{
    ClInt32T i = 0;
    ClInt32T like = 0;
    ClCharT tmpBuf[16];
    ClCharT moIdStr[CL_MAX_NAME_LENGTH];
    ClCharT objInst[CL_MAX_NAME_LENGTH];
    ClCharT className[CL_MAX_NAME_LENGTH];
    ClCharT moIdTemp[CL_MAX_NAME_LENGTH];

    if (this) 
    {
        sprintf(moIdStr, "ClCorMOId: [Svc:%2X] ", this->svcId);

        for (i=0,like=0;i<this->depth && this->depth <= CL_COR_HANDLE_MAX_DEPTH;i++) 
        {
            if (i<(this->depth-1) &&
               this->node[i].type == this->node[i+1].type &&
                this->node[i].instance == this->node[i+1].instance) 
            {
                like++;
            } 
            else 
            {
                sprintf(tmpBuf,"[%d]",like+1);

                if (this->node[i].type == CL_COR_CLASS_WILD_CARD)
                    sprintf(className, "*");
                else
                    sprintf(className, "%04x", this->node[i].type);

                if (this->node[i].instance == CL_COR_INSTANCE_WILD_CARD)
                    sprintf(objInst, "*");
                else
                    sprintf(objInst, "%04x", this->node[i].instance);
    
                sprintf(moIdTemp, "(%s:%s)%s", className, objInst, (like > 0 ? tmpBuf : ""));

                strcat(moIdStr, moIdTemp);

                sprintf(moIdTemp, (i<(this->depth-1))?".":"");

                strcat(moIdStr, moIdTemp);

                like = 0;
            }
        }

        clOsalPrintf("%s\n", moIdStr);
        clLogDebug("COR", "MOID", "MOID STRING [%s]", moIdStr);
    }
}

void
clCorMoIdGetInMsg(ClCorMOIdPtrT this, ClBufferHandleT* pMsg)
{
    ClInt32T i = 0;
    ClInt32T like = 0;
    char tmpBuf[16];
    ClCharT corStr[CL_COR_CLI_STR_LEN] = {0};

    if(this) 
    {
        corStr[0]='\0';
        sprintf(corStr, "ClCorMOId:[Svc:%2X] ", this->svcId);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

        corStr[0]='\0';
        sprintf(corStr, "%s", "(/).");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
        for(i=0,like=0;i<this->depth && this->depth <= CL_COR_HANDLE_MAX_DEPTH;i++) 
        {
            if(i<(this->depth-1) &&
               this->node[i].type == this->node[i+1].type &&
                this->node[i].instance == this->node[i+1].instance) 
            {
                like++;
            } 
            else 
            {
                sprintf(tmpBuf,"[%d]",like+1);
                corStr[0]='\0';
                sprintf(corStr, "(%04x:%04x)%s",
                         this->node[i].type,
                         this->node[i].instance,
                         (like>0?tmpBuf:""));
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
                corStr[0]='\0';
                if((i<(this->depth-1))>0)
                    sprintf(corStr, ".");
                else
                    sprintf(corStr, ":");
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
                like = 0;
            }
        }
        corStr[0]='\0';
        sprintf(corStr, "\n");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
    }
}



/** 
 * Get the Class Type.
 * 
 * API returns the class type that ClCorMOId referrs to.
 *
 *  @param this ClCorMOId handle
 *  @ClCorMoIdClassGetFlagsT  flag 
 *  @ClCorClassTypeT          pClassId  [OUT]returned class Id.
 *  @returns 
 *    CL_OK  on success<br/>
 * 
 */

ClRcT clCorMoIdToClassGet(ClCorMOIdPtrT this, ClCorMoIdClassGetFlagsT flag, ClCorClassTypeT *pClassId)
 {
   ClRcT   rc = CL_OK;
   ClVersionT 	version ; 

	/* Version Set */
	CL_COR_VERSION_SET(version);
   
    CL_FUNC_ENTER();
    /* Basic Validations */
    if(!this  || !pClassId)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL pointer passed .. \n"));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    if(flag < CL_COR_MO_CLASS_GET || flag > CL_COR_MSO_CLASS_GET)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid flag specified. \n"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    if(flag == CL_COR_MSO_CLASS_GET && this->svcId == CL_COR_INVALID_SRVC_ID)
     {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n MOId service is not set. CL_COR_INVALID_SRVC_ID \n"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
     }

    if(flag == CL_COR_MO_CLASS_GET)
    *pClassId = this->node[this->depth-1].type;
    else
    {
        /* We need to make RMD call to get the class type for MSO */

        ClCorMoIdOpT op = CL_COR_MOID_TO_CLASS_GET;
        ClBufferHandleT inMessageHandle;
        ClBufferHandleT outMessageHandle;

        rc = clBufferCreate(&inMessageHandle);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Input message buffer\n"));
            return (rc);
        }

        rc = clBufferCreate(&outMessageHandle);
        if(rc != CL_OK)
        {
            clBufferDelete(&inMessageHandle);
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Output  message buffer\n"));
            return (rc);
        }

        rc = clXdrMarshallClVersionT((void *)&version, inMessageHandle, 0);
        if (rc != CL_OK)
           goto HandleError;

        rc = VDECL_VER(clXdrMarshallClCorMoIdOpT, 4, 0, 0)((void *)&op, inMessageHandle, 0);
        if (rc != CL_OK)
           goto HandleError;

        rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)((void *)this, inMessageHandle, 0);
        if (rc != CL_OK)
            goto HandleError;

        COR_CALL_RMD_SYNC_WITH_MSG(COR_MOID_OP, inMessageHandle, outMessageHandle, rc);

        if(rc == CL_OK)
        { 
            /* Read the outBuf*/
             rc = clXdrUnmarshallClInt32T(outMessageHandle, pClassId);
             if (rc != CL_OK)
                goto HandleError;

        }

HandleError:
        clBufferDelete(&inMessageHandle);
        clBufferDelete(&outMessageHandle);
    }

    return (rc);
}

/**
 *
 *  @param    moIdName   moId in String format
 *  @param    moId       [OUT] moId returned. It has to be allocated by user.
 *
 *  @returns
 *    CL_OK - everything is ok <br>
 */

ClRcT clCorMoIdNameToMoIdGet(ClNameT *moIdName, ClCorMOIdT *moId)
{
    ClRcT rc;
    ClCorMoIdOpT op = CL_COR_NAME_TO_MOID_GET;
    ClBufferHandleT inMessageHandle;
    ClBufferHandleT outMessageHandle;
    ClVersionT 		  version;

    /* Version Set */
    CL_COR_VERSION_SET(version);
   
    CL_FUNC_ENTER();

    /* Basic Validations */
    if(!moIdName  || !moId)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL pointer passed .. \n"));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clBufferCreate(&inMessageHandle);
    if(rc != CL_OK)
   	{
   	    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Input message buffer\n"));
	    return (rc);
   	}

    rc = clBufferCreate(&outMessageHandle);
    if(rc != CL_OK)
   	{
        clBufferDelete(&inMessageHandle);
   	    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Output  message buffer\n"));
	    return (rc);
   	}

    rc = clXdrMarshallClVersionT((void *)&version, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    rc = VDECL_VER(clXdrMarshallClCorMoIdOpT, 4, 0, 0)((void *)&op, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    rc = clXdrMarshallClNameT((void *)moIdName, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    COR_CALL_RMD_SYNC_WITH_MSG(COR_MOID_OP, inMessageHandle, outMessageHandle, rc);

    if (rc != CL_OK)
       goto HandleError;
  
    if(rc == CL_OK)
    { 
        /* Read the outBuf*/
        rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(outMessageHandle, moId);
        if (rc != CL_OK)
            goto HandleError;
    }

    moId->version.releaseCode = CL_RELEASE_VERSION;
    moId->version.majorVersion = CL_MAJOR_VERSION;
    moId->version.minorVersion = CL_MINOR_VERSION;

HandleError:
    clBufferDelete(&inMessageHandle);
    clBufferDelete(&outMessageHandle);
        
    CL_FUNC_EXIT();
    return (rc);
}


/**
 *
 *  @param    moId       moId structure.
 *  @param    moIdName   [OUT]moId in String format
 *
 *  @returns
 *    CL_OK - everything is ok <br>
 */

ClRcT clCorMoIdToMoIdNameGet(ClCorMOIdT *moId, ClNameT *moIdName)
{
    ClRcT rc;
    ClCorMoIdOpT op = CL_COR_MOID_TO_NAME_GET;
    ClBufferHandleT inMessageHandle;
    ClBufferHandleT outMessageHandle;
    ClVersionT 		  version;

    /* Version Set */
    CL_COR_VERSION_SET(version);
   
    CL_FUNC_ENTER();

    /* Basic Validations */
    if(!moIdName  || !moId)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL pointer passed .. \n"));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = clBufferCreate(&inMessageHandle);
    if(rc != CL_OK)
   	{
   	    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Input message buffer\n"));
	    return (rc);
   	}

    rc = clBufferCreate(&outMessageHandle);
    if(rc != CL_OK)
   	{
        clBufferDelete(&inMessageHandle);
   	    CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "\n Could not create Output  message buffer\n"));
	    return (rc);
   	}

    rc = clXdrMarshallClVersionT((void *)&version, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    rc = VDECL_VER(clXdrMarshallClCorMoIdOpT, 4, 0, 0)((void *)&op, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)((void *)moId, inMessageHandle, 0);
    if (rc != CL_OK)
       goto HandleError;

    COR_CALL_RMD_SYNC_WITH_MSG(COR_MOID_OP, inMessageHandle, outMessageHandle, rc);

    if (rc != CL_OK)
       goto HandleError;
  
    if(rc == CL_OK)
    { 
        /* Read the outBuf*/
        rc = clXdrUnmarshallClNameT(outMessageHandle, moIdName);
        if (rc != CL_OK)
            goto HandleError;
    }

HandleError:
    clBufferDelete(&inMessageHandle);
    clBufferDelete(&outMessageHandle);
        
    CL_FUNC_EXIT();
    return (rc);
}



/** 
 * Get the Instance.
 * 
 * API returns the Instance that it referrs to (the bottom most class
 * type and its instance id in the hierarchy).
 *
 *  @param this ClCorMOId handle
 * 
 *  @returns 
 *    ClCorInstanceIdT Instance Id associated <br/>
 * 
 */
ClCorInstanceIdT
clCorMoIdToInstanceGet(ClCorMOIdPtrT this)
{
    return(this->node[this->depth-1].instance);
}

/** 
 * Set the Instance.
 * 
 * API sets the Instance field at the specified level of
 * ClCorMOId.
 *  @param this ClCorMOId handle
 * 
 *  @returns 
 *          CL_OK <br/>
 * 
 */
ClRcT
clCorMoIdInstanceSet(ClCorMOIdPtrT this, ClUint16T ndepth, 
		   ClCorInstanceIdT newInstance)
{
    CL_FUNC_ENTER();
    
    /* Bug Id      : 6020
     * Description : The Api clCorMoIdInstanceSet is dumping core if NULL pointer is given as the parameter for moId.
     * Fix         : Check the moId for the NULL pointer and return.
     */
    if (NULL == this)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer specified."));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    if (ndepth > this->depth)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid depth specified"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
    }
    
    this->node[ndepth-1].instance = newInstance;

    CL_FUNC_EXIT();
    return(CL_OK);
}

/**
 *  Validate the input MOID argument.
 *
 *  This API validates the input MOID argument.
 *                                                                        
 *  @param this  ClCorMOId
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorMoIdValidate(ClCorMOIdPtrT this)
{
    ClRcT retCode = CL_OK;
    CL_FUNC_ENTER();

    if (NULL == this)
    { 
        retCode = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else if ( (this->depth <= 0) || 
              (this->depth > CL_COR_HANDLE_MAX_DEPTH) ) 
    {
        clLogError("COR", "MVL", "Failed while validating the moId depth[%d], \
                The maximum value allowed is [20]", this->depth);
        retCode = CL_COR_SET_RC(CL_COR_ERR_INVALID_DEPTH);
    }
    else if ( (this->qualifier < CL_COR_MO_PATH_ABSOLUTE) ||
              (this->qualifier > CL_COR_MO_PATH_QUALIFIER_MAX))
    {
        retCode = CL_COR_SET_RC(CL_COR_ERR_CLASS_INVALID_PATH);
    }
    else if ( this->svcId < CL_COR_SVC_WILD_CARD || this->svcId > CL_COR_SVC_ID_MAX)
    {
        clLogError("COR", "MVL", "Failed while validating the moId service id[%d], \
                The maximum value allowed is [%d] and minimum value allowed is [-2]", 
                this->svcId, CL_COR_SVC_ID_MAX);
        retCode = CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_ID);
    }
#ifdef EYAL
    else 
    {
        ClCorMOClassPathPtrT  pCORPath;
        ClCorMOIdPtrT     pMOId;
        CORMOClass_h moClassHandle;
        ClCorObjectHandleT moObjHandle;
        int i;

  /* Walk thru the class hierarchy within the MOID and
   * verify with MO Tree if hierarchy is valid 
   */
        clCorMoClassPathAlloc(&pCORPath);
        clCorMoClassPathInitialize(pCORPath);
        clCorMoIdAlloc(&pMOId);
        clCorMoIdInitialize(pMOId);
        pCORPath->qualifier = this->qualifier;
        pMOId->svcId = this->svcId;
        pMOId->qualifier = this->qualifier;
        for (i = 0; i < this->depth; i++ )
        {
            clCorMoClassPathAppend(pCORPath, this->node[i].type);
            if ( (retCode = corUsrBufPtrGet(pCORPath, (void**) &moClassHandle)) != CL_OK) 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid MO-Class"));
                break;
            }

            clCorMoIdAppend(pMOId, this->node[i].type, this->node[i].instance);
            if ( ( i < this->depth - 1 )  && 
                 (retCode = corObjUsrBufPtrGet(pMOId, (void**) &moObjHandle)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid MO-Instance"));
                break;
            }
        }
        clCorMoClassPathFree(pCORPath);
        clCorMoIdFree(pMOId);
    }
#endif

    CL_FUNC_EXIT();
    return(retCode);
}


/**
 *  Compare two MOID's and check if they are equal.
 *
 *  Comparision API. Compare two ClCorMOId's and see if they are equal. The
 *  comparision goes thru only till the depth. 
 *                                                                        
 *  @param this  ClCorMOId handle 1
 *  @param cmp   ClCorMOId Handle 2
 *
 *  @returns 0 - if both ClCorMOId's are same (Even if both MOIDs have instances as wildcards)
 *          -1 - for mis-match
 *           1 - for wild-card match
 */
ClInt32T 
clCorMoIdCompare(ClCorMOIdPtrT this, ClCorMOIdPtrT cmp)
{
    ClInt32T i = 0;
    ClInt8T ret = 0;

    CL_FUNC_ENTER();

    if ((this == NULL) || (cmp == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if(this->depth != cmp->depth ||
       this->svcId != cmp->svcId ||
       this->qualifier != cmp->qualifier) 
    {
        return -1;
    }

  /* Walk thru the values and see if everything matches.
   */
    for(i = 0; i < this->depth; i++) 
    {
        if( cmp->node[i].type == this->node[i].type)
        {
            if(cmp->node[i].instance == CL_COR_INSTANCE_WILD_CARD &&
                this->node[i].instance == CL_COR_INSTANCE_WILD_CARD)
            {
               /* Both the MoIds have instances as wild cards. So its exact match. But if
                     we had a wildcard match previously, it is wildcard match and not an exact match */
               ret = ret | 0;
            }
            else if (cmp->node[i].instance == CL_COR_INSTANCE_WILD_CARD||
                this->node[i].instance == CL_COR_INSTANCE_WILD_CARD)
            {
                ret = 1;
            }
            else if (cmp->node[i].instance != this->node[i].instance) 
            {
                return -1; /* otherwise its a zero mis match */
            }
        }
        else if( (cmp->node[i].type == CL_COR_CLASS_WILD_CARD) || 
		 (this->node[i].type == CL_COR_CLASS_WILD_CARD) )
        {
        /* The class is declared as wildcard. */
        /* Check if the instances are same. If they are same 
         * then return wildcard match otherwise check if any of them are 
         * wild card, then return wildcard match otherwise return nomatch
         */
            if(cmp->node[i].instance == this->node[i].instance)
          	{
                ret = 1;
            }
            else if ( (cmp->node[i].instance == CL_COR_INSTANCE_WILD_CARD) || 
                      (this->node[i].instance == CL_COR_INSTANCE_WILD_CARD) )
            {
                ret = 1;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }

    CL_FUNC_EXIT();
    return(ret);
}

/*
 * Should be used for sorted inserts of MOid.
 */
ClInt32T 
clCorMoIdSortCompare(ClCorMOIdPtrT this, ClCorMOIdPtrT cmp)
{
    ClInt32T i = 0;

    if(this->depth != cmp->depth)
        return this->depth - cmp->depth;
    if(this->svcId != cmp->svcId)
        return this->svcId - cmp->svcId;
    if(this->qualifier != cmp->qualifier)
        return this->qualifier - cmp->qualifier;


  /* Walk thru the values and see if everything matches.
   */
    for(i = 0; i < this->depth; i++) 
    {
        if(this->node[i].type != cmp->node[i].type)
            return this->node[i].type - cmp->node[i].type;

        if(this->node[i].instance != cmp->node[i].instance)
            return this->node[i].instance - cmp->node[i].instance;
    }
    
    return 0;
}


/**
 *  Clone a given ClCorMOId.
 *
 *  Allocates and copies the contents to the new ClCorMOId.
 *                                                                        
 *  @param this  ClCorMOId Handle
 *  @param newH  [out] new clone handle
 *
 *  @returns 
 *    CL_OK        on success <br/>
 *    CL_COR_SET_RC(CL_COR_ERR_NO_MEM)  on out of memory <br/>
 */
ClRcT
clCorMoIdClone(ClCorMOIdPtrT this, ClCorMOIdPtrT* newH)
{
    ClRcT ret = CL_OK;
    ClInt32T i = 0;

    CL_FUNC_ENTER();
    
    if ((this == NULL) || (newH == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    ret = clCorMoIdAlloc(newH);
    if(ret == CL_OK) 
    {
    /* copy the contents here */
        for(i = 0; i < this->depth; i++) 
        {
            (*newH)->node[i].type = this->node[i].type;
            (*newH)->node[i].instance = this->node[i].instance;
        }
        (*newH)->svcId = this->svcId;
        (*newH)->depth = this->depth;
        (*newH)->qualifier = this->qualifier;
        (*newH)->version = this->version;
    }

    CL_FUNC_EXIT();
    return(ret);
}

ClBoolT
clCorMoIdIsWildCard(ClCorMOIdPtrT pMoId)
{
    ClUint32T i = 0;

    if (! pMoId)
    {
        clLogError("MOID", "UTL", "NULL pointer passed.");
        return CL_FALSE;
    }

    for (i=0; i < pMoId->depth; i++)
    {
        if (pMoId->node[i].type == CL_COR_CLASS_WILD_CARD ||
                pMoId->node[i].instance == CL_COR_INSTANCE_WILD_CARD)
        {
            return CL_TRUE;
        }
    }

    return CL_FALSE;
}


ClRcT clCorMoIdPack(ClCorMOIdT *pMoId, ClCharT **ppDataBuffer, ClUint32T *pDataSize)
{
    ClBufferHandleT msg = 0;
    ClRcT rc = CL_OK;

    if(!pMoId || !ppDataBuffer || !pDataSize) 
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    *ppDataBuffer = NULL;

    rc = clBufferCreate(&msg);
    CL_ASSERT(rc == CL_OK);

    rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)((ClUint8T*)pMoId, msg, 0);
    CL_ASSERT(rc == CL_OK);

    rc = clBufferLengthGet(msg, pDataSize);
    CL_ASSERT(rc == CL_OK);

    rc = clBufferFlatten(msg, (ClUint8T**)ppDataBuffer);
    CL_ASSERT(rc == CL_OK);

    clBufferDelete(&msg);
    return rc;
}

ClRcT clCorMoIdUnpack(ClCharT *pData, ClUint32T dataSize, ClCorMOIdT *pMoId)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msg = 0;
    if(!pData || !pMoId || !dataSize) 
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    rc = clBufferCreate(&msg);
    CL_ASSERT(rc == CL_OK);
    rc = clBufferNBytesWrite(msg, (ClUint8T*)pData, dataSize);
    CL_ASSERT(rc == CL_OK);
    rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0)(msg, pMoId);
    CL_ASSERT(rc == CL_OK);
    clBufferDelete(&msg);
    return rc;
}

#if 0
/** 
 * Get the ClCorMOId path qualifier.
 * 
 * Returns the ClCorMOId path qualifier which indicates
 * whether the path is absolute, relative or relative
 * to the location of the blade.
 *
 *  @param this ClCorMOId handle
 *         pQualifier: [OUT] qualifier value is returned here.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoIdQualifierGet(ClCorMOIdPtrT this, ClCorMoPathQualifierT *pQualifier)
{
    CL_FUNC_ENTER();
    
    if ((this == NULL) || (pQualifier == NULL))
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    *pQualifier = this->qualifier; 

    CL_FUNC_EXIT();
    return(CL_OK);
}


/** 
 * Set the ClCorMOId path qualifier.
 * 
 * Sets the ClCorMOId path qualifier which indicates
 * whether the path is absolute, relative or relative
 * to the location of the blade.
 *
 *  @param this COR handle
 *         qualifier: qualifier value to set.
 * 
 *  @returns CL_OK on successs.
 * 
 */
ClRcT
clCorMoIdQualifierSet(ClCorMOIdPtrT this, ClCorMoPathQualifierT qualifier)
{
    CL_FUNC_ENTER();
    

    if (this == NULL)
    {
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (qualifier > CL_COR_MO_PATH_QUALIFIER_MAX)
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    this->qualifier = qualifier;

    CL_FUNC_EXIT();
    return(CL_OK);
}
#endif
