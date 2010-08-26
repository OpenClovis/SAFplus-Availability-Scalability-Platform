#ifndef _MSG_TEST_H_
#define _MSG_TEST_H_

#include <saAmf.h>
#include <saMsg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Call it from main or app initialize. after saAmfComponentNameGet.
 */
int msgTestInitialize(const SaNameT *compName);

/*
 * Start the msg tests for this comp. 
 * Call it from csiset callback on getting active
 */
int msgTestStart(void);

/*
 * Call it from csiset during quiesced/quiescing to stop the queue sendrecv tests.
 */
int msgTestQueueStop(void);

/*
 * Call it during app finalize/terminate to cleanup msg queues
 */
int msgTestStop(void);

#ifdef __cplusplus
}
#endif

#endif
