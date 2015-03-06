#include <clCommon.h>
#include <clDebugApi.h>
#include <clCkptApi.h>
#include <clCkptIpi.h>
#include <clCkptErrors.h>
#include <clTimerApi.h>
#include "alarmClockLog.h"
#include "alarmClockCkpt.h"
#include "bitmap.h"
#include "session.h"

#define SESSION_BITMAP_BITS (1 << 10)
#define SESSION_BITMAP_SIZE BITS_TO_BYTES(SESSION_BITMAP_BITS)
#define SESSION_BITMAP_MASK (SESSION_BITMAP_SIZE - 1)

#define SESSION_HASH_TABLE_BITS (12)
#define SESSION_HASH_TABLE_SIZE ( 1 << SESSION_HASH_TABLE_BITS )
#define SESSION_HASH_TABLE_MASK ( SESSION_HASH_TABLE_SIZE - 1)

#define SESSION_BUFFER_TIMEOUT (3) /* 3 second buffering */

static ClInt32T sessionCmp(ClRbTreeT *node, ClRbTreeT *treeNode);
static CL_RBTREE_DECLARE(sessionTree, sessionCmp);
static struct hashStruct *sessionHashTable[SESSION_HASH_TABLE_SIZE];
static BitmapT sessionBitmap;
static ClUint32T curSessionId;
static ClBoolT sessionBufferingEnabled;

/*
 * Better to use the bitmap offset for the session as its reusable
 * But just falling back to a simple iterator for now as it could be app. specific
 */
static __inline__ ClUint32T __get_session_id()
{
    return ++curSessionId;
}

static __inline__ void __update_session_id(ClUint32T id)
{
    if(id > curSessionId)
        curSessionId = id;
}

static __inline__ ClUint32T __session_hash(ClUint32T id)
{
    return id & SESSION_HASH_TABLE_MASK;
}

static __inline__ void linkSession(SessionCtrlT *ctrl)
{
    ClUint32T hash = __session_hash(ctrl->session.id);
    hashAdd(sessionHashTable, hash, &ctrl->hash);
    clRbTreeInsert(&sessionTree, &ctrl->tree);
}

static __inline__ void unlinkSession(SessionCtrlT *ctrl)
{
    hashDel(&ctrl->hash);
    clRbTreeDelete(&sessionTree, &ctrl->tree);
}

void sessionInit(void)
{
    bitmap_init(&sessionBitmap, SESSION_BITMAP_SIZE);
    curSessionId = 0;
}

void sessionFinalize(void)
{
    ClRbTreeT *iter, *next;
    if(!sessionTree.root || !sessionTree.nodes) return;
    /*
     * Safe walk
     */
    for(iter = clRbTreeMin(&sessionTree); iter; iter = next)
    {
        SessionCtrlT *ctrl = CL_RBTREE_ENTRY(iter, SessionCtrlT, tree);
        next = clRbTreeNext(&sessionTree, iter);
        unlinkSession(ctrl);
        clHeapFree(ctrl);
    }
    sessionInit(); /*reinitialize*/
}

ClUint32T sessionBitmapHeaderSize(ClUint32T *chunkSize)
{
    if(chunkSize) *chunkSize = sessionBitmap.chunk_size;
    return sessionBitmap.chunk_size + sizeof(sessionBitmap.chunks);
}

static ClRcT __sessionWalk(ClBoolT stopOnFailure,
                           void *arg, 
                           ClRcT (*walkCallback) (SessionCtrlT *ctrl, void *arg) )
{
    ClRbTreeT *iter, *next;
    ClRcT rc = CL_OK;
    if(!walkCallback) return CL_ERR_INVALID_PARAMETER;
    for(iter = clRbTreeMin(&sessionTree); iter; iter = next)
    {
        SessionCtrlT *ctrl = CL_RBTREE_ENTRY(iter, SessionCtrlT, tree);
        next = clRbTreeNext(&sessionTree, iter);
        rc = walkCallback(ctrl, arg);
        if(stopOnFailure && (rc != CL_OK))
            return rc;
    }
    return CL_OK;
}

