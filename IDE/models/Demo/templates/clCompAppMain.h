/*******************************************************************************
 * ModuleName  : Demo
 * $File: //depot/dev/main/Andromeda/IDE/models/Demo/templates/clCompAppMain.h $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef CL_COMP_APP_MAIN
#define CL_COMP_APP_MAIN

#include "./clCompCfg.h"

#ifndef COMP_NAME
#error "COMP_NAME is not defined. Bad or missing ./clCompCfg.h"
#endif

ClRcT clCompAppTerminate(ClInvocationT invocation, const ClNameT  *compName);

ClRcT clCompAppAMFCSISet( ClInvocationT         invocation,
        const ClNameT         *compName,
        ClAmsHAStateT         haState,
        ClAmsCSIDescriptorT   csiDescriptor);

ClRcT clCompAppAMFCSIRemove( ClInvocationT         invocation,
        const ClNameT         *compName,
        const ClNameT         *csiName,
        ClAmsCSIFlagsT        csiFlags);

ClRcT clCompAppInitialize(ClUint32T argc,  ClCharT *argv[]);
ClRcT clCompAppFinalize();

ClRcT 	clCompAppStateChange(ClEoStateT eoState);

ClRcT   clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback);
#endif
