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
 * File        : clCorAlarm.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Clovis internal functions exposed to alarm to minimize the rmd communication
 * between COR and alarm.
 *****************************************************************************/

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clBitApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorLog.h>
#include <clCorRMDWrap.h>
#include <clCpmApi.h>
#include <xdrClCorAlarmConfiguredDataIDLT.h>
#include <xdrClIocPhysicalAddressT.h>
#include <xdrClCorMsoConfigOpIDLT.h>
#include <xdrClCorMsoConfigIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

ClRcT clCorAlarmInfoConfigure(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmInfo)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT inMsgHandle = 0;
    ClBufferHandleT outMsgHandle = 0;
    VDECL_VER(ClCorMsoConfigIDLT, 4, 1, 0) msoConfig = {{0}};

    CL_COR_VERSION_SET(msoConfig.version);

    rc = clCpmComponentNameGet(0, &msoConfig.compName);
    if (rc != CL_OK)
    {
        clLogError("COR", "ALMCONFIG", "Failed to get the component name. rc [0x%x]", rc);
        return rc;
    }

    msoConfig.compAddr.nodeAddress = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&msoConfig.compAddr.portId);

    msoConfig.msoConfigOp = COR_MSOCONFIG_CLIENT_TO_SERVER;
    msoConfig.alarmInfo = *pAlarmInfo;

    rc = clBufferCreate(&inMsgHandle);
    if (rc != CL_OK)
    {
        clLogError("COR", "ALMCONFIG", "Failed to create the buffer handle. rc [0x%x]", rc);
        return rc;
    }

    rc = clBufferCreate(&outMsgHandle);
    if (rc != CL_OK)
    {
        clLogError("COR", "ALMCONFIG", "Failed to create the buffer handle. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    rc = VDECL_VER(clXdrMarshallClCorMsoConfigIDLT, 4, 1, 0)(&msoConfig, inMsgHandle, 0);
    if (rc != CL_OK)
    {
        clLogError("COR", "ALMCONFIG", "Faield to marshall ClCorMsoConfigIDLT. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        clBufferDelete(&outMsgHandle);
        return rc;
    }

    COR_CALL_RMD_SYNC_WITH_MSG(COR_EO_MSO_CONFIG, inMsgHandle, outMsgHandle, rc);

    if (rc != CL_OK)
    {
        clLogError("COR", "ATTRLIST", "RMD to COR server failed. rc [0x%x]", rc);
        goto exit;
    }

exit:
    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);

    return rc; 
}