ClRcT sessionWalk(void *arg, ClRcT (*walkCallback) (SessionCtrlT *ctrl, void *arg) )
{
    return __sessionWalk(CL_TRUE, arg, walkCallback);
}

ClRcT sessionWalkSafe(void *arg, ClRcT (*walkCallback) (SessionCtrlT *ctrl, void *arg) )
{
    return __sessionWalk(CL_FALSE, arg, walkCallback);
}

/*
 * Keep it sorted in session ids
 */
static ClInt32T sessionCmp(ClRbTreeT *node, ClRbTreeT *treeNode)
{
    SessionCtrlT *ctrlNode = CL_RBTREE_ENTRY(node, SessionCtrlT, tree);
    SessionCtrlT *ctrlTreeNode = CL_RBTREE_ENTRY(treeNode, SessionCtrlT, tree);
    return ctrlNode->session.id - ctrlTreeNode->session.id;
}

SessionCtrlT *__sessionHashFind(ClUint32T id)
{
    ClUint32T hash = __session_hash(id);
    struct hashStruct *iter;
    for(iter = sessionHashTable[hash]; iter; iter = iter->pNext)
    {
        SessionCtrlT *ctrl = hashEntry(iter, SessionCtrlT, hash);
        if(ctrl->session.id == id)
            return ctrl;
    }
    return NULL;
}

SessionCtrlT *__sessionTreeFind(ClUint32T id)
{
    SessionCtrlT findSession;
    SessionCtrlT *entry = NULL;
    ClRbTreeT *node = NULL;
    memset(&findSession, 0, sizeof(findSession));
    findSession.session.id = id;
    node = clRbTreeFind(&sessionTree, &findSession.tree);
    if(!node) return NULL;
    entry = CL_RBTREE_ENTRY(node, SessionCtrlT, tree);
    return entry;
}

/*
 * Try to take a lock as it returns global buffers or vectors
 */
SessionCtrlT *sessionAddCkpt(acClockT *clock, 
                             SaCkptIOVectorElementT **ioVecs,
                             SaCkptSectionIdT *ctrl_section_id)
{
    ClUint32T dataOffset = 0;
    ClUint32T ctrlBaseOffset = sizeof(sessionBitmap.ckpt_chunks);
    SaCkptIOVectorElementT *ioVecIter = *ioVecs;
    SessionCtrlT *sessionCtrl = NULL;
    dataOffset = bitmap_free_bit(&sessionBitmap);
    if(sessionBitmap.ckpt_chunks != sessionBitmap.chunks)
    {
        memcpy(&ioVecIter->sectionId, ctrl_section_id, sizeof(ioVecIter->sectionId));
        ioVecIter->dataBuffer = &sessionBitmap.chunks;
        ioVecIter->dataOffset = 0;
        ioVecIter->readSize = ioVecIter->dataSize = sizeof(sessionBitmap.chunks);
        ++ioVecIter;
        memcpy(&ioVecIter->sectionId, ctrl_section_id, sizeof(ioVecIter->sectionId));
        ioVecIter->dataBuffer = sessionBitmap.map + sessionBitmap.ckpt_chunks*sessionBitmap.chunk_size;
        ioVecIter->dataOffset = ctrlBaseOffset + sessionBitmap.ckpt_chunks * sessionBitmap.chunk_size;
        ioVecIter->dataSize = (sessionBitmap.chunks - sessionBitmap.ckpt_chunks) * sessionBitmap.chunk_size;
        ioVecIter->readSize = ioVecIter->dataSize;
        ++ioVecIter;
        sessionBitmap.ckpt_chunks = sessionBitmap.chunks;
    }
    else
    {
        memcpy(&ioVecIter->sectionId, ctrl_section_id, sizeof(ioVecIter->sectionId));
        ioVecIter->dataBuffer = sessionBitmap.map + BIT_TO_BYTE(dataOffset);
        ioVecIter->dataOffset = ctrlBaseOffset + BIT_TO_BYTE(dataOffset);
        ioVecIter->readSize = ioVecIter->dataSize = 1;
        ++ioVecIter;
    }
    *ioVecs = ioVecIter;
    sessionCtrl = clHeapCalloc(1, sizeof(*sessionCtrl));
    CL_ASSERT(sessionCtrl != NULL);
    sessionCtrl->session.id = __get_session_id();
    sessionCtrl->offset = dataOffset;
    sessionCtrl->session.flags = __SESSION_ADD;
    memcpy(&sessionCtrl->session.clock, clock, sizeof(sessionCtrl->session.clock));
    linkSession(sessionCtrl);
    return sessionCtrl;
}

