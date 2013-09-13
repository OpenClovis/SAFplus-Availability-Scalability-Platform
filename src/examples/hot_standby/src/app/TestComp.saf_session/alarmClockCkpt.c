/*******************************************************************************
 APIs to create/write/read checkpoints for the training exercise

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
/* The API defined in this file use gettimeofday() instead of
 * OSAL API because of the need to measure with micro-second
 * granularity for APIs
 */
#include <sys/time.h>

#include <clHeapApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include "alarmClockCkpt.h"
#include "alarmClockLog.h"
#include "alarmClockDef.h"
#include "bitmap.h"
#include "session.h"

#define HOT_STANDBY_IDLE (0x1)
#define HOT_STANDBY_ACTIVE (0x2)

ClInt32T g_hot_standby = 0;
static SaCkptHandleT ckpt_svc_hdl = 0;
SaVersionT           ckpt_version = {'B', 1, 1};

struct
{
    SaCkptSectionIdT sectionId;
    SaCkptSectionIdT ctrlSectionId;
} sectionIdMap[1] = { { .sectionId.id = (SaUint8T*)"s00001", 
                        .sectionId.idLen = sizeof("s00001")-1,
                        .ctrlSectionId.id = (SaUint8T*)"ctrl_s00001",
                        .ctrlSectionId.idLen = sizeof("ctrl_s00001")-1,
    },
};


#define NUM_SECTION_ID_MAPS (sizeof(sectionIdMap)/sizeof(sectionIdMap[0]))
#define MAX_NUM_IOVECS (3)

static SaCkptIOVectorElementT *ioVecs = NULL;
static ClUint32T sectionCreated;
static ClOsalMutexT alarmClockCkptMutex;

/*******************************************************************************
Feature API: alarmClockCkptInitialize

*******************************************************************************/
SaAisErrorT
alarmClockCkptInitialize (void)
{
    SaAisErrorT  ret_code = SA_AIS_OK;

    if (ckpt_svc_hdl == 0)
    {
        ret_code = saCkptInitialize(&ckpt_svc_hdl, NULL, &ckpt_version);    
        if (ret_code != SA_AIS_OK)
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR,
                    "alarmClockCkptInitialize(pid=%d): Failed %x\n", 
                    getpid(), ret_code);
        }
    }    
    sessionInit();
    ClRcT rc = clOsalMutexInit(&alarmClockCkptMutex);
    CL_ASSERT(rc == CL_OK);
    ioVecs = clHeapCalloc(MAX_NUM_IOVECS, sizeof(*ioVecs));
    CL_ASSERT(ioVecs != NULL);
    return ret_code;
}

/*******************************************************************************
Feature API: alarmClockCkptFinalize

*******************************************************************************/
SaAisErrorT
alarmClockCkptFinalize (void)
{
    SaAisErrorT  ret_code = SA_AIS_OK;

    if (ckpt_svc_hdl != 0)
    {
        ret_code = saCkptFinalize(ckpt_svc_hdl);    
        if (ret_code != SA_AIS_OK)
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR,
                    "alarmClockCkptFinalize(pid=%d): Failed %x\n", 
                    getpid(), ret_code);
        }
        else
        {
            ckpt_svc_hdl = 0;
        }
    }    
    return ret_code;
}

