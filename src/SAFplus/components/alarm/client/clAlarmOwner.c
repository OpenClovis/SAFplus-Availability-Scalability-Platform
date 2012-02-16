#undef __CLIENT__
#define __SERVER__

#include <clCommon.h>
#include <clVersion.h>
#include <clEoApi.h>
#include <clAlarmCommons.h>
#include <clDebugApi.h>
#include <clAlarmOMIpi.h>
#include "clAlarmOmClass.h"
#include <clAlarmClient.h>
#include <clAlarmClientFuncTable.h>

ClRcT clAlarmClientNativeClientTableInit(ClEoExecutionObjT * eoObject)
{
    ClRcT    rc = CL_OK;
    
    CL_FUNC_ENTER();
	clLogTrace( "ALM", "INT", " Enterting [%s] ", __FUNCTION__);
	
    rc = clEoClientInstallTables(eoObject, 
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, ALM_CLIENT));
    if (rc != CL_OK)
    {
		clLogError( "ALM", "INT", 
                " Installing alarm native table failed. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
	
	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);
	CL_FUNC_EXIT();
    return rc;
}

ClRcT clAlarmClientNativeClientTableFinalize(ClEoExecutionObjT* eoObj)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

	clLogTrace( "ALM", "INT", " Enterting [%s] ", __FUNCTION__);

    rc = clEoClientUninstallTables(eoObj, 
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, ALM_CLIENT)); 
    if (rc != CL_OK)
    {
        clLogError("ALM", "FIN", "Failed to unstall the client tables. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

	clLogTrace( "ALM", "INT", " Leaving [%s] ", __FUNCTION__);
    CL_FUNC_EXIT();
    return rc;
}
