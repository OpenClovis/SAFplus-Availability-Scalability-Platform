#undef  __SERVER__
#define __CLIENT__
#include <clGmsClientRmdFunc.h>

ClRcT clGmsClientClientTableRegistrer(ClEoExecutionObjT* eo)
{
    ClRcT rc = CL_OK;

    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, GMS_Client), eo->eoPort);
    if(CL_OK != rc)
    {
        clLogError("GMS", "INT", "GMS EO client table register failed with [%#x]", rc);
        return rc;
    }

    return rc;
}
