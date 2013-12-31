
#ifndef _CL_GMS_CLIENT_API_H_
#define _CL_GMS_CLIENT_API_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "clHandleApi.h"
#include "clVersionApi.h"
#include "clGmsRmdClient.h"
#include "clGmsApiClient.h"

#define   CB_QUEUE_FIN             'f'


/******************************************************************************
 * EXPORTED LIB INIT/FINALIZE FUNCTIONS
 *****************************************************************************/

/* Library initialize function.  Not reentrant! Creates GMS handle database */

ClRcT clGmsLibInitialize(void);

/* Library cleanup function.  Not reentrant! Remoes GMS handle database*/

ClRcT clGmsLibFinalize(void);

#ifdef __cplusplus
 }
#endif


#endif