ClRcT sessionDelCkpt(ClUint32T id,
                     SessionCtrlT *ctrl,
                     SaCkptIOVectorElementT **ioVecs,
                     SaCkptSectionIdT *ctrl_section_id)
{
    ClUint32T dataOffset = 0;
    ClUint32T ctrlBaseOffset = sizeof(sessionBitmap.ckpt_chunks);
    SaCkptIOVectorElementT *ioVecIter = *ioVecs;
    ClRcT rc = CL_OK;

    rc = sessionDelete(id, ctrl);
    if(rc != CL_OK)
    {
        goto out;
    }
    dataOffset = ctrl->offset;
    memcpy(&ioVecIter->sectionId, ctrl_section_id, sizeof(ioVecIter->sectionId));
    ioVecIter->dataBuffer = sessionBitmap.map + BIT_TO_BYTE(dataOffset);
    ioVecIter->dataOffset = ctrlBaseOffset + BIT_TO_BYTE(dataOffset);
    ioVecIter->readSize = ioVecIter->dataSize = 1;
    ++ioVecIter;
    *ioVecs = ioVecIter;

    out:
    return rc;
}

/*
 * Internal routine that just unlinks the session
 */
static ClRcT __sessionDelete(ClUint32T id)
{
    SessionCtrlT *entry = __sessionTreeFind(id);
    if(!entry) return CL_ERR_NOT_EXIST;
    unlinkSession(entry);
    clHeapFree(entry);
    return CL_OK;
}

/*
 * We can either use hash or tree search. Tree is faster for duplicates
 * Also validates/updates the session bitmap
 */
ClRcT sessionDelete(ClUint32T id, SessionCtrlT *saveEntry)
{
    SessionCtrlT *entry;
    entry = __sessionTreeFind(id);
    if(!entry) return CL_ERR_NOT_EXIST;
    if(BITS_TO_BYTES(entry->offset) >= sessionBitmap.size)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                           "Delete: Session [%d] offset [%d] invalid", 
                           id, entry->offset);
    }
    else
    {
        if(!clear_bit(entry->offset, &sessionBitmap))
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                               "Delete: Session [%d] offset [%d] never set", 
                               id, entry->offset);
        }
    }
    unlinkSession(entry);
    if(saveEntry)
    {
        memcpy(saveEntry, entry, sizeof(*saveEntry));
        saveEntry->session.flags &= ~__SESSION_ADD;
        saveEntry->session.flags |= __SESSION_DEL;
    }
    clHeapFree(entry);
    alarmClockLogWrite(CL_LOG_SEV_DEBUG, "Deleted session with id [%d]", id);
    return CL_OK;
}

/*
 * Add/delete the synced session structure
 * Check for the offset or entry to be present in our bitmap
 */
