#ifndef _SESSION_H_
#define _SESSION_H_

#include <clRbTree.h>
#include <clHash.h>
#include "alarmClockDef.h"
#include <saCkpt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Session
{
#define __SESSION_ADD (0x1)
#define __SESSION_DEL (0x2)
    ClUint32T id;
    ClUint32T flags;
    acClockT  clock;
#if 0
    ClUint32T dataSize;
    ClUint8T data[]; /* alarm or data struct wrapper */
#endif
} SessionT;

typedef struct SessionCtrl
{
    ClRbTreeT tree; /* index into the session tree info */
    struct hashStruct hash; /*index into the session hash table */
    ClUint32T offset;
    SessionT session; /*session data*/
} SessionCtrlT;

extern void sessionInit(void);

extern void sessionFinalize(void);

extern ClUint32T sessionBitmapHeaderSize(ClUint32T *chunkSize);

extern SessionCtrlT *sessionAddCkpt(acClockT *clock, SaCkptIOVectorElementT **ioVecs,
                                    SaCkptSectionIdT *ctrl_section_id);

extern ClRcT sessionDelCkpt(ClUint32T id,
                            SessionCtrlT *ctrl,
                            SaCkptIOVectorElementT **ioVecs,
                            SaCkptSectionIdT *ctrl_section_id);

extern ClRcT sessionUpdate(SessionT *session, ClUint32T offset);

extern ClRcT sessionDelete(ClUint32T id, SessionCtrlT *saveEntry);

extern ClRcT sessionWalk(void *arg, ClRcT (*walkCallback) (SessionCtrlT *ctrl, void *arg));

extern ClRcT sessionWalkSafe(void *arg, ClRcT (*walkCallback) (SessionCtrlT *ctrl, void *arg));

extern void  sessionBufferStart(void);

extern void  sessionBufferStop(void);

extern ClBoolT sessionBufferMode(void);

#ifdef __cplusplus
}
#endif

#endif