/*******************************************************************************
Feature  API: alarmClockCkptCallback

Description : 

Other API in this file are either completely generic or mostly so, the hot standby
feature cannot be generic as  the state needs to be conveyed to the application
and made "hot" so to speak. Here we are going to have to know internals of the 
alarm clock structure

Arguments In: 
    1. ClCkptHdlT : ckpt handle
    2. SaNameT *    ckpt_name being updated
    3. ClCkptIOVectorElementT   *io_vector containing updated checkpoint data (all sections)
    4. ClInt32T     number of sections within checkpoint
    5. ClPtrT       cookie; can be used to keep globals within the task

Return Value:
    ClInt32Teger 0 if success, non zero if failure
*******************************************************************************/
static ClRcT 
alarmClockCkptCallback( ClCkptHdlT              ckpt_hdl,
                        SaNameT                 *ckpt_name,
                        ClCkptIOVectorElementT  *io_vector,
                        ClUint32T               num_sections,
                        ClPtrT                  cookie )
{
    ClRcT       ret_code = CL_OK;
    ClInt32T    count;
    ClInt32T    pid = getpid();
    ClInt32T    data_size = sizeof(SessionT);
    ClCkptSectionIdT *target_section_id;

    target_section_id = (ClCkptSectionIdT*)&sectionIdMap[0].sectionId;

    alarmClockLogWrite(CL_LOG_SEV_INFO, 
            "alarmClockCkptCallback(pid=%d): ckpt[%.*s] update received\n", 
            pid, ckpt_name->length, ckpt_name->value);
    
    /*
     * Normal case, would be zero-contention on the lock and futex with no contention 
     * would incur no futex_wait or syscall overhead.
     */
    alarmClockCkptLock();

    for (count = 0; count < num_sections; count++)
    {
        /*
         * check if the received update is for the section of interest to us,
         * if it is update the section, 
         */
        if ((target_section_id->idLen == io_vector[count].sectionId.idLen) && 
            (memcmp(target_section_id->id, (ClCharT *)io_vector[count].sectionId.id, 
                    target_section_id->idLen) == 0))
        {
            if (io_vector[count].dataSize != data_size)
            {
                alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                       "alarmClockCkptCallback(pid=%d): received %d bytes; expected %d bytes\n", 
                        pid, (ClInt32T)io_vector[count].readSize, (ClInt32T)sizeof(acClockT));
            }
            else
            {

                alarmClockLogWrite(CL_LOG_SEV_INFO, 
                                   "alarmClockCkptCallback(pid=%d): received %d bytes from section %.*s "
                                   "for offset [%d]", 
                                   pid, (ClInt32T)io_vector[count].readSize, 
                                   target_section_id->idLen, (const char *)target_section_id->id,
                                   (ClUint32T)io_vector[count].dataOffset);
                
                SessionT *session = (SessionT*)io_vector[count].dataBuffer;
                ClUint32T offset = (ClUint32T)io_vector[count].dataOffset/data_size;
                sessionUpdate(session, offset);
                alarmClockCopyHotStandby(&session->clock);
            }

            break;
        }
    }
    /*
     * ckpt in sync through hot standby update
     */
    if(!(g_hot_standby & HOT_STANDBY_ACTIVE))
        g_hot_standby |= HOT_STANDBY_ACTIVE;

    alarmClockCkptUnlock();

    return ret_code;
}

/*******************************************************************************
Feature  API: alarmClockCkptCreate

Description : Create a checkpoint given a name and the number of sections. In 
addition the nature of the checkpoint needs to be specified, synchronous versus 
asynchronous. If asynchronous collocated versus non collocated.

Arguments In: 
    1. Checkpoint Name
    2. Number of Sections
    3. Size of section

*******************************************************************************/
SaAisErrorT
alarmClockCkptCreate (
                      const SaStringT           ckpt_name,
                      SaUint32T                 num_sections,
                      SaSizeT                   section_size,
                      SaCkptCheckpointHandleT   *ckpt_hdl )
{

    SaAisErrorT                         ret_code = SA_AIS_OK;
    SaNameT                             ckpt_name_t;
    SaCkptCheckpointCreationAttributesT ckpt_cr_attr;
    SaCkptCheckpointOpenFlagsT          ckpt_open_flags;
    SaTimeT                             timeout;

    /* Initiailze name struct for ckpt
     */
    ckpt_name_t.length = strlen(ckpt_name);
    strcpy( ( SaStringT )ckpt_name_t.value,ckpt_name);
    
    /* Initialize check point creation flags
     */
    ckpt_cr_attr.creationFlags = CL_CKPT_DISTRIBUTED | CL_CKPT_PEER_TO_PEER_REPLICA;

    /* Maximum checkpoint size = size of all checkpoints combined
     */
    ckpt_cr_attr.checkpointSize = 0;

    /* Retention time: forever
     */
    ckpt_cr_attr.retentionDuration = (SaTimeT)-1;
    if ( num_sections == 1 ) 
    {   
        /* use a named section instead of the default section */
        ckpt_cr_attr.maxSections = num_sections + 1;
    }
    else
    {
        ckpt_cr_attr.maxSections = num_sections;
    }

    ckpt_cr_attr.maxSectionSize = 0;
    ckpt_cr_attr.maxSectionIdSize = 256;

    /*  forever
     */
    timeout = (SaTimeT)-1;

    /* Initialize the checkpoint open flags
     */

    ckpt_open_flags = (SA_CKPT_CHECKPOINT_READ  |
                       SA_CKPT_CHECKPOINT_WRITE |
                       SA_CKPT_CHECKPOINT_CREATE);
    /*  forever
     */
    timeout = (SaTimeT)-1;

    ret_code = saCkptCheckpointOpen(ckpt_svc_hdl, 
                                    &ckpt_name_t, 
                                    &ckpt_cr_attr,
                                    ckpt_open_flags,
                                    timeout,
                                    ckpt_hdl); 

    if ((ret_code != SA_AIS_OK)||(*ckpt_hdl == 0))
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptCreate: Failed to create ckpt:0x%x\n", ret_code);
        return ret_code;        
    }

    alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptCreate: Successful creation ckpt [%llx]\n", 
                       *ckpt_hdl);    
    return SA_AIS_OK;    
}