ClRcT sessionUpdate(SessionT *session, ClUint32T offset)
{
    ClUint32T mapExtent = BITS_TO_BYTES(offset);
    ClRcT rc = CL_OK;
    SessionCtrlT *ctrl = NULL;
    /*
     * Skip zero or empty session ids
     */
    if(!session->id) return rc;
    /*
     * Delete the session
     */
    if(session->flags & __SESSION_DEL)
    {
        rc = CL_ERR_NOT_EXIST;
        if(mapExtent >= sessionBitmap.size)
        {
            alarmClockLogWrite(CL_LOG_SEV_NOTICE, 
                               "Update: Session with id [%d], offset [%d] doesn't exist",
                               session->id, offset);
            goto out;
        }
        if(!clear_bit(offset, &sessionBitmap) )
        {
            alarmClockLogWrite(CL_LOG_SEV_NOTICE, 
                               "Update: Session with id [%d], offset [%d] was not synced",
                               session->id, offset);
            goto out;
        }
        rc = __sessionDelete(session->id);
        if(rc == CL_OK)
            alarmClockLogWrite(CL_LOG_SEV_DEBUG, 
                               "Deleted session with id [%d], offset [%d]", 
                               session->id, offset);
        else
            alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                               "Session delete for id [%d], offset [%d] "
                               "returned with [%#x]", session->id, offset, rc);
        goto out;
    }

    /*
     * Check if the bitmap has to be grown
     */
    if(mapExtent >= sessionBitmap.size)
    {
        if(! (mapExtent & SESSION_BITMAP_MASK) )
        {
            mapExtent += SESSION_BITMAP_SIZE;
        }
        else
        {
            /*
             * Align to the next bitmap size boundary
             */
            mapExtent += SESSION_BITMAP_MASK;
            mapExtent &= ~SESSION_BITMAP_MASK;
        }
        bitmap_grow(&sessionBitmap, sessionBitmap.size, mapExtent);
    }

    if(set_bit(offset, &sessionBitmap))
    {
        rc = CL_ERR_ALREADY_EXIST;
        alarmClockLogWrite(CL_LOG_SEV_NOTICE, 
                           "Update: Session entry for id [%d], offset [%d] already exists",
                           session->id, offset);
        /*
         * If you want to overwrite, just __sessionTreeFind and overwrite the data
         */
        goto out;
    }
    __update_session_id(session->id);
    ctrl = clHeapCalloc(1, sizeof(*ctrl));
    CL_ASSERT(ctrl != NULL);
    memcpy(&ctrl->session, session, sizeof(ctrl->session));
    linkSession(ctrl);
    rc = CL_OK;
    alarmClockLogWrite(CL_LOG_SEV_DEBUG, 
                       "Session added with id [%d], offset [%d]", 
                       session->id, offset);

    out:
    return rc;
}

static ClRcT sessionBufferTimerCallback(void *unused)
{
    alarmClockCkptLock();
    sessionBufferStop();
    alarmClockCkptUnlock();
    return CL_OK;
}

/*
 * Start buffering for the session during the transition from hot-standby
 * to active since there could be a brief phase where we would continue
 * to receive updates from hot standby context. These are deferred updates
 * and any subsequent session updates wouldn't clash with incoming hot-standby
 * updates in allocation of session ids or session offsets
 */

void sessionBufferStart(void)
{
    ClTimerHandleT bufferTimer = 0;
    static ClTimerTimeOutT bufferTimeout = { .tsSec = SESSION_BUFFER_TIMEOUT, .tsMilliSec = 0 };
    if(sessionBufferingEnabled) return;
    /*
     * Temporarily grow the session bitmap and start allocating
     * from the holes to avoid any collision with incoming hot-standby updates
     */
    bitmap_buffer_enable(&sessionBitmap);
    curSessionId += (SESSION_BITMAP_BITS - 1);
    sessionBufferingEnabled = CL_TRUE;
    /*
     * The timer would be auto-deleted after timeout
     */
    clTimerCreateAndStart(bufferTimeout, CL_TIMER_VOLATILE, CL_TIMER_SEPARATE_CONTEXT,
                          sessionBufferTimerCallback, NULL, &bufferTimer);
}

void sessionBufferStop(void)
{
    if(!sessionBufferingEnabled) return;
    sessionBufferingEnabled = CL_FALSE;
    bitmap_buffer_disable(&sessionBitmap);
}

ClBoolT sessionBufferMode(void)
{
    return sessionBufferingEnabled;
}
