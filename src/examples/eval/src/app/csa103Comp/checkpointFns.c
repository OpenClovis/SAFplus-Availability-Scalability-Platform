#include <clCommon.h>
#include <clLogApi.h>
#include <saAmf.h>
#include <saCkpt.h>
#include "checkpointFns.h"

extern ClBoolT unblockNow;
extern ClLogStreamHandleT gEvalLogStream;
extern SaAmfHAStateT  ha_state;
extern ClUint32T      seq;

static SaInvocationT syncCount=0;

#define clprintf(severity, ...)   clAppLog(gEvalLogStream, severity, 10,CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,__VA_ARGS__)

void checkpointOpenCompleted(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle, SaAisErrorT error)
{
clprintf (CL_LOG_SEV_INFO, "Checkpoint open completed");
}


void checkpointSynchronizeCompleted (SaInvocationT invocation,SaAisErrorT error)
{
    /* Just log that the sync completed but not too often */
    if ((invocation%10)==0) clprintf (CL_LOG_SEV_INFO, "Checkpoint synchronize [%llu] completed ",invocation);
}


SaAisErrorT checkpoint_initialize(void)
{
    SaAisErrorT      rc = CL_OK;
    SaVersionT ckpt_version = {'B', 1, 1};
    SaNameT    ckpt_name = { strlen(CKPT_NAME), CKPT_NAME };
    SaCkptCallbacksT callbacks;
    SaCkptCheckpointCreationAttributesT attrs;

    /* Set up the checkpoint configuration */
    attrs.creationFlags     = SA_CKPT_WR_ACTIVE_REPLICA_WEAK | SA_CKPT_CHECKPOINT_COLLOCATED;        
    attrs.checkpointSize    = sizeof(ClUint32T);
    attrs.retentionDuration = (ClTimeT) 10;        /* How long to keep the checkpoint around even if noone has it open */
    attrs.maxSections       = 2;                   /* Max number of 'rows' in the table */
    attrs.maxSectionSize    = sizeof(ClUint32T);   /* Max 'row' size */
    attrs.maxSectionIdSize  = (ClSizeT)64;         /* Max 'row' identifier size */

    /* These callbacks are only called when an Async style function call completes */
    callbacks.saCkptCheckpointOpenCallback        = checkpointOpenCompleted;
    callbacks.saCkptCheckpointSynchronizeCallback = checkpointSynchronizeCompleted;
        
    clprintf(CL_LOG_SEV_INFO,"Checkpoint Initialize");

        
    /* Initialize checkpointing library */
    rc = saCkptInitialize(&ckptLibraryHandle,	/* Checkpoint service handle */
                          &callbacks,			    /* Optional callbacks table */
                          &ckpt_version);   /* Required verison number */
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed to initialize checkpoint service with rc [%#x]", rc);
        return rc;
    }
    clprintf(CL_LOG_SEV_INFO,"Checkpoint service initialized (handle=0x%llx)", ckptLibraryHandle);
        
    /* Create the checkpoint table for read and write. */
    rc = saCkptCheckpointOpen(ckptLibraryHandle,      // Service handle
                              &ckpt_name,         // Checkpoint name
                              &attrs,       // Optional creation attr.
                              (SA_CKPT_CHECKPOINT_READ |
                               SA_CKPT_CHECKPOINT_WRITE |
                               SA_CKPT_CHECKPOINT_CREATE),
                              (SaTimeT)SA_TIME_MAX,        // No timeout
                              &ckpt_handle);      // Checkpoint handle

    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to open checkpoint",rc);
        return rc;
    }
    clprintf(CL_LOG_SEV_INFO,"Checkpoint opened (handle=0x%llx)", ckpt_handle);
    return rc;
}


void checkpoint_finalize(void)
{
    SaAisErrorT rc;

    rc = saCkptCheckpointClose(ckpt_handle);
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to close checkpoint handle 0x%llx", rc, ckpt_handle);
    }
    rc = saCkptFinalize(ckptLibraryHandle);
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to finalize checkpoint", rc);
    }
}

SaAisErrorT checkpoint_write_seq(ClUint32T seq)
{
    SaAisErrorT rc = SA_AIS_OK;
    ClUint32T seq_no;
    
    /* Putting data in network byte order */
    seq_no = htonl(seq);
    
    /* Write checkpoint */
    rc = saCkptSectionOverwrite(ckpt_handle, &ckpt_sid, &seq_no, sizeof(ClUint32T));
    if (rc != SA_AIS_OK)
    {
        /* The error SA_AIS_ERROR_NOT_EXIST can occur either because we are not the active replica OR
           if the overwrite failed because the section did not exist then try to create the section.

           But in this case we know our application will not attempt to write unless it has set itself as
           the active replica so the problem MUST be a missing section.
        */
        if ((rc == 0x1000a) || (rc == SA_AIS_ERR_NOT_EXIST))
        {
            SaCkptSectionCreationAttributesT    section_cr_attr;
            
            /* Expiration time for section can also be varied
             * once parameters are read from a configuration file
             */
            section_cr_attr.expirationTime = SA_TIME_END;
            section_cr_attr.sectionId = &ckpt_sid;
            rc = saCkptSectionCreate(ckpt_handle,&section_cr_attr,(SaUint8T*) &seq_no, sizeof(ClUint32T));            
        }
        if (rc != SA_AIS_OK)
        {
            clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to write to section", rc);
        }
    }

    if (rc == SA_AIS_OK)
    {
        /*
         * Synchronize the checkpoint to all the replicas.
         */
#if 0          /* Synchronous synchronize example */
        rc = saCkptCheckpointSynchronize(ckpt_handle, SA_TIME_END );
        if (rc != SA_AIS_OK)
        {
            clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to synchronize the checkpoint", rc);
        }
#else
        rc = saCkptCheckpointSynchronizeAsync(ckpt_handle,syncCount);
        syncCount++;
        
#endif        
    }

    return rc;
}

ClRcT checkpoint_read_seq(ClUint32T *seq)
{
    SaAisErrorT rc = CL_OK;
    ClUint32T err_idx; /* Error index in ioVector */
    ClUint32T seq_no = 0xffffffff;
    SaCkptIOVectorElementT iov = {
        .sectionId  = ckpt_sid,
        .dataBuffer = (ClPtrT)&seq_no,
        .dataSize   = sizeof(ClUint32T),
        .dataOffset = (ClOffsetT)0,
        .readSize   = sizeof(ClUint32T)
    };
        
    rc = saCkptCheckpointRead(ckpt_handle, &iov, 1, &err_idx);
    if (rc != SA_AIS_OK)
    {
        if (rc != SA_AIS_ERR_NOT_EXIST)  /* NOT_EXIST is ok, just means no active has written */
            return CL_OK;
        
        clprintf(CL_LOG_SEV_ERROR,"Error: [0x%x] from checkpoint read, err_idx = %u", rc, err_idx);
    }

    *seq = ntohl(seq_no);
    
    return CL_OK;
}

ClRcT checkpoint_replica_activate(void)
{
    SaAisErrorT rc = SA_AIS_OK;

    if ((rc = saCkptActiveReplicaSet(ckpt_handle)) != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR, "checkpoint_replica_activate failed [0x%x] in ActiveReplicaSet", rc);
    }
    else rc = CL_OK;

    return rc;
}
