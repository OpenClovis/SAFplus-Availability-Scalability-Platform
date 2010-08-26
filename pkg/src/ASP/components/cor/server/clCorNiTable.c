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
 * File        : clCorNiTable.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This module contains routines to manage name to class id mappings.
 *
 *
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clIocApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <netinet/in.h>

/* Internal Header*/
#include "clCorNiLocal.h"
#include "clCorNiIpi.h"
#include "clCorClient.h"
#include "clCorDefs.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorRMDWrap.h"

#include <xdrCorNiOpInf_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

#ifdef DEBUG
extern ClRcT corPreProvDbgInit();
#endif

/* Global variables */
extern ClUint32T gCorSlaveSyncUpDone;
static corNIInfo_t  gCorNiInfo; 


/* All static functions */
static ClRcT    corNIClassShow(ClCntKeyHandleT    userKey, ClCntDataHandleT   name,
                      ClCntArgHandleT    userArg, ClUint32T            dataLength);
/*
static ClRcT    corNIAttrShow(ClCntKeyHandleT    userKey, ClCntDataHandleT   name,
                      ClCntArgHandleT    userArg, ClUint32T            dataLength);*/
static ClInt32T corNIClassCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void      corNIClassFree(ClCntKeyHandleT userKey, ClCntDataHandleT userData);
static void      corNiAtttrFree(ClCntKeyHandleT userKey, ClCntDataHandleT userData);
static corClassName_t  *corClassNameAlloc();

static ClUint32T  corNiClassPack(ClCntNodeHandleT  nodeHdl,
                                  char             *pBuf,
                                  ClUint32T       *pLen);
static ClRcT       corNIAttrPack (ClCntKeyHandleT   attrId, 
                                   ClCntDataHandleT  name,
                                   ClCntArgHandleT   pData,
                                   ClUint32T           dataLength);
extern ClCorInitStageT  gCorInitStage ;
/* FILE *fp = NULL; */
/***************************************************************************** 
******          RMD client stubs for NI table access (Start)           ****** 
******************************************************************************/



