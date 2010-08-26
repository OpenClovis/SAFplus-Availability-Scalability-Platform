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
 * File        : clCorDmMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  Handles Data Manager related routines
 *
 *
 *****************************************************************************/

/*************************************************************************/
/*                                                                       */
/* MODULE: Data Manager                                                  */
/*                                                                       */
/* DATE:12/29/2003                                                       */
/*                                                                       */
/* AUTHOR:Eyal                                                           */
/*                                                                       */
/*   Handles Data Manager related routines                   */
/*                                                                       */
/* Copyright (C) 2002-2009 by OpenClovis Inc. All rights reserved.*/
/* The source code for this program is not published or otherwise        */
/* divested of its trade secrets, irrespective of what has been deposited*/
/* with the U.S. Copyright office.                                       */
/*                                                                       */
/* All Rights Reserved. US Government Users Restricted Rights - Use,     */
/* duplication or dislosure restricted unless granted by formal written  */
/* contract with OpenClovis Inc.                                  */
/*                                                                       */
/*************************************************************************/
/** @pkg cl.cor.dm */

/* FILES INCLUDED */

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <ctype.h>

/** Internal headers **/
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorLog.h"
#include "clCorLlist.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALS */
#ifdef DEBUG
extern ClRcT corObjDbgInit();
extern ClRcT corClassDbgInit();
extern ClRcT corMoTreeDbgInit();
#endif


/* Error check all HT return codes 
 * Install handlers in the hash table on creation/modification/deletion
 * Need to think on multi threaded scenario, as the data can be accessed
 *    from multiple modules
 * Need to remove all checks on the internal API's.  All the external
 *    ones will do validation and internal API's to assume its valid,
 *    these changes for performance.
 */

DataMgr_h   dmGlobal = 0; /* need to fix this on Single/Multi Process */

/** 
 *  Create New Data Manager.
 *
 *  Creates a new Data Manager data structure with the given
 *  memory manager info.
 *
 *  @param mem   Memory Manager Handle
 *
 *  @returns 
 *    DataMgr_h   (non-null) valid Data Manager handle
 *      null(0)    on failure.
 * 
 */
DataMgr_h 
dmCreate(MemMgr_h mem
         )
{
    DataMgr_h tmp = 0;

    CL_FUNC_ENTER();

    /* todo: after R1.0 when memmgr in place, need to add checks for it
     */ 
    tmp = (DataMgr_h) clHeapAllocate(sizeof(DataMgr_t));
    if(tmp != 0) 
      {
        /* init stuff here */
        tmp->mmgr = mem;
        /* TODO: check for return code here */
        HASH_CREATE(&tmp->classTable);
      } 
    else 
      {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
      }  
    
    CL_FUNC_EXIT();  
    return (tmp);
}

/** 
 *  Deletes Data Manager.
 *
 *  Deletes Data Manager and its associated data structures.
 *
 *  @param this          Data Manager Handle
 *
 *  @returns 
 *    CL_OK     on successful deletion.
 * 
 */
ClRcT 
dmDelete(DataMgr_h this
         )
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();

    /* 
     * todo: need to free everything within this. Need to register
     * hashtable delete functions and free all the items within it.
     */
    if(this)
      {
        HASH_FREE(this->classTable);
        clHeapFree(this);
      } else 
        {
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NULL_PTR))); 
          ret = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }
    
    CL_FUNC_EXIT();
    return ret;
}

/** 
 *  Initializes Data Manager.
 *
 *  Inits Data Manager and its objects.
 *  NOTE: Need to pass the persistent pointer, so that objects can be
 *   loaded and inited.  Also, a subtree can be passed and inited
 *
 *  @param none
 *
 *  @returns 
 *    CL_OK     on successful init.
 * 
 */
ClRcT 
dmInit()
{
    ClRcT ret = CL_OK;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ( "DM INIT Entry"));

    /* todo: need to call the memory manager init routine
     * in turn
     */
    if(!dmGlobal) 
      {
        /* init everything  here
         */
        dmGlobal = dmCreate(0);
        
#ifdef DEBUG
        /* Add to debug agent */
        dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
        corObjDbgInit();
        corClassDbgInit();
        corMoTreeDbgInit();
#endif
        /* add to show manager */
        /* showMgrModAdd(COMP_PREFIX, COMP_NAME, dmShow); */
      }
    
    /* if still dmGlobal is not init, then its error scenario 
     */
    if(!dmGlobal)
      {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
        ret = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
      }
    
    CL_FUNC_EXIT();
    return (ret);
}

