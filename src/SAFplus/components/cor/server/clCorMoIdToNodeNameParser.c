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
 * File        : clCorMoIdToNodeNameParser.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Parser for MOId to Node Name table.
 *****************************************************************************/

#include <clLogApi.h>
#include <clCorMoIdToNodeNameParser.h>
#include <string.h>
#include <clParserApi.h>
#include <clCorNiLocal.h>
#include <clCorMoIdToNodeNameTable.h>
#include <clCorClient.h>
#include <clCorErrors.h>
#include <clCorLog.h>
#include <clCorUtilityApi.h>
#include <clCorPvt.h>
#include <clCpmApi.h>

#define MAX_SLOTS 64

/* Globals */

static ClRcT corNodeObjectCreate(ClCorMOIdT *nodeMoid)
{
    ClRcT rc = CL_OK;
    ClInt16T depth = clCorMoIdDepthGet(nodeMoid);

    if(depth < 0 ) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

    for(nodeMoid->depth = 1; nodeMoid->depth <= depth; ++nodeMoid->depth)
    {
        clCorMoIdServiceSet(nodeMoid, CL_COR_INVALID_SRVC_ID);
        rc = _clCorUtilMoAndMSOCreate(nodeMoid);
        if(rc != CL_OK)
        {
            clLogError("COR", "CREATE", "Failed to create Node Object. rc [0x%x]", rc);
            return rc;
        }
    }

    return CL_OK;
}

/* This is a high level function which is invoked during COR initialization */
/* This will read the XML file and create entries in the hash tables */
ClRcT clCorMoIdToNodeNameTableFromConfigCreate(void)
{
    ClRcT rc = CL_OK;
    moIdToNodeNameParseTableEntryT  table[MAX_SLOTS] = {{0}};
    ClUint32T numEntries = 0;
    ClInt32T  i = 0;
    ClCorMOIdT moId;
	SaNameT	   nodeName = {0};
    ClUint32T master = 0;
    ClParserPtrT    top = NULL;
    ClCharT         *aspPath = NULL;

    clLog(CL_LOG_SEV_TRACE, "MNT", "PRS", "Entering [%s]", __FUNCTION__);

    /* First Initialize the table in our code */
    rc = clCorMoIdNodeNameMapCreate();
    if(CL_OK != rc)
    {
        clLog(CL_LOG_SEV_ERROR,"MNT","PRS", "Could not create MOID to Node Name Map. rc[0x%x]",rc);
        return rc;
    }

    /* Get the handle of the file first */
    aspPath = getenv("ASP_CONFIG");
    if (aspPath != NULL)
    {
        top = clParserOpenFile(aspPath, ASP_CONFIG_FILE_NAME);
        if (top == NULL)
        {
            clLog(CL_LOG_SEV_ERROR,"MNT", "PRS","Error while opening the config file \
                    [%s] at [%s]. rc[0x%x]", ASP_CONFIG_FILE_NAME, aspPath, CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            clDbgIfNullReturn(top, CL_CID_COR);
        }
    }
    else
    {
        clLog(CL_LOG_SEV_ERROR,"MNT", "PRS", CL_LOG_MESSAGE_0_ENV_NOT_SET);
        clDbgIfNullReturn(top, CL_CID_COR); 
    }

    clLog(CL_LOG_SEV_TRACE,"MNT","PRS","Parsing the file [%s] present at [%s]", aspPath, ASP_CONFIG_FILE_NAME);

    master = clCpmIsMaster();

    /* Now  pass the handle for the XML file and get the values of all the entries in the table */
    rc = clCorMoIdToNodeNameTableRead(top, table, &numEntries);

    for(i=0;i<numEntries;i++)
    {
        clLog(CL_LOG_SEV_TRACE, "MNT","PRS","Getting the MoId for the MoId Name [%s]", table[i].MoId);

        rc = corXlateMOPath(table[i].MoId, &moId);
        if(CL_OK != rc)
		{
            clLog(CL_LOG_SEV_ERROR,"MNT","PAR", "Failed to get the \
                    MOId for string [%s]. rc[0x%x]", table[i].MoId, rc);
            return rc;
		}

		memset(&nodeName, 0, sizeof(SaNameT));
		memcpy(nodeName.value, table[i].NodeName, strlen(table[i].NodeName));
		nodeName.length = strlen(table[i].NodeName);

        rc = clCorMOIdNodeNameMapAdd(&moId, &nodeName);
        if(CL_OK != rc)
        {
            clLog(CL_LOG_SEV_ERROR, "MNT", "PRS", "Could not add in MOID to Node Name Map. rc[0x%x]", rc);
            return rc;
        }

        /* Free the memory allocated for moid string */
        clHeapFree(table[i].MoId);
        clHeapFree(table[i].NodeName);
        
        if(master)
        {
            rc = corNodeObjectCreate(&moId);
            if(CL_OK != rc)
            {
                clLogError("MNT", "PRS", "Could not create node moid [%s] for node [%s]",
                           table[i].MoId, table[i].NodeName);
                return rc;
            }
        }
    }

    clLog(CL_LOG_SEV_TRACE, "MNT", "PRS", "Leaving [%s]", __FUNCTION__);

    return CL_OK;
}

ClRcT clCorMoIdToNodeNameTableRead(ClParserPtrT top, moIdToNodeNameParseTableEntryT entries[], ClUint32T * numOfEntries)
{
    ClParserPtrT nodeInsts = NULL;
    ClParserPtrT nodeInst  = NULL;
    int i;
    const char* pMoId = NULL;
    const char* name = NULL;
    
    if( (NULL == top) || (NULL == entries) || (NULL == numOfEntries) )
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "NULL parameter passed", CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));

    /* top is a handle to the file */
    nodeInsts = clParserChild(top, "nodeInstances");

    if(NULL == nodeInsts)
        CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "Error while parsing the file", CL_ERR_UNSPECIFIED);
        
	nodeInst = clParserChild(nodeInsts, "nodeInstance");
    /* Within COR there are multiple maps present. Get them one by one */
    for(i = 0; nodeInst; nodeInst = nodeInst->next, i++)
    {
        name = clParserAttr(nodeInst, "name");
        pMoId = clParserAttr(nodeInst, "nodeMoId");
        if ((NULL == name) || (NULL == pMoId))
        {
            clLogError("MNT", "RED", "The attribute obtained from the config file is NULL");
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

        entries[i].NodeName = clHeapAllocate(strlen(name) + 1);
        entries[i].MoId = clHeapAllocate(strlen(pMoId) + 1);
		if(entries[i].MoId == NULL || entries[i].NodeName == NULL)
		{	
			clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
					CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
			return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
		}
        memcpy(entries[i].MoId, pMoId, strlen(pMoId));
        memcpy(entries[i].NodeName, name, strlen(name));
    }

    *numOfEntries = i;
    
    clParserFree(top);
    return CL_OK;
}