ClRcT  VDECL(_corNIOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corNiOpInf_t *pData = NULL;

   if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
   {
       clLogError("NIT", "EOF", "The COR server Initialization is in progress....");
       return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
   }

    pData = clHeapAllocate(sizeof(corNiOpInf_t));
    if(pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);   
    }
   if((rc = VDECL_VER(clXdrUnmarshallcorNiOpInf_t, 4, 0, 0)(inMsgHandle, (void *)pData)) != CL_OK)
	{
			clHeapFree(pData);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corNiOpInfo_t"));
			return rc;
	}
 
	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pData->op)
    {
         case  COR_NI_OP_CLASS_NAME_SET:
               if((rc = _corNIClassAdd(pData->name, pData->classId)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to set class name, rc = %x", rc) );
               }
               break;

         case  COR_NI_OP_CLASS_NAME_GET:
               if((rc = _corNiKeyToNameGet(pData->classId, pData->name)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to get class name, rc = %x", rc) );
               }

               /* Write to the message*/
			   VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0)(pData, outMsgHandle, 0); 
               break;

         case  COR_NI_OP_CLASS_ID_GET:
               if((rc = _corNiNameToKeyGet(pData->name, &pData->classId)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to get class id, rc = %x", rc) );
               }
               /* Write to the message*/
               /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)&pData->classId, sizeof(ClCorClassTypeT)); */
			   /* clXdrMarshallClCorClassTypeT(&pData->classId, outMsgHandle, 0);  */
			   VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0)(pData, outMsgHandle, 0); 
               break;

         case  COR_NI_OP_CLASS_NAME_DELETE:
               if((rc = _corNIClassDel(pData->classId)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to delete attribute id, rc = %x", rc) );
               }
               break;

         case  COR_NI_OP_ATTR_NAME_SET:
               if((rc =  _corNIAttrAdd(pData->classId, pData->attrId, pData->name)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to set attribute name, rc = %x", rc) );
               }
               break;

         case  COR_NI_OP_ATTR_NAME_GET:
               if((rc = _corNIAttrNameGet(pData->classId, pData->attrId, pData->name)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to get attribute name, rc = %x", rc) );
               }
               /* Write to the message*/
               /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)pData->name, sizeof(pData->name)); */
				/* clXdrMarshallArrayClCharT(pData->name, sizeof(pData->name), outMsgHandle, 0);*/			    
				VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0)(pData, outMsgHandle, 0);				   
               break;
    
         case  COR_NI_OP_ATTR_ID_GET:
               if((rc = _corNIAttrKeyGet(pData->classId, pData->name, &pData->attrId)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to get attribute id, rc = %x", rc) );
               }
               /* Write to the message*/
               /* clBufferNBytesWrite (outMsgHandle, (ClUint8T *)&pData->attrId, sizeof(ClCorAttrIdT)); */
			   /* clXdrMarshallClCorAttrIdT(&pData->attrId, outMsgHandle, 0); */
			   VDECL_VER(clXdrMarshallcorNiOpInf_t, 4, 0, 0)(pData, outMsgHandle, 0);
               break;

         case  COR_NI_OP_ATTR_NAME_DELETE:
               if((rc = _corNIAttrDel(pData->classId, pData->attrId)) != CL_OK)
               {
                  CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "Failed to delete attribute id, rc = %x", rc) );
               }
               break;
               
         default:
                CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "INVALID OPERATION, rc = %x", rc) );
                rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
         break;
    }

    if (pData->op == COR_NI_OP_CLASS_NAME_SET ||
            pData->op == COR_NI_OP_CLASS_NAME_DELETE ||
            pData->op == COR_NI_OP_ATTR_NAME_SET ||
            pData->op == COR_NI_OP_ATTR_NAME_DELETE)
    {
        if (gCorSlaveSyncUpDone == CL_TRUE)
        {
            ClRcT retCode = CL_OK;

            retCode = clCorSyncDataWithSlave(COR_EO_NI_OP, inMsgHandle);
            if (retCode != CL_OK)
            {
                clLogError("SYNC", "", "Failed to sync data with slave COR. rc [0x%x]", retCode);
                /* Ignore the error code. */
            }
        }
    }

	clHeapFree(pData);
    return rc;
}


/**
 *  Initialize the COR name interface library.
 *
 *  This API intializes the COR name interface (NI) library. This needs to
 *    be called before any Ni function can be used.
 *
 *  @param initialMapping: Name to class id mappings supplied at the
 *    init time.
 *  @param maxEntries: Maximum number of name to class entries.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM) - maxEntries is zero.
 *           CL_COR_SET_RC(CL_COR_ERR_NO_MEM) - Not enough memory to allocate TXN resources.
 */
ClRcT
corNiInit()
{
	ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
#ifdef DEBUG
    rc= dbgAddComponent(COMP_PREFIX, "corNi", COMP_DEBUG_VAR_PTR);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
        return rc;
    }
    /*
    As of now added this fn which adds PreProvisioning component to 
    Debug Infra , but this fn needs to be called from appropriate place 
    */
    rc=corPreProvDbgInit();
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("corPreProvDbgInit Failed \n "));
        CL_FUNC_EXIT();
        return rc;
    }
#endif
    if (CL_OK!= (rc = clCntLlistCreate(corNIClassCompare,
                                           corNIClassFree,
                                           corNIClassFree,
	                                       CL_CNT_UNIQUE_KEY,
                                           &gCorNiInfo.niList)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to create translation table"));
        CL_FUNC_EXIT();
        return rc;
    }
 
    CL_FUNC_EXIT();
    return rc; 
}

/* Function to delete entries from NI table and delete the table itself  */
void corNiFinalize(void)
{
	clCntDelete(gCorNiInfo.niList);
}
/**
 *  Add an entry into the name interface table.
 *
 *  This API adds an entry into the name interface table.
 *   This entry is used to convert a name into class id and 
 *   vice versa.
 *
 *  @param name: character name.
 *  @param name: equivalent class id.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) - name is NULL.
 *           CL_COR_SET_RC(CL_COR_ERR_NO_RESOURCE) - Name entry is out of space.
 */