ClRcT
dmClassFinalize(CORHashKey_h   key, 
            CORHashValue_h classBuf, 
            void *         userArg,
            ClUint32T     dataLength
            )
{
    ClRcT rc = CL_OK;

	CORClass_h corClass = (CORClass_h) classBuf;

	/* Delete the attrList (Hash), objFreeList, attrs and objects vector */
	HASH_FINALIZE(corClass->attrList);
    COR_LLIST_FINALIZE(corClass->objFreeList, rc);
    if (rc != CL_OK && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
    {
        clLogError("DMC", "FIN", "Failed to finalize obj free list. rc [0x%x]", rc);
    }

	corVectorRemoveAll(&corClass ->attrs);
	corVectorRemoveAll(&corClass ->objects);
        	
	return CL_OK;
}

ClRcT
dmClassDataCleanUp(CORHashKey_h   key, 
            CORHashValue_h classBuf, 
            void *         userArg,
            ClUint32T     dataLength
            )
{
	ClUint32T* node = (ClUint32T*)classBuf;
        clHeapFree(node); 	
	return CL_OK;
}

void dmFinalize(void)
{
	/* Walk through all classes and */
	/* 1. Delete Objects and attributes for the classes */
	/* 2. Delete attributes of the classes */

	if(dmGlobal)
		{
		HASH_ITR_ARG(dmGlobal->classTable, dmClassFinalize, NULL, 0);
		HASH_ITR_ARG(dmGlobal->classTable, dmClassDataCleanUp, NULL, 0);
		HASH_FINALIZE(dmGlobal->classTable);
		clHeapFree(dmGlobal);
		dmGlobal = NULL;
		}
	
}

/** 
 * Pack all meta structure.
 *
 * API to pack all the metadata (class information). The packing is
 * done in the buffer passed and the total size of the packed data is
 * returned by the API.
 *
 * @param buf    buffer holding the packed class information.
 * 
 * @returns 
 *    size   size of the packed data.
 *
 */
ClUint32T
dmClassesPack(void * buf
              )
{
    ClUint32T size = 0;
    void *  packBuf = buf;
    void ** userArg = &packBuf;

    CL_FUNC_ENTER();
  
    if(dmGlobal && buf) 
      {
        HASH_ITR_ARG(dmGlobal->classTable, dmClassPack, (void *) userArg,0);
        size = (char *)packBuf - (char *) buf;
        
#ifdef DEBUG
        dmBinaryShow("Meta Data", buf, size);
#endif
      }

    CL_FUNC_EXIT();
    return size;
}


/** 
 * UnPack all meta structure.
 *
 * API to unpack.
 *
 *  @param this       object handle
 *  @param 
 * 
 *  @returns 
 *    ClRcT  CL_OK on success <br>
 *     CL_COR_SET_RC(CL_COR_ERR_NULL_PTR) on null parameter.
 *
 *  @todo      
 *
 */
ClRcT
dmClassesUnpack(void *     contents, 
                ClUint32T size
                )
{
    ClUint32T i = 0;
    Byte_h  buf = (Byte_h)contents;
    CORClass_h tmp = 0;      
    CORClass_h ch = 0;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
  
    /* walk thru all the data manager classes and unpack them
     */
    if(dmGlobal && buf) 
      {
#ifdef DEBUG
        dmBinaryShow("Meta Data", buf, size);
#endif
        
        /* loop thru the size and try to unpack it
         */
        while(i < size) 
          {
            i += dmClassUnpack(buf+i, &tmp);
            
            /* check if the class definition already present. If so,
             * then check if its identical, otherwise, return back
             * error!!!
             */
            ch = dmClassGet(tmp->classId);
            if(ch != 0) 
            {
                if( (rc = dmClassCompare(ch, tmp) ) != CL_OK) 
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Class %04x already defined here! There is a mismatch in the definition arrived!",
                                          tmp->classId));
                    CL_FUNC_EXIT();
                    return rc;
                } 
                else 
                {
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Class %04x definition is identical here! Merged!!",
                                          tmp->classId));
                    /* If the class has no instance at the receiver side but has instance at the
                       sender side, replace the size of receiver class with that of the sender class.
                       Also, initialize the objects field. Otherwise, it will cause problem during object unpack.
                    */
                    if(ch->objects.head == 0 && tmp->objects.head != 0)
                    {
                        ch->size = tmp->size;
                        corVectorInit(&ch->objects,
                                    ch->size+sizeof(CORInstanceHdr_t),
                                    ch->objBlockSz);
                    }

                    corVectorRemoveAll(&tmp->attrs);
                    corVectorRemoveAll(&tmp->objects);
                    HASH_FREE(tmp->attrList);
                    COR_LLIST_FREE(tmp->objFreeList);
                    clHeapFree(tmp);
                  }
            } 
            else 
              {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "New class definition %04x created!",
                                      tmp->classId));
                /* add to the hash table
                 */
                HASH_PUT(dmGlobal->classTable, tmp->classId, tmp);
              }
          }
        
      }
    
    CL_FUNC_EXIT();
    return CL_OK;
}

/** 
 * Show data manager internals.
 *
 * API to show dm classes and objects.
 *
 *  @returns 
 *    none
 *
 *  @todo      
 *
 */
void 
dmShow(char* params,ClBufferHandleT *pMsg)
{
    ClCharT corStr[CL_COR_CLI_STR_LEN];

    /* walk thru all the data manager classes and dump them
     */
    if(dmGlobal)
      {
        if(params)
          {
            corStr[0]='\0';
            sprintf(corStr, "\n *** Command:show dm [%s]\n", params);
            clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

            
            if(!strcmp(params,"all")) 
              {
                corStr[0]='\0';
                sprintf(corStr, "\r\nClasses and Objects Defined:\r\n");
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
/*                HASH_ITR(dmGlobal->classTable, dmClassShow);*/
                HASH_ITR_COOKIE(dmGlobal->classTable, dmClassShow,pMsg);
              }
            else if(isdigit(*params))
              {
                ClCorClassTypeT classId;
                CORClass_h  class;

		/* Get the class value , take care of hex/decimal */
		if(strlen(params)<=2)
		classId = atoi(params);
		else 
		{
  		     if(!strncmp(params, "0x", 2) || 
				      !strncmp(params, "0X", 2))
      		      {
			      sscanf(params,"%x",(ClUint32T *)&classId);
		      }
		      else classId = atoi(params);
                }
		
                class=dmClassGet(classId);
                dmClassShow(0, (CORHashValue_h) class, pMsg,sizeof(pMsg));
              }
          }
          else
            {
              corStr[0]='\0';
              sprintf(corStr, "\nUsage: [all|<hexClassId>]\n");
              clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                              strlen(corStr));
              /* print the default verbose output
               */
               HASH_ITR_COOKIE(dmGlobal->classTable, dmClassVerboseShow, pMsg);
            }
      }
}