/*
 * Register checkpoint for immediate consumption, all components that
 * register using this API for a checkpoint will be notified through
 * the callback function provided when a checkpoint update occurs
 * All components except the one that actually updates the checkpoint.
 */
SaAisErrorT alarmClockCkptHotStandbyRegister(SaCkptCheckpointHandleT ckpt_hdl)
{
    ClRcT ret_code = CL_OK;
    /* When the standby becomes Active it checks this flag to see whether
     * it needs to perform a checkpoint read or not. If TRUE then the
     * assumption is that the checkpoint updates on standby have happened
     * via the callback
     */
    g_hot_standby = HOT_STANDBY_IDLE;
    ret_code = clCkptImmediateConsumptionRegister(ckpt_hdl, alarmClockCkptCallback, NULL);
    if (ret_code != CL_OK)
    {
        alarmClockLogWrite( CL_LOG_SEV_ERROR, 
                            "alarmClockCkptCreate(pid=%d): Failed to register ckpt callback rc[0x%x]\n",
                            getpid(), ret_code);

        alarmClockLogWrite(CL_LOG_SEV_WARNING, 
                           "alarmClockCkptCreate(pid=%d): Falling back to warm standby mode\n", getpid());

        return SA_AIS_ERR_BAD_OPERATION;
    }
    alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptCreate: Successfully registered for callbacks");
    return SA_AIS_OK;
}

SaAisErrorT alarmClockCkptHotStandbyDeregister(SaCkptCheckpointHandleT ckpt_hdl)
{
    ClRcT ret_code = CL_OK;
    g_hot_standby = HOT_STANDBY_IDLE;
    sessionBufferStop();
    ret_code = clCkptImmediateConsumptionRegister(ckpt_hdl, NULL, NULL);
    return (ret_code == CL_OK) ? SA_AIS_OK : SA_AIS_ERR_BAD_OPERATION;
}

static SaAisErrorT
alarmClockCkptSectionCreate(SaCkptCheckpointHandleT ckpt_hdl,
                            ClUint32T section_index)
{
    SaCkptSectionCreationAttributesT    section_cr_attr;
    SaAisErrorT ret_code = SA_AIS_OK;

    if(section_index > NUM_SECTION_ID_MAPS)
        return SA_AIS_ERR_INVALID_PARAM;

    memset(&section_cr_attr, 0, sizeof(section_cr_attr));

    /* Expiration time for section can also be varied
     * once parameters are read from a configuration file
     */
    section_cr_attr.expirationTime = SA_TIME_END;
    section_cr_attr.sectionId = &sectionIdMap[section_index-1].ctrlSectionId;
    ret_code = saCkptSectionCreate(ckpt_hdl, &section_cr_attr, NULL, 0);
    section_cr_attr.sectionId = &sectionIdMap[section_index-1].sectionId;
    ret_code |= saCkptSectionCreate(ckpt_hdl, &section_cr_attr, NULL, 0);
    sectionCreated = CL_TRUE;

    return ret_code;
}


