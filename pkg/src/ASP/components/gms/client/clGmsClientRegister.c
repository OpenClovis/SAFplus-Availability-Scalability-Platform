#undef  __SERVER__
#define __CLIENT__
#include <clGmsServerFuncTable.h>

ClRcT clGmsClientTableRegister(ClEoExecutionObjT* eo)
{
    ClRcT rc = CL_OK;
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, GMS), CL_IOC_GMS_PORT);
    if(CL_OK != rc)
    {
        clLogError("GMS", "INT", "GMS EO client table register failed with [%#x]", rc);
        return rc;
    }
    return rc;
}

