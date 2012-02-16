#undef  __CLIENT__
#define __SERVER__
#include <clGmsClientRmdFunc.h>

ClRcT clGmsClientRmdTableInstall(ClEoExecutionObjT* eo)
{
    ClRcT rc = CL_OK;

    rc = clEoClientInstallTables (eo, CL_EO_SERVER_SYM_MOD(gAspFuncTable, GMS_Client));
    if ( rc != CL_OK )
    {
        clLog (EMER,GEN,NA,
                "Eo client install failed with rc [0x%x]. Booting aborted",rc);
        return rc;
    }

    return rc;
}


ClRcT clGmsClientRmdTableUnInstall(ClEoExecutionObjT* eo)
{
    ClRcT rc = CL_OK;

    rc = clEoClientUninstallTables (eo, CL_EO_SERVER_SYM_MOD(gAspFuncTable, GMS_Client));
    if ( rc != CL_OK )
    {
        clLog (EMER,GEN,NA,
                "Eo client install failed with rc [0x%x]. Booting aborted",rc);
        return rc;
    }

    return rc;
}