/*******************************************************************************
Feature  API: alarmClockCkptWrite

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
    1. ClCkptHdlT : ckpt handle
    2. section number
    3. data to write
    4. size of data to write 

*******************************************************************************/
SaAisErrorT  
alarmClockCkptWrite (
    SaCkptCheckpointHandleT ckpt_hdl,
    SaUint32T               section_num,
    void*                   data,
    SaUint32T               data_size )
{
    SaAisErrorT          ret_code = SA_AIS_OK; 
    SaCkptSectionIdT     *section_id = NULL;
    SaCkptSectionIdT     *ctrl_section_id = NULL;
    SaCkptIOVectorElementT *ioVecIter = ioVecs;
    ClUint32T errVecIndex = 0;

    if(section_num > NUM_SECTION_ID_MAPS || 
       (data_size != sizeof(acClockT)))
        return SA_AIS_ERR_INVALID_PARAM;

    data_size = sizeof(SessionT);
    section_id = &sectionIdMap[section_num-1].sectionId;
    ctrl_section_id = &sectionIdMap[section_num-1].ctrlSectionId;
    memset(ioVecs, 0, sizeof(*ioVecs) * MAX_NUM_IOVECS);
    SessionCtrlT *sessionCtrl = sessionAddCkpt((acClockT*)data, &ioVecIter, ctrl_section_id);
    CL_ASSERT(sessionCtrl != NULL);
    memcpy(&ioVecIter->sectionId, section_id, sizeof(ioVecIter->sectionId));
    ioVecIter->dataOffset = sessionCtrl->offset * data_size;
    ioVecIter->dataBuffer = &sessionCtrl->session;
    ioVecIter->readSize = ioVecIter->dataSize = sizeof(sessionCtrl->session);
    ++ioVecIter;
    ret_code = saCkptCheckpointWrite(ckpt_hdl, ioVecs, (ClUint32T)(ioVecIter - ioVecs),
                                     &errVecIndex);

    if (ret_code != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptWrite(hdl=%llx): Failed to write: 0x%x "
                           "for session [%d] at offset [%d]. Error iovec index [%d]\n", 
                           ckpt_hdl, ret_code, sessionCtrl->session.id,
                           sessionCtrl->offset, errVecIndex); 
    }
    else
    {
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptWrite: wrote %d bytes to section: %.*s, "
                           "iovecs [%d] for session [%d] at offset [%d]", 
                           data_size, section_id->idLen, (char*)section_id->id,
                           (ClUint32T)(ioVecIter - ioVecs), 
                           sessionCtrl->session.id, sessionCtrl->offset); 
    }

    return ret_code;
}

static ClRcT __alarmClockCkptSectionCheck(ClCkptHdlT ckpt_hdl, ClUint32T section_num)
{
    ClCkptSectionIdT *section_id;
    if(section_num > NUM_SECTION_ID_MAPS)
        return CL_ERR_OUT_OF_RANGE;
    section_id = (ClCkptSectionIdT*)&sectionIdMap[section_num-1].sectionId;
    return clCkptSectionCheck(ckpt_hdl, section_id);
}

/*******************************************************************************
Feature  API: alarmClockCkptRead

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
    1. ClCkptHdlT : ckpt handle
    2. section number
    3. data to read
    4. size of data to read 

*******************************************************************************/
static void bitmapWalkCallback(BitmapT *bitmap, ClUint32T bit, void *arg)
{
    SaCkptIOVectorElementT *ioVec = arg;
    alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptRead(%d): Bit set at offset [%u]",
                       getpid(), bit);
    if(!ioVec) return;
    SessionT *session = (SessionT*)ioVec->dataBuffer + bit;
    ClUint32T readSize = (ClUint32T)ioVec->readSize;
    if( (bit * sizeof(SessionT)) >= readSize)
    {
        alarmClockLogWrite(CL_LOG_SEV_WARNING, "alarmClockCkptRead(%d): Bit [%d] "
                           "exceeds session size [%d]", getpid(), bit, readSize);
        return;
    }
    sessionUpdate(session, bit);
}