ClRcT
_corNIClassAdd(char *name, ClCorClassTypeT classId)
{
    corClassName_t   *pClsName = NULL;
    ClRcT           rc = CL_OK;
    CL_FUNC_ENTER();

    if (NULL == name)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNULL pointer pass as input"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

   /* If class already exists, then set the name here itself and return.*/
    if (CL_OK == (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId, (ClCntDataHandleT *)&pClsName)))
    {
        clLogDebug("SER", "INT", "Class [%d] with name [%s] already exists", classId, name);

          memset(pClsName->name, '\0', sizeof(pClsName->name));
          strcpy(pClsName->name, name);
          return rc;
    }

    if (NULL  == (pClsName = corClassNameAlloc()))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Memory Allocation failed"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    strcpy(pClsName->name, name);
    clLogDebug("SER", "INT", "Adding classname [%s] to NIINFOLIST", pClsName->name);
    if (CL_OK != (rc = clCntNodeAdd(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId, (ClCntDataHandleT)pClsName, NULL)))
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to add an entry in the NI Table"));
            CL_FUNC_EXIT();
            return rc;
        }
	else 
            rc = CL_OK;
    }
    CL_FUNC_EXIT();
    return rc; 
}

/**
 *  Delete an entry from the name interface table.
 *
 *  This API deletes an entry from the name interface table.
 *
 *  @param name: character name.
 *
 *  @returns CL_OK  - Success<br>
 *    CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) - name is NULL.
 *    CL_COR_SET_RC(CL_COR_UTILS_ERR_MEMBER_NOT_FOUND) - entry with the name not found.
 */
