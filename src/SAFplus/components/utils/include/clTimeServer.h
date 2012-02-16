#ifndef _CL_TIMESERVER_H_
#define _CL_TIMESERVER_H_

/*
 * 8 bit entity.
 */
#define CL_TIME_SERVER_VERSION (0x1)

#define CL_TIME_SERVER_PORT (9580)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClTimeVal
{
    ClUint64T tvSec;
    ClUint64T tvUsec;
}ClTimeValT;

extern ClRcT clTimeServerGet(const ClCharT *host, ClTimeValT *t);
extern void clTimeServerInitialize(void);
extern void clTimeServerFinalize(void);

#ifdef __cplusplus
}
#endif

#endif