SaAisErrorT
alarmClockCkptRead (
                    SaCkptCheckpointHandleT    ckpt_hdl,
                    SaUint32T                  section_num,
                    void*                      data,
                    SaUint32T                  data_size )
{
    SaAisErrorT             ret_code = SA_AIS_OK;
    SaUint32T               error_index = 0;
    SaCkptIOVectorElementT *ioVecIter = ioVecs;
    ClUint32T bitmapChunks = 0;
    ClUint32T chunkSize = 0;
    ClUint32T ctrlHeaderSize = sessionBitmapHeaderSize(&chunkSize);

    if(g_hot_standby & HOT_STANDBY_ACTIVE)
    {
        /*
         * Start session buffering if hotstandby mode is active
         */
        sessionBufferStart();
        return SA_AIS_ERR_EXIST;
    }

    if(section_num > NUM_SECTION_ID_MAPS)
        return SA_AIS_ERR_INVALID_PARAM;

    /*
     * If section doesn't exist, then skip the read
     */
    if(!sectionCreated)
    {
        if(__alarmClockCkptSectionCheck(ckpt_hdl, section_num) != CL_OK)
            return SA_AIS_ERR_NOT_EXIST;
        sectionCreated = CL_TRUE;
    }
    
    memset(ioVecs, 0, sizeof(*ioVecs) * MAX_NUM_IOVECS);

    /*
     * We don't need to read in the ctrl section since we would
     * be syncing our session bitmaps while reading the session data
     */
    memcpy(&ioVecIter->sectionId, &sectionIdMap[section_num-1].ctrlSectionId,
           sizeof(ioVecIter->sectionId));
    ++ioVecIter;
    memcpy(&ioVecIter->sectionId, &sectionIdMap[section_num-1].sectionId,
           sizeof(ioVecIter->sectionId));
    ++ioVecIter;
    ret_code = saCkptCheckpointRead(ckpt_hdl, ioVecs, 2, &error_index);
    if(ret_code == SA_AIS_OK)
    {
        if(!ioVecs[0].readSize)
        {
            alarmClockLogWrite(CL_LOG_SEV_NOTICE, "alarmClockCkptRead(pid=%d): "
                               "Session empty", getpid());
            goto out_free;
        }

        if(ioVecs[0].readSize < ctrlHeaderSize)
        {
            alarmClockLogWrite(CL_LOG_SEV_WARNING, "alarmClockCkptRead(pid=%d): "
                               "read [%d] bytes, expected [%d] bytes",
                               getpid(), (ClUint32T)ioVecs[0].readSize, ctrlHeaderSize);
            ret_code = SA_AIS_ERR_BAD_OPERATION;
            goto out_free;
        }

        bitmapChunks = *(ClUint32T*)ioVecs[0].dataBuffer;
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptRead(pid=%d): "
                           "Bitmap chunks [%d], Bitmap size [%d]\n",
                           getpid(), bitmapChunks, 
                           (ClUint32T)(ioVecs[0].readSize - sizeof(bitmapChunks)));
        BitmapT bitmap = {0};
        bitmap.map = (ClUint8T*)ioVecs[0].dataBuffer + sizeof(bitmapChunks);
        bitmap.words = BYTES_TO_WORDS(bitmapChunks * chunkSize);
        /*
         * We can also walk the session itself (#else case) without using the bitmap segment
         * but could be slightly costly in the worst case with session holes in between
         * for deleted or empty sessions.   
         */
#if 1
        __find_next_bit_walk(&bitmap, &ioVecs[1], bitmapWalkCallback);
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptRead(pid=%d): "
                           "Found [%d] active sessions", getpid(), bitmap.bits);
#else
        __find_next_bit_walk(&bitmap, NULL, bitmapWalkCallback);
        /*
         * Read in session data
         */
        ClUint32T numSessions = 0;
        ClUint8T *sessionBuffer = ioVecs[1].dataBuffer;
        SessionT *sessionIter = (SessionT*)sessionBuffer;
        ClUint32T sessionDataSize = ioVecs[1].readSize;
        numSessions = sessionDataSize/sizeof(SessionT);
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptRead(pid=%d): "
                           "Found [%d] sessions with ckpt size of [%d] bytes", 
                           getpid(), numSessions, sessionDataSize);
        for(ClUint32T i = 0; i < numSessions; ++i, ++sessionIter)
        {
            /*
             * Real world, has to unmarshall or copy-in to avoid unaligned access
             */
            sessionUpdate(sessionIter, i);
        }
#endif
    }

    out_free:
    if(ioVecs[0].dataBuffer)
    {
        clHeapFree(ioVecs[0].dataBuffer);
        ioVecs[0].dataBuffer = NULL;
    }
    if(ioVecs[1].dataBuffer)
    {
        clHeapFree(ioVecs[1].dataBuffer);
        ioVecs[1].dataBuffer = NULL;
    }

    return ret_code;
}

/*
 * Create the section if it doesn't exist.
 * Called on becoming active
 */
