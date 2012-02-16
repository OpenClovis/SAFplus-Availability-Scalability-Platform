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
 * ModuleName  : amf
 * File        : clAmsTriggerClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the client side implementation of the AMS trigger management 
 * API. 
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clRmdApi.h>
#include <clIocApi.h>
#include <clAmsTriggerApi.h>
#include <clAmsTriggerRmd.h>
#include <clAmsErrors.h>
#include <clCpmApi.h>
#include <xdrClAmsEntityConfigT.h>
#include <xdrClMetricT.h>

static ClRcT clAmsTriggerLoadRmd(ClAmsEntityT *pEntity, ClMetricT *pMetric, 
                                 ClUint32T funcId, ClRmdOptionsT *pRmdOptions)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT inMsg = 0;
    ClIocAddressT master = { {0} };
    ClInt32T tries = 0;

    if(!pMetric)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    master.iocPhyAddress.portId = CL_IOC_CPM_PORT;

    while(tries++ < 2)
    {
        ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0};
        rc = clCpmMasterAddressGet(&master.iocPhyAddress.nodeAddress);
        if(rc == CL_OK)
            goto out_success;
        if(rc != CL_OK)
            clOsalTaskDelay(delay);
    }

    clLogError("TRIGGER", "LOAD", "CPM master address get returned [%#x]", rc);
    return rc;

    out_success:
    rc = clBufferCreate(&inMsg);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "LOAD", "Buffer create returned [%#x]", rc);
        goto out;
    }

    if(pEntity)
    {
        /*
         * Patch the entity name incase user hasn't given the right thing
         */
        if(pEntity->name.length != strlen(pEntity->name.value) + 1)
            pEntity->name.length = strlen(pEntity->name.value) + 1;

        rc = VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(pEntity, inMsg, 0);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "LOAD", "Marshall entity returned [%#x]", rc);
            goto out_delete;
        }
    }

    rc = VDECL_VER(clXdrMarshallClMetricT, 4, 0, 0)(pMetric, inMsg, 0);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "LOAD", "Marshall metric returned [%#x]", rc);
        goto out_delete;
    }

    rc = clRmdWithMsg(master, funcId, inMsg, 0, CL_RMD_CALL_ASYNC | CL_RMD_CALL_DO_NOT_OPTIMIZE, pRmdOptions, NULL);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "LOAD", "RMD returned [%#x]", rc);
        goto out_delete;
    }

    out_delete:
    clBufferDelete(&inMsg);

    out:
    return rc;
}

ClRcT clAmsTriggerLoad(ClAmsEntityT *pEntity, ClMetricT *pMetric)
{
    return clAmsTriggerLoadRmd(pEntity, pMetric, 
                               CL_AMS_TRIGGER_LOAD_RMD, NULL);
}

ClRcT clAmsTriggerLoadAll(ClMetricT *pMetric)
{
    return clAmsTriggerLoadRmd(NULL, pMetric, 
                               CL_AMS_TRIGGER_LOAD_ALL_RMD, NULL);
}

ClRcT clAmsTrigger(ClAmsEntityT *pEntity, ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(pEntity, &metric, 
                               CL_AMS_TRIGGER_RMD, NULL);
}

ClRcT clAmsTriggerAll(ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(NULL, &metric, 
                               CL_AMS_TRIGGER_ALL_RMD, NULL);
}

ClRcT clAmsTriggerRecoveryReset(ClAmsEntityT *pEntity, ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(pEntity, &metric,
                               CL_AMS_TRIGGER_RECOVERY_RESET_RMD, NULL);
}

ClRcT clAmsTriggerRecoveryResetAll(ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(NULL, &metric,
                               CL_AMS_TRIGGER_RECOVERY_RESET_ALL_RMD, NULL);
}

ClRcT clAmsTriggerReset(ClAmsEntityT *pEntity, ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(pEntity, &metric,
                               CL_AMS_TRIGGER_RESET_RMD, NULL);
}

