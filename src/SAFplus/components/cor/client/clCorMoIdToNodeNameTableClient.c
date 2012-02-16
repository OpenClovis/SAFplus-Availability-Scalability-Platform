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
 * File        : clCorMoIdToNodeNameTableClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * MOId to Node Name live table - Client APIs.
 *****************************************************************************/
 #include <clCorClient.h>
 #include <clCorRMDWrap.h>
 #include <xdrCorClientMoIdToNodeNameT.h>
 
ClRcT clCorMoIdToNodeNameGet(ClCorMOIdPtrT pMoId, ClNameT* nodeName)
{
    	ClRcT rc;
    	corClientMoIdToNodeNameT tab = {{0}};
	ClUint32T size;

	CL_COR_VERSION_SET(tab.version);
	tab.moId = *pMoId;
	tab.op = COR_MOID_TO_NODE_NAME_GET;

	size = sizeof(corClientMoIdToNodeNameT);
	
	COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_MOID_TO_NODE_NAME_TABLE_OP,
                                     VDECL_VER(clXdrMarshallcorClientMoIdToNodeNameT, 4, 0, 0),
                                     &tab, 
                                     sizeof(corClientMoIdToNodeNameT ),
                                     VDECL_VER(clXdrUnmarshallcorClientMoIdToNodeNameT, 4, 0, 0),
                                     &tab,
                                     &size,
                                     rc);

	if(CL_OK != rc)
    {
		CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nFailed to get nodeName from the server\n", rc);
    }
    else
    {
        /* Got the nodeName successfully. Pass it back */
        *nodeName = tab.nodeName;
    }

	return rc;
}
 
                             // coverity[pass_by_value]
ClRcT clCorNodeNameToMoIdGet(ClNameT nodeName, 
                             ClCorMOIdPtrT  pMoId)
{
    ClRcT rc;
    corClientMoIdToNodeNameT tab= {{0}};
	ClUint32T size;

	CL_COR_VERSION_SET(tab.version);
    /*
     * In anticipation of calls from AMS context.
     */
    if(nodeName.length > strlen(nodeName.value))
        nodeName.length = strlen(nodeName.value);
	tab.nodeName = nodeName;
	tab.op = COR_NODE_NAME_TO_MOID_GET;

	size = sizeof(corClientMoIdToNodeNameT);
	
	COR_CALL_RMD_WITHOUT_ATMOST_ONCE(COR_MOID_TO_NODE_NAME_TABLE_OP,
                                     VDECL_VER(clXdrMarshallcorClientMoIdToNodeNameT, 4, 0, 0),
                                     &tab, 
                                     sizeof(corClientMoIdToNodeNameT ),
                                     VDECL_VER(clXdrUnmarshallcorClientMoIdToNodeNameT, 4, 0, 0),
                                     &tab,
                                     &size,
                                     rc);

	if(CL_OK != rc)
    {
		CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, "\nFailed to get moId from the server\n", rc);
    }
    else
    {
        /* Got the moId successfully. Pass it back */
        *pMoId= tab.moId;
    }

	return rc;
}