ClRcT
_corNIClassDel(ClCorClassTypeT classId)
{
    ClRcT            rc = CL_OK;
    ClCntNodeHandleT   nodeHdl = 0;

    CL_FUNC_ENTER();
    if (CL_OK != (rc = clCntNodeFind(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId, &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nFailed to delete entry in the NI Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    if (CL_OK != (rc = clCntNodeDelete(gCorNiInfo.niList, nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to delete entry in the NI Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
	CL_FUNC_EXIT();
    return rc;
}

/**
 *  Get class id given a name.
 *
 *  This API returns the class id given the class name.
 *
 *  @param name: character name.
 *  @param key: [OUT] class id.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) - name or key is NULL.
 */
ClRcT
_corNiNameToKeyGet(char *name, ClCorClassTypeT *pKey)
{
    ClCntNodeHandleT  nodeHdl = 0;
    corClassName_t   *pClsName = NULL;
    ClRcT           rc = CL_OK;
    ClCntKeyHandleT hnd = 0; 
    CL_FUNC_ENTER();

    if ((NULL == name) || ( NULL == pKey))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nNULL pointer pass as input"));
		CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (CL_OK != (rc = clCntFirstNodeGet(gCorNiInfo.niList, &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Ni Table is Empty [rc 0x %x]", rc));
        goto exitOnError;
    }

    while (nodeHdl != 0)
    {
        if (CL_OK != (rc = clCntNodeUserDataGet(gCorNiInfo.niList, nodeHdl, (ClCntDataHandleT *) &pClsName)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Usr data get failed [rc 0x %x]", rc));
            goto exitOnError;
        }
        if (!strcasecmp(name, pClsName->name))
        {
            if (CL_OK != (rc = clCntNodeUserKeyGet(gCorNiInfo.niList, 
                                             nodeHdl, &hnd)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Could not get key from Node [rc 0x %x]", rc));
                goto exitOnError;
            }
            *pKey = (ClCorClassTypeT)(ClWordT)hnd;
            return CL_OK;
        }
        if (CL_OK != (rc = clCntNextNodeGet(gCorNiInfo.niList, nodeHdl , &nodeHdl)))
        {
            if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Next node Get for NI table failed [rc 0x %x]", rc));
                goto exitOnError;
            }
        }
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nERROR: entry not found") );
	CL_FUNC_EXIT();
	return CL_COR_SET_RC(CL_COR_UTILS_ERR_INVALID_KEY);
exitOnError:
    CL_FUNC_EXIT();
    return rc;
}

/**
 *  Get name given a class id.
 *
 *  This API returns the name given a class id.
 *
 *  @param key: class id.
 *  @param name: [OUT] character name.
 *
 *  @returns CL_OK  - Success<br>
 *           CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) - name is NULL.
 */
ClRcT
_corNiKeyToNameGet(ClCorClassTypeT key, char *name )
{
    corClassName_t   *pClsName = NULL;
    ClRcT            rc = CL_OK;

    CL_FUNC_ENTER();
    if (NULL == name)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nNULL pointer pass as input"));
		CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)key, (ClCntDataHandleT *)&pClsName)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nERROR: entry not found for key (%d) [rc 0x %x]", key,rc) );
        CL_FUNC_EXIT();
        return rc;
    }
    strcpy(name, pClsName->name);
    return rc;
}

/**
 *  Show the contents of the name to class id table.
 *
 *  This API shows the contents of the name to class id table.
 *
 *  @returns N/A
 */
void
corNiTableShow(ClBufferHandleT *pMsgHdl)
{
    CL_FUNC_ENTER();
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    corStr[0]='\0';
    sprintf(corStr, "\n Class Id                       Class Name\n"
                      " ========                       ==========");
                                                                                                                             
    clBufferNBytesWrite (*pMsgHdl, (ClUint8T*)corStr,
                                strlen(corStr));

    clCntWalk(gCorNiInfo.niList,corNIClassShow, pMsgHdl, sizeof(ClBufferHandleT) );  
    return;
	CL_FUNC_EXIT();
}
/*
 *  Internal routine to show one entry in NI table
 */
ClRcT corNIClassShow(ClCntKeyHandleT    userKey, 
                      ClCntDataHandleT   name,
                      ClCntArgHandleT    userArg, 
                      ClUint32T            dataLength)
{
    corClassName_t   *pCls = NULL;
    ClBufferHandleT* pMsg = (ClBufferHandleT* )userArg; 
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    pCls = (corClassName_t *)name;
    if (name != 0) 
    {
        corStr[0]='\0';
        sprintf(corStr, "\n %-20p           %-20s",(ClPtrT)userKey, pCls->name);

        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                    strlen(corStr));
        /* clCntWalk(pCls->attrList, corNIAttrShow, NULL, 0);  */
    }
    return CL_OK;
}

/*
 *  Internal routine to show one entry in NI table
 */
ClInt32T corNIClassCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

/*
 *  Internal routine to free the memory of an NI table entry
 */
void  corNIClassFree(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{ 
    corClassName_t *pClsName = NULL;

    if (userData != 0) 
    {
	pClsName = (corClassName_t *) userData;
	clCntDelete(pClsName->attrList);
        clHeapFree(pClsName);
	
    }
    return;
}

/*
 *  Internal routine to free the memory of an NI table entry
 */
void  corNiAtttrFree(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    char *name = NULL;
    if (userData != 0) 
    {
        name = (char *)userData;
        clHeapFree(name);
    }
    return;
}

/*
 *  Internal routine to pack the NI Table
 */
ClUint32T  corNiTablePack(void** pBuf, ClUint32T   *pLen)
{
    corNiTblPkt_t    *pPkt = NULL;
    char             *pData = NULL;
    ClCntNodeHandleT  nodeHdl = 0;
    ClUint32T       nodesCount = 0;
    ClUint32T       pktLen = 0;
    ClRcT           rc = CL_OK;

    CL_FUNC_ENTER();

    /* Validations */
    if ((NULL == pBuf ) || (NULL == pLen))
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    /* Get the number of nodes */
    if (CL_OK != (rc = clCntSizeGet(gCorNiInfo.niList, &nodesCount)))
    {
        goto exitOnError;
    }

    if (nodesCount == 0)  /* A rare case */
    {
        *pBuf = NULL;
        *pLen = 0;
        return CL_OK;
    }
 
    pktLen  = (nodesCount * sizeof(corNiPktCls_t)) +
              (gCorNiInfo.niAttrCount * sizeof(corNiPktAttr_t)) +
              sizeof(corNiTblPkt_t); 
 
    /* Allocate memory for Packet header + data*/
    if (NULL == (pPkt = (corNiTblPkt_t *) clHeapAllocate(pktLen)))
    { 
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nERROR: Memory Allocation failed "));
        goto exitOnError;
    }
   
    /* Here is the pkt format  
    ________________________________________________________
    |      |     |          |         |       |      |      |
    | SIGN | LEN |ClassCount| Class1  | Class2|......|Classn|
    |      |     |          |         |       |      |      |
    ________________________________________________________
                            /          \
                           /            \
                           _____________
                           |Class Id   |
                           |Class Name |
                           |Attr Count |
                           |__________ |____________________________
                           |Attr Entry1| Attr Id + Attr Name        |
                           |___________|____________________________
                           |Attr Entry2| Attr Id + Attr Name        |
                           |___________|____________________________
                           |...........|
                           |...........|
                           |...........|
                           |Attr Entryn|
                           _____________
            

               
    */
    /* Safety precautions */
    pPkt->signature = htonl(COR_NI_PKT_SIGNATURE);
    pPkt->len       = htonl(pktLen);
    pPkt->clsCount  = htonl(nodesCount); 

    pData = pPkt->data;

    /* Get the first node */
    if (CL_OK != (rc = clCntFirstNodeGet(gCorNiInfo.niList, &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Ni Table is Empty [rc 0x %x]", rc));
        goto exitOnError;
    }

    while (nodeHdl != 0) /* As long as nodes are there */
    {
        ClUint32T     clsLen = 0;
        if (CL_OK != (rc = corNiClassPack(nodeHdl, pData, &clsLen)))
        {
           goto exitOnError;
        }
        pData  = pData + clsLen;
        /* Go to the next node */
        if (CL_OK != (rc = clCntNextNodeGet(gCorNiInfo.niList, nodeHdl , &nodeHdl)))
        {
            if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Next node Get for NI table failed [rc 0x %x]", rc));
                goto exitOnError;
            }
            else /* All done */
                break;
        }
    }

    *pBuf =  (void  *)pPkt;
    *pLen =  pktLen; 
    /* *pLen =  htonl(pktLen);  */
    return CL_OK;

exitOnError:
    {
        if (NULL != pPkt) clHeapFree(pPkt);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nERROR: Could not pack the Ni Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

}

/*
 *  Internal routine to pack class Entry in the NI Table
 */
ClUint32T  corNiClassPack(ClCntNodeHandleT  nodeHdl,
                           char             *pBuf,
                           ClUint32T       *pLen)
{
    corClassName_t   *pClsName = NULL; 
    char             *pData = NULL; 
    corNiPktCls_t    *pEntry = (corNiPktCls_t *)pBuf;
    ClRcT           rc = CL_OK;
    ClCntKeyHandleT  classKey = 0;
	/* ClUint32T classId, attrCount; */

    if (CL_OK != (rc = clCntNodeUserDataGet(gCorNiInfo.niList, nodeHdl, (ClCntDataHandleT *) &pClsName)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Usr data get failed [rc 0x %x]", rc));
        goto exitOnError;
    }
    /* Pack the data */
    if (CL_OK != (rc = clCntNodeUserKeyGet(gCorNiInfo.niList, nodeHdl, 
                                   (ClCntKeyHandleT*)&classKey)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Usr key get failed [rc 0x %x]", rc));
        goto exitOnError;
    }

    pEntry->classId = (ClCorClassTypeT)(ClWordT)classKey;
    
	pEntry->classId = htonl(pEntry->classId);
    strcpy(pEntry->name, pClsName->name);
    if (CL_OK != (rc = clCntSizeGet(pClsName->attrList, (void *)&pEntry->attrCount))) 
    {
        goto exitOnError;
    }
    pData = pEntry->attrData;
	
    if (pEntry->attrCount != 0)
    {
        if (CL_OK != (rc =clCntWalk(pClsName->attrList, corNIAttrPack, &pData , 0)))
        {
            goto exitOnError;
        }
    }
    *pLen =  sizeof (corNiPktCls_t) + (pEntry->attrCount * sizeof(corNiPktAttr_t));
	pEntry->attrCount = htonl(pEntry->attrCount);
    return CL_OK;

exitOnError:
    return rc;
}

/*
 *  Internal routine to pack an attribute in the NI Table
 */
ClRcT corNIAttrPack (ClCntKeyHandleT   attrId, 
                      ClCntDataHandleT  name,
                      ClCntArgHandleT   pData,
                      ClUint32T           dataLength)
{
    corNiPktAttr_t   **ppAttr = (corNiPktAttr_t **)(pData);

    if (NULL == pData)
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);

    (*ppAttr)->attrId = htonl((ClCorAttrIdT)(ClWordT)attrId);
    strcpy((*ppAttr)->name, (char *)name);
    (*ppAttr) ++;
    return CL_OK;
}

/*
 *  Internal routine to unpack the NI Table
 */
ClUint32T  corNiTableUnPack(void *pBuf, ClUint32T  len)
{
    corNiTblPkt_t    *pPkt = NULL;
    corNiPktCls_t    *pEntry = NULL;
    corNiPktAttr_t   *pAttr = NULL;
    char             *pData = NULL;

    ClUint32T       nodesCount = 0;
    ClRcT           rc = CL_OK;

    /* Validations */
    if (NULL == pBuf)
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    
    pPkt = (corNiTblPkt_t *)pBuf;

    if ((ntohl(pPkt->signature) != COR_NI_PKT_SIGNATURE) || (ntohl(pPkt->len) != len))
    /* if ((pPkt->signature != COR_NI_PKT_SIGNATURE) || (pPkt->len != len)) */
    {
        rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_BUFFER);
        goto exitOnError;
    }

    /* Looks like trustworthy packet! Proceed further */
    pData = pPkt->data;

    /* How many nodes ?*/
    nodesCount = ntohl(pPkt->clsCount);
    /* nodesCount = pPkt->clsCount; */

    pEntry  = (corNiPktCls_t *)(pData);
    for (;nodesCount > 0; nodesCount--)
    {
        corClassName_t    *pCls = NULL;
        ClUint32T        attrCount = 0;

        if (CL_OK != (rc = _corNIClassAdd(pEntry->name, ntohl(pEntry->classId)))) 
        /* if (CL_OK != (rc = _corNIClassAdd(pEntry->name, pEntry->classId))) */
        {
            goto exitOnError;
        }

        /* Get the Class */
        if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)ntohl(pEntry->classId), 
        /* if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, pEntry->classId, */
                                               (ClCntDataHandleT *)&pCls)))
        {
            goto exitOnError;
        }

        pAttr = (corNiPktAttr_t *)pEntry->attrData;
        /* Add all the attributes */
        for (attrCount = ntohl(pEntry->attrCount); attrCount > 0; attrCount--) 
        /* for (attrCount = pEntry->attrCount; attrCount > 0; attrCount--) */
        {
            if (NULL == pAttr)
                goto exitOnError;
#if 0
            /* Allocate memory for AttrName */
            if (NULL == (attrName = (char *)clHeapAllocate(CL_COR_MAX_NAME_SZ)))
            {
		clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get the Class[rc x%x]", rc));
                goto exitOnError;
            }
            memset(attrName, '\0', CL_COR_MAX_NAME_SZ);

            /* Copy the name */
            strcpy(attrName, pAttr->name);

            /* Add the attribute */

            if (CL_OK != (rc = clCntNodeAdd(pCls->attrList, pAttr->attrId, 
                                            (ClCntDataHandleT)attrName, NULL)))
            {
                clHeapFree(attrName);
                if (CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE )
                {
                    goto exitOnError;
                }
            }
#endif
	rc = _corNIAttrAdd(ntohl(pEntry->classId), ntohl(pAttr->attrId), pAttr->name);
	/* rc = _corNIAttrAdd(pEntry->classId, pAttr->attrId, pAttr->name); */

	if(CL_OK != rc)
		{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get the Class[rc x%x]", rc));
		goto exitOnError;
		}
	
            pAttr++;
        } /* End of for (attr loop)*/
        pData = pData  + sizeof (corNiPktCls_t) + (sizeof(corNiPktAttr_t) *  ntohl(pEntry->attrCount));
        /* pData = pData + sizeof (corNiPktCls_t) +(sizeof(corNiPktAttr_t) *  pEntry->attrCount); */
        pEntry  = (corNiPktCls_t *)pData;
    } /* End of for (class loop ) */
    return CL_OK;
exitOnError:
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nERROR: Could not unpack the Ni Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
}

/* 
 *   Thi routine is for internal usage. 
 *   Allocate memory and inistalise the list.
 */
corClassName_t  *corClassNameAlloc()
{
    corClassName_t   *pClsName = NULL;
    ClRcT           rc=CL_OK;

    CL_FUNC_ENTER();

    /* Allocate memory for the structure */
    if (NULL == (pClsName = clHeapAllocate(sizeof(corClassName_t))))
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nERROR: Could not unpack the Ni Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return NULL;
    }
    memset(pClsName, '\0', sizeof(corClassName_t)); /* Clean slate */

    /* Create a linked list for attributes */
    if (CL_OK!= (rc = clCntLlistCreate(corNIClassCompare,
                                           corNiAtttrFree,
                                           corNiAtttrFree,
	                                       CL_CNT_UNIQUE_KEY,
                                           &pClsName->attrList)))
    {
        clHeapFree(pClsName);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to attribute names table [rc x%x]", rc));
        CL_FUNC_EXIT();
        return NULL;
    }
    return pClsName;
}

/*
 *    Add a name entry for an attribute
 */
ClRcT  _corNIAttrAdd(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name)
{
    corClassName_t    *pCls = NULL;
    char               *attrName = NULL;
    ClRcT             rc = CL_OK;

    CL_FUNC_ENTER();

    if(strlen(name) >= CL_COR_MAX_NAME_SZ)
    {
        clLogError("ATTR", "ADD", "Attribute length [%d] defined in the config file exceeds the "
                   "supported limit of [%d]. Please change the value of CL_COR_MAX_NAME_SZ macro "
                   "to accomodate this attribute", (ClUint32T)strlen(name), 
                   (ClUint32T)CL_COR_MAX_NAME_SZ-1);
        return CL_COR_SET_RC(CL_ERR_OUT_OF_RANGE);
    }

    /* Get the Class */
    if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId,
                                           (ClCntDataHandleT *)&pCls)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get the Class[rc x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* If Attribute already exists, then set the name here itself and return.*/
    if (CL_OK == (rc = clCntDataForKeyGet(pCls->attrList, (ClPtrT)(ClWordT)attrId,
                                    (ClCntDataHandleT *) &attrName)))
    {
      memset(attrName, '\0', CL_COR_MAX_NAME_SZ);
      strcpy(attrName, name);
      return rc;
    }

    /* Allocate memory for AttrName */
    if (NULL == (attrName = (char *)clHeapAllocate(CL_COR_MAX_NAME_SZ)))
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM))); 
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset(attrName, '\0', CL_COR_MAX_NAME_SZ);

    /* Copy the user supplied name */
    strcpy(attrName, name);

    /* Add the attribute */
    if (CL_OK != (rc = clCntNodeAdd(pCls->attrList, (ClPtrT)(ClWordT)attrId, 
                                    (ClCntDataHandleT)attrName, NULL)))
    {
        clHeapFree(attrName);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get the Class[rc x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    gCorNiInfo.niAttrCount ++;
    CL_FUNC_EXIT();
    return rc;
}

/*
 *    Delete a name entry for an attribute
 */
ClRcT  _corNIAttrDel(ClCorClassTypeT  classId, ClCorAttrIdT  attrId)
{
    corClassName_t     *pCls = NULL;
    ClCntNodeHandleT    nodeHdl = 0;
    ClRcT             rc = CL_OK;
    
    CL_FUNC_ENTER();

    /* Get the class  */
    if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId,
                                           (ClCntDataHandleT *)&pCls)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get the Class[rc x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* Get the attribute */
    if (CL_OK != (rc = clCntNodeFind(pCls->attrList, (ClPtrT)(ClWordT)attrId, &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nFailed to Find the attribute in the NI Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* Delete the attr */
    if (CL_OK != (rc = clCntNodeDelete(pCls->attrList, nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to delete attribute entry in the NI Table [rc 0x %x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    gCorNiInfo.niAttrCount --;
    CL_FUNC_EXIT();
    return rc;
}
   

/*
 *   Get the name of an attribute 
 */
ClRcT  _corNIAttrNameGet(ClCorClassTypeT  classId, ClCorAttrIdT  attrId, char *name)
{
    corClassName_t    *pCls = NULL;
    char               *attrName = NULL;
    ClRcT             rc = CL_OK;

    CL_FUNC_ENTER();

    /* Get the Class */
    if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId,
                                           (ClCntDataHandleT *)&pCls)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get the Class[rc x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* Get the Attrbute name */
    if (CL_OK != (rc = clCntDataForKeyGet(pCls->attrList, (ClPtrT)(ClWordT)attrId,
                                           (ClCntDataHandleT *)&attrName)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get the Attribute 0x %x[rc 0x%x]", attrId, rc));
        CL_FUNC_EXIT();
        return rc;
    }
    /* Copy the name */
    strcpy(name, attrName);
    CL_FUNC_EXIT();
    return rc;
}

/*
 *   Get the Id of an attribute 
 */
ClRcT  _corNIAttrKeyGet(ClCorClassTypeT  classId, char *name,  ClCorAttrIdT  *pAttrId)
{
    corClassName_t    *pCls = NULL;
    ClCntNodeHandleT    nodeHdl = 0;
    char               *attrName = NULL;
    ClRcT             rc = CL_OK;
    ClCntKeyHandleT   keyAttrId = 0;

    CL_FUNC_ENTER();
    /* Get the Class */
    if (CL_OK != (rc = clCntDataForKeyGet(gCorNiInfo.niList, (ClPtrT)(ClWordT)classId,
                                           (ClCntDataHandleT *)&pCls)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Failed to get the Class[rc x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    /* Get  the first attribute */
    if (CL_OK != (rc = clCntFirstNodeGet(pCls->attrList, &nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n First node get failed [rc 0x %x]", rc));
        goto exitOnError;
    }

    /* Start fishing */
    while (nodeHdl != 0) /* As long as nodes are there */
    {
        /* Get the data*/
        if (CL_OK != (rc = clCntNodeUserDataGet(pCls->attrList, nodeHdl, (ClCntDataHandleT *) &attrName)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Usr data get failed [rc 0x %x]", rc));
            goto exitOnError;
        }

        /* See if we got the name */
        if (!(strcmp(attrName, name)))
        {
            if (CL_OK != (rc = clCntNodeUserKeyGet(pCls->attrList, nodeHdl, 
                                           (ClCntKeyHandleT*) &keyAttrId)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Usr key get failed [rc 0x %x]", rc));
                goto exitOnError;
            }

            /* Copy the key data */
            *pAttrId = (ClCorAttrIdT)(ClWordT)keyAttrId;
            break;
        }

        /* Go to the next node */
        if (CL_OK != (rc = clCntNextNodeGet(pCls->attrList, nodeHdl , &nodeHdl)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Next node Get for NI table failed [rc 0x %x]", rc));
            goto exitOnError;
        }
    }
    CL_FUNC_EXIT();
    return rc;
exitOnError:
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\n Attribute not found [rc 0x %x]", rc));
    return rc;
}
