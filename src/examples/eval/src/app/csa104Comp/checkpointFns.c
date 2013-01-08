#include <clCommon.h>
#include <clLogApi.h>
#include <saAmf.h>
#include <saCkpt.h>

ClRcT checkpoint_initialize(void);
ClRcT checkpoint_finalize(void);
ClRcT checkpoint_write_seq(ClUint32T);
ClRcT checkpoint_read_seq(ClUint32T*);
ClRcT checkpoint_replica_activate(void);

extern ClBoolT unblockNow;
extern ClLogStreamHandleT gEvalLogStream;
extern SaAmfHAStateT  ha_state;
extern ClUint32T      seq;

#define CKPT_NAME     "csa103Ckpt"  /* Checkpoint name for this application  */
SaCkptHandleT  ckpt_svc_handle; /* Checkpointing service handle       */
SaCkptCheckpointHandleT ckpt_handle;  /* Checkpoint handle        */
#define CKPT_SID_NAME "1"           /* Checkpoint section id                 */
SaCkptSectionIdT ckpt_sid = { /* Section id for checkpoints           */
        (SaUint16T)sizeof(CKPT_SID_NAME)-1,
        (SaUint8T*)CKPT_SID_NAME
};

#define clprintf(severity, ...)   clAppLog(gEvalLogStream, severity, 10,CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,__VA_ARGS__)

#if 0
void csa103TestDispatcher(ClTaskPoolHandleT *pTestHandle)
{
    ClRcT rc = CL_OK;
    rc = clTaskPoolCreate(pTestHandle, 1, NULL, NULL);
    CL_ASSERT(rc == CL_OK);
    clTaskPoolRun(*pTestHandle, csa103CkptTest, NULL);
}
#endif

ClRcT checkpoint_initialize()
{
    SaAisErrorT      rc = CL_OK;
    SaVersionT ckpt_version = {'B', 1, 1};
    SaNameT    ckpt_name = { strlen(CKPT_NAME), CKPT_NAME };
    ClUint32T  seq_no;
    SaCkptCheckpointCreationAttributesT create_atts = {
        .creationFlags     = SA_CKPT_WR_ACTIVE_REPLICA_WEAK |
                             SA_CKPT_CHECKPOINT_COLLOCATED,
        .checkpointSize    = sizeof(ClUint32T),
        .retentionDuration = (ClTimeT)10, 
        .maxSections       = 2, // two sections 
        .maxSectionSize    = sizeof(ClUint32T),
        .maxSectionIdSize  = (ClSizeT)64
    };

    SaCkptSectionCreationAttributesT section_atts = {
        .sectionId = &ckpt_sid,
        .expirationTime = SA_TIME_END
    };
	  
    clprintf(CL_LOG_SEV_INFO,"checkpoint_initialize");
    /* Initialize checkpointing service instance */
    rc = saCkptInitialize(&ckpt_svc_handle,	/* Checkpoint service handle */
						  NULL,			    /* Optional callbacks table */
						  &ckpt_version);   /* Required verison number */
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed to initialize checkpoint service with rc [%#x]", rc);
        return rc;
    }
    clprintf(CL_LOG_SEV_INFO,"Checkpoint service initialized (handle=0x%llx)", ckpt_svc_handle);
    
    //
    // Create the checkpoint for read and write.
    rc = saCkptCheckpointOpen(ckpt_svc_handle,      // Service handle
                              &ckpt_name,         // Checkpoint name
                              &create_atts,       // Optional creation attr.
                              (SA_CKPT_CHECKPOINT_READ |
                               SA_CKPT_CHECKPOINT_WRITE |
                               SA_CKPT_CHECKPOINT_CREATE),
                              (SaTimeT)-1,        // No timeout
                              &ckpt_handle);      // Checkpoint handle

    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to open checkpoint",rc);
        (void)saCkptFinalize(ckpt_svc_handle);
        return rc;
    }
    clprintf(CL_LOG_SEV_INFO,"Checkpoint opened (handle=0x%llx)", ckpt_handle);
    
    /*
     * Try to create a section so that updates can operate by overwriting
     * the section over and over again.
     * If subsequent processes come through here, they will fail to create
     * the section.  That is OK, even though it will cause an error message
     * If the section create fails because the section is already there, then
     * read the sequence number
     */
    // Put data in network byte order
    seq_no = htonl(seq);

    // Creating the section
    checkpoint_replica_activate();
    rc = saCkptSectionCreate(ckpt_handle,           // Checkpoint handle
                             &section_atts,         // Section attributes
                             (SaUint8T*)&seq_no,    // Initial data
                             (SaSizeT)sizeof(seq_no)); // Size of data
    if (rc != SA_AIS_OK && (CL_GET_ERROR_CODE(rc) != SA_AIS_ERR_EXIST))
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed to create checkpoint section");
        (void)saCkptCheckpointClose(ckpt_handle);
        (void)saCkptFinalize(ckpt_svc_handle);
        return rc;
    }
    else if (rc != SA_AIS_OK && (CL_GET_ERROR_CODE(rc) == SA_AIS_ERR_EXIST))
    {
        rc = checkpoint_read_seq(&seq);
        if (rc != CL_OK)
        {
            clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to read checkpoint section", rc);
            (void)saCkptCheckpointClose(ckpt_handle);
            (void)saCkptFinalize(ckpt_svc_handle);
            return rc;
        }
    }
    else
    {
        clprintf(CL_LOG_SEV_INFO,"Section created");
    }

    return CL_OK;
}

ClRcT checkpoint_finalize(void)
{
    SaAisErrorT rc;

    rc = saCkptCheckpointClose(ckpt_handle);
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to close checkpoint handle 0x%llx", rc, ckpt_handle);
    }
    rc = saCkptFinalize(ckpt_svc_handle);
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to finalize checkpoint", rc);
    }
    return CL_OK;
}

ClRcT checkpoint_write_seq(ClUint32T seq)
{
    SaAisErrorT rc = SA_AIS_OK;
    ClUint32T seq_no;
    
    /* Putting data in network byte order */
    seq_no = htonl(seq);
    
    /* Write checkpoint */
    retry:
    rc = saCkptSectionOverwrite(ckpt_handle,
                                &ckpt_sid,
                                &seq_no,
                                sizeof(ClUint32T));
    if (rc != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to write to section", rc);
        if(rc == SA_AIS_ERR_NOT_EXIST)
            rc = checkpoint_replica_activate();
        if(rc == CL_OK) goto retry;
    }
    else 
    {
        /*
         * Synchronize the checkpoint to all the replicas.
         */
        rc = saCkptCheckpointSynchronize(ckpt_handle, SA_TIME_END );
        if (rc != SA_AIS_OK)
        {
            clprintf(CL_LOG_SEV_ERROR,"Failed [0x%x] to synchronize the checkpoint", rc);
        }
    }

    return CL_OK;
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