ClRcT clAmsTriggerResetAll(ClMetricIdT id)
{
    ClMetricT metric = {0};
    metric.id = id;
    return clAmsTriggerLoadRmd(NULL, &metric,
                               CL_AMS_TRIGGER_RESET_ALL_RMD, NULL);
}

/*
 * Get the metric details. Incase the user wants all the metrics,
 * _he_ should allocate memory till CL_METRIC_MAX in pMetric.
 * Ask Andy regarding why he should allocate memory :-) 
 * Its a good programming practice to avoid allocating memory in APIs.
 */

static ClRcT clAmsTriggerGetMetricHandler(ClAmsEntityT *pEntity, ClMetricIdT id, 
                                          ClMetricT *pMetric, ClUint32T funcId)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT inMsg = 0;
    ClBufferHandleT outMsg = 0;
    ClIocAddressT master = {{0}};
    ClInt32T tries = 0;

    if(!pEntity || !pMetric)
    {
        clLogError("TRIGGER", "GET", "Invalid parameter");
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto out;
    }

    if(id >= CL_METRIC_MAX)
    {
        clLogError("TRIGGER", "GET", "Invalid metric id [%#x]", id);
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto out;
    }
    
    master.iocPhyAddress.portId = CL_IOC_CPM_PORT;

    while(tries++ < 3)
    {
        ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0};
        rc = clCpmMasterAddressGet(&master.iocPhyAddress.nodeAddress);
        if(rc == CL_OK)
            goto out_success;
        
        clOsalTaskDelay(delay);
        continue;
    }
    
    clLogError("TRIGGER", "GET", "CPM master not found. Error returned [%#x]", rc);
    return rc;

    out_success:
    rc = clBufferCreate(&inMsg);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Buffer create error [%#x]", rc);
        goto out;
    }

    rc = clBufferCreate(&outMsg);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Buffer create error [%#x]", rc);
        goto out_free;
    }

    /*
     * patch entity if required.
     */
    if(pEntity->name.length != strlen(pEntity->name.value) + 1)
        pEntity->name.length = strlen(pEntity->name.value) + 1;

    rc = VDECL_VER(clXdrMarshallClAmsEntityConfigT, 4, 0, 0)(pEntity, inMsg, 0);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Marshall entity returned [%#x]", rc);
        goto out_free;
    }

    rc = VDECL_VER(clXdrMarshallClMetricIdT, 4, 0, 0)(&id, inMsg, 0);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Metric marshall returned [%#x]", rc);
        goto out_free;
    }

    rc = clRmdWithMsg(master, funcId, inMsg, outMsg, 
                      CL_RMD_CALL_NEED_REPLY | CL_RMD_CALL_DO_NOT_OPTIMIZE, NULL, NULL);
    
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "METRIC get rmd returned [%#x]", rc);
        goto out_free;
    }

    if(id == CL_METRIC_ALL)
    {
        rc = VDECL_VER(clXdrUnmarshallArrayClMetricT, 4, 0, 0)(outMsg, pMetric, CL_METRIC_MAX);
    }
    else
    {
        rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(outMsg, pMetric);
    }

    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Metric unmarshall for id [%s] returned [%#x]",
                   CL_METRIC_STR(id), rc);
        goto out_free;
    }

    out_free:
    if(inMsg)
        clBufferDelete(&inMsg);

    if(outMsg)
        clBufferDelete(&outMsg);

    out:
    return rc;
}

ClRcT clAmsTriggerGetMetric(ClAmsEntityT *pEntity, ClMetricIdT id, ClMetricT *pMetric)
{
    return clAmsTriggerGetMetricHandler(pEntity, id, pMetric, CL_AMS_TRIGGER_GET_METRIC_RMD);
}

ClRcT clAmsTriggerGetMetricDefault(ClAmsEntityT *pEntity, ClMetricIdT id, ClMetricT *pMetric)
{
    return clAmsTriggerGetMetricHandler(pEntity, id, pMetric, CL_AMS_TRIGGER_GET_METRIC_DEFAULT_RMD);
}
