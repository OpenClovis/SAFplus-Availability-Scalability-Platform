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
 * File        : clCorDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This modules provides the COR's debug function list to debug.
 *****************************************************************************/

#include <clEoApi.h>
#include <clDebugApi.h>
#include <clCorPvt.h>

/* Container to store read session related data */
ClCntHandleT corDbgBundleCnt = CL_HANDLE_INVALID_VALUE;
ClHandleT    gCorDebugReg    = CL_HANDLE_INVALID_VALUE;

extern ClRcT cliCorClassCreate(int argc, char **argv, char** retStr);
extern ClRcT cliCorClassDelete(int argc, char **argv, char** retStr);
extern ClRcT cliCorClassSimpleAttributeCreate(int argc, char **argv,
                                              char** retStr);
extern ClRcT cliCorClassAttributeValueSet(int argc, char **argv,
                                          char** retStr);
extern ClRcT cliCorMOClassCreateByName(int argc, char **argv, char** retStr);
extern ClRcT cliCorMOClassDeleteNodeByName(int argc, char **argv,
                                           char** retStr);
extern ClRcT cliCorMSOClassCreateByName(int argc, char **argv, char** retStr);
extern ClRcT cliCorMSOClassDeleteByName(int argc, char **argv, char** retStr);
extern ClRcT cliCorObjCreate( int argc, char **argv , char** retStr);
extern ClRcT cliCorObjDelete( int argc, char **argv, char** retStr );
extern ClRcT cliCorSet(int argc, char **argv, char** retStr);
extern ClRcT cliCorGet(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleInitialize(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleCreateJobAdd(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleSetJobAdd(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleGetJobAdd(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleDeleteJobAdd(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleApply(int argc, char **argv, char** retStr);
extern ClRcT cliCorBundleFinalize(int argc, char **argv, char** retStr);
extern ClRcT cliCorDMShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorObjTreeShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorMOClassTreeShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorNiShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorRmShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorObjectShow(int argc, char **argv, char** retStr);
extern ClRcT cliCorMoIdToIocAddressGet(int argc, char **argv, char** retStr);
extern ClRcT cliCorIocAddressToMoIdGet(int argc, char **argv, char** retStr);
extern ClRcT cliCorCardEvent(int argc, char **argv, char** retStr);
extern ClRcT cliCorMoAndMsoObjectsCreate(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorMoAndMsoObjectsDelete(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorSubTreeDelete(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorListClasses(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorOIRegister(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorOIUnRegister(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorPrimaryOISet(ClUint32T argc, ClCharT **argv, ClCharT **retStr);
extern ClRcT cliCorPrimaryOIClear(ClUint32T argc, ClCharT **argv, ClCharT **retStr);


static ClDebugFuncEntryT corDebugFuncList[] = {
        {(ClDebugCallbackT)cliCorClassCreate,"corClassCreate", "Creates COR class"},
        {(ClDebugCallbackT)cliCorClassDelete,"corClassDelete", "Deletes COR class"},
        {(ClDebugCallbackT)cliCorClassSimpleAttributeCreate,"corClassAttrCreate", "Create a simple attribute for COR class"},
        {(ClDebugCallbackT)cliCorClassAttributeValueSet,"corClassAttrValSet", "Set the init,min and max value for a class attribute."},
        {(ClDebugCallbackT)cliCorMOClassCreateByName,"moClassCreate", "Create a MO class"},
        {(ClDebugCallbackT)cliCorMOClassDeleteNodeByName,"moClassDelete", "Delete a MO class"},
        {(ClDebugCallbackT)cliCorMSOClassCreateByName,"msoClassCreate", "Create a MSO class"},
        {(ClDebugCallbackT)cliCorMSOClassDeleteByName,"msoClassDelete", "Delete a MSO class"},
        {(ClDebugCallbackT)cliCorObjCreate,"corObjCreate", "Create a Object"},
        {(ClDebugCallbackT)cliCorObjDelete,"corObjDelete", "Delete a Object"},
        {(ClDebugCallbackT)cliCorMoAndMsoObjectsCreate,"corMoMsoObjCreate", "Create MO and associated MSO Objects"},
        {(ClDebugCallbackT)cliCorMoAndMsoObjectsDelete,"corMoMsoObjDelete", "Delete MO and associated MSO Objects"},
        {(ClDebugCallbackT)cliCorSubTreeDelete,"corSubTreeDelete", "Delete a sub-tree in the COR object hierarchy"},
        {(ClDebugCallbackT)cliCorSet,"attrSet", "Set the attribute value"},
        {(ClDebugCallbackT)cliCorGet,"attrGet",  "Get the attribute value"},
        {(ClDebugCallbackT)cliCorBundleInitialize,"corBundleInitialize",  "Initializes a bundle and return a bundle handle"},
        {(ClDebugCallbackT)cliCorBundleCreateJobAdd,"corBundleCreateJobAdd",  "Enqueue the create job in the bundle. "},
        {(ClDebugCallbackT)cliCorBundleSetJobAdd,"corBundleSetJobAdd",  "Enqueue the set job in the bundle. "},
        {(ClDebugCallbackT)cliCorBundleGetJobAdd,"corBundleGetJobAdd",  "Enqueue the get job in the bundle. "},
        {(ClDebugCallbackT)cliCorBundleDeleteJobAdd,"corBundleDeleteJobAdd",  "Enqueue the delete job in the bundle. "},
        {(ClDebugCallbackT)cliCorBundleApply,"corBundleApply",  "Apply all the jobs for this bundle."},
        {(ClDebugCallbackT)cliCorBundleFinalize,"corBundleFinalize",  "Finalize the bundle."},
        {(ClDebugCallbackT)cliCorObjectShow,"objectShow", "Show the object contents"},
        {(ClDebugCallbackT)cliCorDMShow,"dmShow", "Show dm Classes"},
        {(ClDebugCallbackT)cliCorMOClassTreeShow,"moClassTreeShow", "Show dm Classes"},
        {(ClDebugCallbackT)cliCorObjTreeShow,"objTreeShow", "Show Object Tree"},
        {(ClDebugCallbackT)cliCorNiShow,"niShow", "Show Name Interface"},
        {(ClDebugCallbackT)cliCorRmShow,"rmShow", "Show Route Information"},
        {(ClDebugCallbackT)cliCorListClasses,"listClasses", "Show all the COR classes"},
        {(ClDebugCallbackT)cliCorOIRegister, "corOIRegister", "Register the application as an OI for the MO"},
        {(ClDebugCallbackT)cliCorOIUnRegister, "corOIUnRegister", "Unregister the application as an OI for the MO"},
        {(ClDebugCallbackT)cliCorPrimaryOISet, "corPrimaryOISet", "Register the application as Primary OI for the MO"},
        {(ClDebugCallbackT)cliCorPrimaryOIClear, "corPrimaryOIClear", "Unregister the application as Primary OI for the MO"},
        {NULL, "", ""}
};


ClDebugModEntryT clModTab[] = 
{
	{"cor", "cor", corDebugFuncList, "Clovis Object Registry Module Commands"}, 	
	{"", "", 0, ""}
};

static ClInt32T _clCorDbgBundleCmpFn(
    CL_IN       ClCntKeyHandleT         sessHdl1,
    CL_IN       ClCntKeyHandleT         sessHdl2)
{
    return (*(ClCorBundleHandlePtrT)sessHdl1 - *(ClCorBundleHandlePtrT)sessHdl2);
}

static void _clCorDbgBundleNodeDelete(
    CL_IN       ClCntKeyHandleT         key,
    CL_IN       ClCntDataHandleT        data)
{
    clHeapFree((ClPtrT)data);
    clHeapFree((ClCorBundleHandlePtrT)key);
    
    return ;
}

static  void _clCorDbgBundleNodeDestroy(
    CL_IN       ClCntKeyHandleT     key,
    CL_IN       ClCntDataHandleT    data)
{

    ClCorDbgGetBundlePtrT pBundleData = (ClCorDbgGetBundlePtrT)data;
    if(pBundleData != NULL)
    {   
        clCntDelete(pBundleData->cntHandle);
        clBufferDelete(&pBundleData->msgH);
    }
    clHeapFree((ClPtrT)data);
    clHeapFree((ClCorBundleHandlePtrT)key);
    
    return ;
}


ClRcT corDebugRegister(ClEoExecutionObjT* pEoObj)
{
    ClRcT       rc = CL_OK;
    
    /* Container for storing different session information*/
    rc = clCntLlistCreate (_clCorDbgBundleCmpFn,
                           _clCorDbgBundleNodeDelete,
                           _clCorDbgBundleNodeDestroy, CL_CNT_UNIQUE_KEY, 
                           (ClCntHandleT *) &(corDbgBundleCnt));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to create the session container. rc[0x%x] ", rc));
        return rc;
    }

    rc = clDebugPromptSet("COR");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }

    return clDebugRegister(corDebugFuncList, 
           sizeof(corDebugFuncList)/sizeof(corDebugFuncList[0]), 
                           &gCorDebugReg);
}

                                                                                                                             
ClRcT corDebugDeregister(ClEoExecutionObjT* pEoObj)
{
    clCntDelete(corDbgBundleCnt);

    return clDebugDeregister(gCorDebugReg);
}