SaAisErrorT alarmClockCkptActivate(SaCkptCheckpointHandleT ckptHdl, ClUint32T section_num)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaCkptSectionIdT *id;

    if(section_num > NUM_SECTION_ID_MAPS)
        return SA_AIS_ERR_INVALID_PARAM;

    id = &sectionIdMap[section_num-1].sectionId;

    rc = clCkptSectionCheck(ckptHdl, (ClCkptSectionIdT*)id);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = alarmClockCkptSectionCreate(ckptHdl, 1);
            if(rc == SA_AIS_OK)
            {
                alarmClockLogWrite(CL_LOG_SEV_INFO, 
                                   "alarmClockActivate: Section [%.*s] created successfully", 
                                   id->idLen, (const char *)id->id);
            }
        }
        else rc = SA_AIS_ERR_UNAVAILABLE; /* internal error */
    }
    else rc = SA_AIS_OK;

    if(rc != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_INFO, "alarmClockActivate: Section operation on section [%d] "
                           "failed with [%#x]", section_num, rc);
    }
    
    return rc;
}

SaAisErrorT alarmClockCkptSessionDelete(SaCkptCheckpointHandleT ckpt_hdl, 
                                        ClUint32T section_num,
                                        ClUint32T id)
{
    SaCkptIOVectorElementT *ioVecIter = ioVecs;
    SessionCtrlT ctrl;
    SaCkptSectionIdT *ctrl_section_id;
    SaCkptSectionIdT *section_id;
    SaUint32T errVecIndex = 0;
    SaAisErrorT rc = SA_AIS_OK;
    if(section_num > NUM_SECTION_ID_MAPS)
        return SA_AIS_ERR_INVALID_PARAM;
    ctrl_section_id = &sectionIdMap[section_num-1].ctrlSectionId;
    section_id = &sectionIdMap[section_num-1].sectionId;
    memset(ioVecs, 0, sizeof(*ioVecs) * MAX_NUM_IOVECS);
    rc = sessionDelCkpt(id, &ctrl, &ioVecIter, ctrl_section_id);
    if(rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptSessionDelete: "
                           "Session id [%d] delete failed with [%#x]", 
                           id, rc);
        rc = SA_AIS_ERR_NOT_EXIST;
        goto out;
    }
    /*
     * Write a delete entry by marking the session as deleted
     */
    memcpy(&ioVecIter->sectionId, section_id, sizeof(ioVecIter->sectionId));
    ioVecIter->dataOffset = ctrl.offset * sizeof(SessionT);
    ioVecIter->dataBuffer = &ctrl.session;
    ioVecIter->readSize = ioVecIter->dataSize = sizeof(ctrl.session);
    ++ioVecIter;
    rc = saCkptCheckpointWrite(ckpt_hdl, ioVecs, (ClUint32T)(ioVecIter - ioVecs), 
                               &errVecIndex);
    if(rc != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptSessionDelete: "
                           "Session delete for id [%d] at offset [%d] failed with ckpt error [%#x]",
                           id, ctrl.offset, rc);
        goto out;
    }
    alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptSessionDelete: "
                       "Session delete success for id [%d] at offset [%d]",
                       id, ctrl.offset);

    out:
    return rc;
}

static ClRcT ckptSessionDeleteCallback(SessionCtrlT *ctrl, void *arg)
{
    SaCkptCheckpointHandleT ckpt_hdl = *(SaCkptCheckpointHandleT*) ( (void**)arg ) [0];
    ClUint32T section_num = *(ClUint32T*) ( (void**)arg )[1];
    SaAisErrorT rc = alarmClockCkptSessionDelete(ckpt_hdl, section_num, ctrl->session.id);
    if(rc == SA_AIS_OK) rc = CL_OK;
    return rc;
}

SaAisErrorT alarmClockCkptSessionFinalize(SaCkptCheckpointHandleT ckpt_hdl, 
                                          ClUint32T section_num)
{
    SaAisErrorT rc = SA_AIS_OK;
    void *arr[2];
    arr[0] = (void*)&ckpt_hdl;
    arr[1] = (void*)&section_num;
    rc = sessionWalk((void*)arr, ckptSessionDeleteCallback);
    if(rc == CL_OK) rc = SA_AIS_OK;
    return rc;
}

ClRcT alarmClockCkptLock(void)
{
    return clOsalMutexLock(&alarmClockCkptMutex);
}

ClRcT alarmClockCkptUnlock(void)
{
    return clOsalMutexUnlock(&alarmClockCkptMutex);
}
