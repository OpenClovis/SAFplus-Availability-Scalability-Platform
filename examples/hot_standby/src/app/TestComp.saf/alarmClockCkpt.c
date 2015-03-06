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
#include "alarmClockCkpt.h"
#include "alarmClockLog.h"
#include "alarmClockDef.h"

#define HOT_STANDBY_IDLE (0x1)
#define HOT_STANDBY_ACTIVE (0x2)

ClInt32T g_hot_standby = 0;


static SaCkptHandleT ckpt_svc_hdl = 0;
SaVersionT           ckpt_version = {'B', 1, 1};

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
    ClInt32T    data_size = sizeof(acClockT);
    ClUint32T section_index = 1; /*our section name apparently*/
    ClCkptSectionIdT target_section_id;

    target_section_id.id = (ClUint8T*)&section_index;
    target_section_id.idLen = sizeof(section_index);

    alarmClockLogWrite(CL_LOG_SEV_INFO, 
            "alarmClockCkptCallback(pid=%d): ckpt[%.*s] update received\n", 
            pid, ckpt_name->length, ckpt_name->value);
    for (count = 0; count < num_sections; count++)
    {
        /*
         * check if the received update is for the section of interest to us,
         * if it is update the section, 
         */
        if ((target_section_id.idLen == io_vector[count].sectionId.idLen) && 
            (memcmp(target_section_id.id, (ClCharT *)io_vector[count].sectionId.id, 
                    target_section_id.idLen) == 0))
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
                        "alarmClockCkptCallback(pid=%d): received %d bytes from section %d\n", 
                        pid, (ClInt32T)io_vector[count].readSize, section_index); 

                alarmClockCopyHotStandby((acClockT*)io_vector[count].dataBuffer);
            }

            break;
        }
    }
    /*
     * ckpt in sync through hot standby update
     */
    if(!(g_hot_standby & HOT_STANDBY_ACTIVE))
        g_hot_standby |= HOT_STANDBY_ACTIVE;
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
    ckpt_cr_attr.creationFlags = SA_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_DISTRIBUTED;

    /* Maximum checkpoint size = size of all checkpoints combined
     */
    ckpt_cr_attr.checkpointSize = num_sections * section_size;

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

    ckpt_cr_attr.maxSectionSize = section_size;
    ckpt_cr_attr.maxSectionIdSize = sizeof(SaUint32T);

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

    if (1) // *ckpt_hot_standby == CL_TRUE)
    {
        /*
         * Register checkpoint for immediate consumption, all components that
         * register using this API for a checkpoint will be notified through
         * the callback function provided when a checkpoint update occurs
         * All components except the one that actually updates the checkpoint.
         */
        
        /* When the standby becomes Active it checks this flag to see whether
         * it needs to perform a checkpoint read or not. If TRUE then the
         * assumption is that the checkpoint updates on standby have happened
         * via the callback
         */
        g_hot_standby = HOT_STANDBY_IDLE;
        ret_code = clCkptImmediateConsumptionRegister(*ckpt_hdl, alarmClockCkptCallback, NULL);
        if (ret_code != CL_OK)
        {
            alarmClockLogWrite( CL_LOG_SEV_ERROR, 
                    "alarmClockCkptCreate(pid=%d): Failed to register ckpt callback rc[0x%x]\n",
                    getpid(), ret_code);

            alarmClockLogWrite(CL_LOG_SEV_WARNING, 
                    "alarmClockCkptCreate(pid=%d): Falling back to warm standby mode\n", getpid());

            //*ckpt_hot_standby = CL_FALSE;
            return ret_code;
        }
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptCreate: Successfully registered for callbacks");    
    }

    alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptCreate: Successful creation ckpt [%llx]\n", 
                       *ckpt_hdl);    
    return SA_AIS_OK;    
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
    SaCkptSectionIdT     section_id;


    section_id.id = (SaUint8T*)&section_num;
    section_id.idLen = sizeof(section_num);
    
    ret_code = saCkptSectionOverwrite(ckpt_hdl, &section_id, data, data_size);
    if (ret_code != SA_AIS_OK)
    {
        /* If the overwrite failed because the section did not exist then try to create the section */
        if ((ret_code == 0x1000a) || (ret_code == SA_AIS_ERR_NOT_EXIST))
        {
            SaCkptSectionCreationAttributesT    section_cr_attr;
            
            /* Expiration time for section can also be varied
             * once parameters are read from a configuration file
             */
            section_cr_attr.expirationTime = SA_TIME_END;
            section_cr_attr.sectionId = &section_id;
            ret_code = saCkptSectionCreate(ckpt_hdl,&section_cr_attr,data, data_size);            
        }
    }
    
    if (ret_code != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptWrite(hdl=%llx): Failed to write: 0x%x\n", 
                           ckpt_hdl, ret_code); 
    }
    else
    {
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, "alarmClockCkptWrite: wrote %d bytes to section: %d\n", data_size, section_num); 
    }

    return ret_code;
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
SaAisErrorT
alarmClockCkptRead (
    SaCkptCheckpointHandleT    ckpt_hdl,
    SaUint32T                  section_num,
    void*                      data,
    SaUint32T                  data_size )
{
    SaAisErrorT             ret_code = SA_AIS_OK;
    SaUint32T               error_index;
    SaCkptIOVectorElementT  io_vector;

   /*
     * If we are in sync via the hot-standby, no need to read
     */
    if((g_hot_standby & HOT_STANDBY_ACTIVE))
        return SA_AIS_ERR_NO_OP;
    
    
    io_vector.sectionId.id = (SaUint8T*)&section_num;
    io_vector.sectionId.idLen = sizeof(section_num);

    /* assign other fields of the IoVector
     */
    io_vector.dataBuffer  = data;
    io_vector.dataSize    = data_size;
    io_vector.dataOffset  = 0;
    io_vector.readSize    = 0;

    ret_code = saCkptCheckpointRead(ckpt_hdl, &io_vector, 1, &error_index );
    if (ret_code != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockCkptRead: (error_index=%d) Failed to read: 0x%x\n", error_index, ret_code); 
        goto finalizeRet;
    }
    else
    {
        if ( io_vector.readSize != data_size )
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                    "alarmClockCkptRead(pid=%d): read %d bytes; expected %d bytes\n", 
                   getpid(), (ClInt32T)io_vector.readSize, data_size);
        }
        else
        {
            alarmClockLogWrite(CL_LOG_SEV_DEBUG, 
                "alarmClockCkptRead(pid=%d): read %d bytes from section:%d\n", 
                getpid(), data_size, section_num); 
        }
    }
    
finalizeRet:
    return ret_code;
}

SaAisErrorT alarmClockCkptActivate(SaCkptCheckpointHandleT ckptHdl, ClUint32T section_num)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaCkptSectionIdT id;

    id.idLen = sizeof(section_num);
    id.id = (ClUint8T*)&section_num;

    rc = saCkptActiveReplicaSet(ckptHdl);
    if(rc != SA_AIS_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockReplicaSet: returned [%#x]", rc);
        return rc;
    }
    
    rc = clCkptSectionCheck(ckptHdl, (ClCkptSectionIdT*)&id);
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            SaCkptSectionCreationAttributesT attr;
            attr.sectionId = &id;
            attr.expirationTime = (ClTimeT)CL_TIME_END;
            rc = saCkptSectionCreate(ckptHdl, &attr, NULL, 0);
            if(rc == SA_AIS_OK)
            {
                alarmClockLogWrite(CL_LOG_SEV_INFO, 
                                   "alarmClockActivate: Section [%d] created successfully", 
                                   section_num);
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
