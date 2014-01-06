#define __SERVER__
#include <clCommon.h>
#include <clBufferApi.h>
#include <clHeapApi.h>
#include <clLogApi.h>
#include <clAmsEntityTrigger.h>
#include <clAmsTriggerRmd.h>
#include <xdrClMetricT.h>
#include <xdrClAmsEntityConfigT.h>

static ClRcT VDECL(clAmsEntityTriggerLoadRmd)(ClEoDataT data,
                                              ClBufferHandleT inMsg,
                                              ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {CL_AMS_ENTITY_TYPE_ENTITY};
    ClMetricT metric = {0};

    rc = VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, (ClPtrT)&entity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Entity unmarshall returned [%#x]", rc);
        goto out;
    }
    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, (ClPtrT)&metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerLoad(&entity, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger load returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerLoadAllRmd)(ClEoDataT data,
                                                 ClBufferHandleT inMsg,
                                                 ClBufferHandleT outMsg)
{
    ClMetricT metric = {0};
    ClRcT rc = CL_OK;

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerLoadAll(&metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger load all returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerRmd)(ClEoDataT data,
                                          ClBufferHandleT inMsg,
                                          ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {CL_AMS_ENTITY_TYPE_ENTITY};
    ClMetricT metric = {0};

    rc = VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, &entity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD",
                   "Unmarshall entity returned [%#x]", rc);
        goto out;
    }

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGER", "RMD",
                   "Unmarshall metric returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerLoadTrigger(&entity, metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Load trigger returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerAllRmd)(ClEoDataT data,
                                             ClBufferHandleT inMsg,
                                             ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClMetricT metric = {0};

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }
    
    rc = clAmsEntityTriggerLoadTriggerAll(metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Load trigger all returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerRecoveryResetRmd)(ClEoDataT data,
                                                       ClBufferHandleT inMsg,
                                                       ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {CL_AMS_ENTITY_TYPE_ENTITY};
    ClMetricT metric =  {0};

    rc = VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, &entity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Entity unmarshall returned [%#x]", rc);
        goto out;
    }
    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerRecoveryReset(&entity, metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger recovery returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerRecoveryResetAllRmd)(ClEoDataT data,
                                                          ClBufferHandleT inMsg,
                                                          ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClMetricT metric = {0};

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerRecoveryResetAll(metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger recovery all returned [%#x]", rc);
        goto out;
    }
    
    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerResetRmd)(ClEoDataT data,
                                               ClBufferHandleT inMsg,
                                               ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {CL_AMS_ENTITY_TYPE_ENTITY};
    ClMetricT metric = {0};
    
    rc = VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, &entity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Entity unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }

    rc = clAmsEntityTriggerLoadDefault(&entity, metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger reset returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerResetAllRmd)(ClEoDataT data,
                                                  ClBufferHandleT inMsg,
                                                  ClBufferHandleT outMsg)
{
    ClRcT rc = CL_OK;
    ClMetricT metric = {0};

    rc = VDECL_VER(clXdrUnmarshallClMetricT, 4, 0, 0)(inMsg, &metric);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric unmarshall returned [%#x]", rc);
        goto out;
    }
    
    rc = clAmsEntityTriggerLoadDefaultAll(metric.id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Trigger reset all returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT clAmsEntityTriggerGetMetricHandler(ClEoDataT data,
                                                ClBufferHandleT inMsg,
                                                ClBufferHandleT outMsg,
                                                ClBoolT loadDefault)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {CL_AMS_ENTITY_TYPE_ENTITY};
    ClMetricT *pMetric  = NULL;
    ClMetricIdT id = (ClMetricIdT)0;

    rc = VDECL_VER(clXdrUnmarshallClAmsEntityConfigT, 4, 0, 0)(inMsg, (ClPtrT)&entity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Unmarshall entity returned [%#x]", rc);
        goto out;
    }

    rc = VDECL_VER(clXdrUnmarshallClMetricIdT, 4, 0, 0)(inMsg, (ClPtrT)&id);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Unmarshall metric id returned [%#x]", rc);
        goto out;
    }

    pMetric = (ClMetricT*) clHeapCalloc( id == CL_METRIC_ALL ? CL_METRIC_MAX : 1, sizeof(*pMetric));

    if(pMetric == NULL)
    {
        clLogError("TRIGGER", "RMD", "Metric allocation failure");
        rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
        goto out;
    }

    if(loadDefault == CL_FALSE)
    {
        rc = clAmsEntityTriggerGetMetric(&entity, id, pMetric);
    }
    else
    {
        rc = clAmsEntityTriggerGetMetricDefault(&entity, id, pMetric);
    }

    if(rc != CL_OK)
    {
        goto out_free;
    }
    
    if(id == CL_METRIC_ALL)
    {
        rc = VDECL_VER(clXdrMarshallArrayClMetricT, 4, 0, 0)(pMetric, CL_METRIC_MAX, outMsg, 0);
    }
    else
    {
        rc = VDECL_VER(clXdrMarshallClMetricT, 4, 0, 0)(pMetric, outMsg, 0);
    }

    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "Metric marshall returned [%#x]", rc);
        goto out_free;
    }

    out_free:
    if(pMetric) clHeapFree(pMetric);

    out:
    return rc;
}

static ClRcT VDECL(clAmsEntityTriggerGetMetricRmd)(ClEoDataT data,
                                                   ClBufferHandleT inMsg,
                                                   ClBufferHandleT outMsg)
{
    return clAmsEntityTriggerGetMetricHandler(data, inMsg, outMsg, CL_FALSE);
}

static ClRcT VDECL(clAmsEntityTriggerGetMetricDefaultRmd)(ClEoDataT data,
                                                          ClBufferHandleT inMsg,
                                                          ClBufferHandleT outMsg)
{
    return clAmsEntityTriggerGetMetricHandler(data, inMsg, outMsg, CL_TRUE);
}

#include "clAmsEntityTriggerFuncTable.h"

ClRcT clAmsTriggerRmdInitialize(void)
{
    ClRcT rc = CL_OK;
    rc = clEoClientInstallTables(gAms.eoObject, 
                                 CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMFTrigger));
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "EO client install returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

ClRcT clAmsTriggerRmdFinalize(void)
{
    ClRcT rc = CL_OK;
    rc = clEoClientUninstallTables(gAms.eoObject, 
                                   CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMFTrigger));
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RMD", "EO client uninstall returned [%#x]", rc);
        goto out;
    }
    out:
    return rc;
}
