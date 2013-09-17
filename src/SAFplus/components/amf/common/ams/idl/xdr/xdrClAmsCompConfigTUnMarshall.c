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


/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description : Unmarshall routine for ClAmsCompConfigT
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/
#include "xdrClAmsCompConfigT.h"

ClRcT clXdrUnmarshallClAmsCompConfigT_4_0_0(ClBufferHandleT msg , void* pGenVar)
{
    ClAmsCompConfigT_4_0_0* pVar = (ClAmsCompConfigT_4_0_0*)pGenVar;
    ClRcT     rc     = CL_OK;
    ClUint32T length = 0;

    if ((void*)0 == pVar)
    {
        return CL_XDR_RC(CL_ERR_NULL_POINTER);
    }

    clXdrUnmarshallClUint32T(msg, &length);
    if( 0 == length)
    {
        pGenVar = NULL;
    }
    else
    {

    rc = clXdrUnmarshallClAmsEntityConfigT_4_0_0(msg,&(pVar->entity));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numSupportedCSITypes));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallSaNameT(msg,&(pVar->proxyCSIType));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClAmsCompCapModelT_4_0_0(msg,&(pVar->capabilityModel));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClAmsCompPropertyT_4_0_0(msg,&(pVar->property));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint16T(msg,&(pVar->isRestartable));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint16T(msg,&(pVar->nodeRebootCleanupFail));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->instantiateLevel));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxInstantiate));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxInstantiateWithDelay));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxTerminate));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxAmStart));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxAmStop));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxActiveCSIs));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClUint32T(msg,&(pVar->numMaxStandbyCSIs));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClAmsCompTimerDurationsT_4_0_0(msg,&(pVar->timeouts));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClAmsRecoveryT_4_0_0(msg,&(pVar->recoveryOnTimeout));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallClAmsEntityConfigT_4_0_0(msg,&(pVar->parentSU));
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallArrayClCharT(msg,pVar->instantiateCommand, 256);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrUnmarshallPtrSaNameT(msg,(void**)&(pVar->pSupportedCSITypes),pVar->numSupportedCSITypes);
    if (CL_OK != rc)
    {
        return rc;
    }

    }

    return rc;
}


