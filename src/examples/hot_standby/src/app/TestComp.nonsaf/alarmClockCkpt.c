/*******************************************************************************
 APIs to create/write/read checkpoints for the training exercise

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#define __HOT_STANDBY_IDLE (0x1)
#define __HOT_STANDBY_ACTIVE (0x2)

static ClInt32T g_hot_standby;
static ClCkptSvcHdlT ckpt_svc_hdl = 0;
ClVersionT           ckpt_version = {'B', 1, 1};

/*******************************************************************************
Feature API: alarmClockCkptInitialize

*******************************************************************************/
ClRcT
alarmClockCkptInitialize (void)
{
    ClRcT ret_code = CL_OK;

    if (ckpt_svc_hdl == 0)
    {
        ret_code = clCkptInitialize(&ckpt_svc_hdl, NULL, &ckpt_version);    
        if (ret_code != CL_OK)
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
ClRcT
alarmClockCkptFinalize (void)
{
    ClRcT ret_code = CL_OK;

    if (ckpt_svc_hdl != 0)
    {
        ret_code = clCkptFinalize(ckpt_svc_hdl);    
        if (ret_code != CL_OK)
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
    ClInt32T    section_id_len;
    ClCharT     section_id_name[ CL_MAX_NAME_LENGTH ];

    sprintf(section_id_name, "s00001");
    section_id_len = strlen(section_id_name);

    alarmClockLogWrite(CL_LOG_SEV_INFO, 
            "alarmClockCkptCallback(pid=%d): ckpt[%.*s] update received\n", 
            pid, ckpt_name->length, ckpt_name->value);
    for (count = 0; count < num_sections; count++)
    {
        /*
         * check if the received update is for the section of interest to us,
         * if it is update the section, 
         */
        if ((section_id_len == io_vector[count].sectionId.idLen) && 
            (strncmp(section_id_name, (ClCharT *)io_vector[count].sectionId.id, section_id_len) == 0))
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
                        "alarmClockCkptCallback(pid=%d): received %d bytes from %s\n", 
                        pid, (ClInt32T)io_vector[count].readSize, section_id_name); 

                alarmClockCopyHotStandby((acClockT*)io_vector[count].dataBuffer);
            }

            break;
        }
    }
    /*
     * ckpt in sync through hot standby update
     */
    if(!(g_hot_standby & __HOT_STANDBY_ACTIVE))
        g_hot_standby |= __HOT_STANDBY_ACTIVE;
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
    4. ckpt_hot_standby: If set to true then hot standby turned on

Arguments Out:
    1. : returns ckpt_handle
    2. ckpt_hot_standby: If set to true then hot standby was successful

Return Value:
    ClInt32Teger 0 if success, non zero if failure
*******************************************************************************/
ClRcT
alarmClockCkptCreate (
                      const ClCharT   *ckpt_name,
                      ClInt32T        num_sections,
                      ClInt32T        section_size,
                      ClCkptHdlT      *ckpt_hdl,
                      ClBoolT         *ckpt_hot_standby )
{

    ClRcT                               ret_code = CL_OK;
    SaNameT                             ckpt_name_t;
    ClCkptCheckpointCreationAttributesT ckpt_cr_attr;
    ClCkptOpenFlagsT                    ckpt_open_flags;
    ClTimeT                             timeout;
    ClCharT                              section_id_name[ CL_MAX_NAME_LENGTH ];

    /* First ensure that the Initialize function is called
     */
    ret_code = alarmClockCkptInitialize();
    if (ret_code != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR,
                           "alarmClockCkptCreate(pid=%d): failed initialze service\n",
                           getpid());
        return ret_code;
    }

    /* Initiailze name struct for ckpt
     */
    ckpt_name_t.length = strlen(ckpt_name);
    strcpy(ckpt_name_t.value, ckpt_name);

    /* Get the max size for a  name of sectionId
     */
    sprintf(section_id_name, "s%05d", num_sections);     

    if (*ckpt_hot_standby == CL_TRUE)
    {
        /* Initialize check point creation flags (includes hot standby flag)
         */
        ckpt_cr_attr.creationFlags = CL_CKPT_CHECKPOINT_COLLOCATED | CL_CKPT_DISTRIBUTED;
    }
    else
    {
        /* Initialize check point creation flags 
         */
        ckpt_cr_attr.creationFlags = CL_CKPT_CHECKPOINT_COLLOCATED; 
    }

    /* Maximum checkpoint size = size of all checkpoints combined
     */
    ckpt_cr_attr.checkpointSize = num_sections * section_size;

    /* Retention time: forever
     */
    ckpt_cr_attr.retentionDuration = (ClTimeT)-1;
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
    ckpt_cr_attr.maxSectionIdSize = (ClSizeT)(strlen(section_id_name)+1);

    /*  forever
     */
    timeout = (ClTimeT)-1;

    /* Initialize the checkpoint open flags
     */
    ckpt_open_flags = (CL_CKPT_CHECKPOINT_READ  |
                       CL_CKPT_CHECKPOINT_WRITE |
                       CL_CKPT_CHECKPOINT_CREATE);

    /*  forever
     */
    timeout = (ClTimeT)-1;

    ret_code = clCkptCheckpointOpen(ckpt_svc_hdl, 
                                    &ckpt_name_t, 
                                    &ckpt_cr_attr,
                                    ckpt_open_flags,
                                    timeout,
                                    (ClCkptHdlT *)ckpt_hdl);

    if (ret_code != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR,
                           "alarmClockCkptCreate(pid=%d): Failed to create ckpt:0x%x\n",
                           getpid(), ret_code);
        return ret_code;
    }

    if (*ckpt_hot_standby == CL_TRUE)
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
        g_hot_standby = __HOT_STANDBY_IDLE;
        ret_code = clCkptImmediateConsumptionRegister(*ckpt_hdl, alarmClockCkptCallback, NULL);
        if (ret_code != CL_OK)
        {
            alarmClockLogWrite( CL_LOG_SEV_ERROR, 
                                "alarmClockCkptCreate(pid=%d): Failed to register ckpt callback rc[0x%x]\n",
                                getpid(), ret_code);

            alarmClockLogWrite(CL_LOG_SEV_WARNING, 
                               "alarmClockCkptCreate(pid=%d): Falling back to warm standby mode\n", getpid());

            *ckpt_hot_standby = CL_FALSE;
        }
    }

    alarmClockLogWrite(CL_LOG_SEV_INFO, "alarmClockCkptCreate: Ckpt [%s] created successfully",
                       ckpt_name);
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

Return Value:
    ClInt32Teger 0 if success, non zero if failure
*******************************************************************************/
ClRcT 
alarmClockCkptWrite (
    ClCkptHdlT     ckpt_hdl,
    ClInt32T       section_num,
    void*          data,
    ClInt32T       data_size )
{
    ClRcT                ret_code = CL_OK;
    ClInt32T             pid = getpid();
    ClCharT              section_id_name[ CL_MAX_NAME_LENGTH ];
    ClCkptSectionIdT     section_id;

    sprintf(section_id_name, "s%05d", section_num);
    section_id.id = (ClUint8T*)section_id_name;
    section_id.idLen = strlen(section_id_name);

    ret_code = clCkptSectionOverwrite(ckpt_hdl,
                                      &section_id,
                                      data, data_size);
    if (ret_code != CL_OK)
    {
        if(CL_GET_ERROR_CODE(ret_code) == 0xa || 
           CL_GET_ERROR_CODE(ret_code) == CL_ERR_NOT_EXIST)
        {
            ClCkptSectionCreationAttributesT     section_cr_attr;
            section_cr_attr.sectionId = &section_id;
            section_cr_attr.expirationTime = (ClTimeT)CL_TIME_END;
            ret_code = clCkptSectionCreate(ckpt_hdl, &section_cr_attr,
                                           data, data_size);
        }
    }

    if(ret_code != CL_OK)
    {
            alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                               "alarmClockCkptWrite(pid=%d): Failed to write: 0x%x\n", 
                               pid, ret_code); 
    }
    else
    {
        alarmClockLogWrite(CL_LOG_SEV_DEBUG, 
                           "alarmClockCkptWrite(pid=%d): wrote %d bytes to %s\n", 
                           pid, data_size, section_id_name); 
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

Return Value:
    ClInt32Teger 0 if success, non zero if failure
*******************************************************************************/
ClRcT
alarmClockCkptRead (
    ClCkptHdlT     ckpt_hdl,
    ClInt32T       section_num,
    void*          data,
    ClInt32T       data_size )
{
    ClRcT       ret_code = CL_OK;
    ClUint32T   error_index;
    ClCharT     section_id_name[ CL_MAX_NAME_LENGTH ];
    ClCkptIOVectorElementT  io_vector;

    /*
     * Check if its in sync through hot standby update before reading
     */
    if((g_hot_standby & __HOT_STANDBY_ACTIVE))
        return CL_ERR_NO_OP;

    sprintf(section_id_name, "s%05d", section_num);
    io_vector.sectionId.id = (ClUint8T*)section_id_name;
    io_vector.sectionId.idLen = strlen(section_id_name);

    /* assign other fields of the IoVector
     */
    io_vector.dataBuffer  = data;
    io_vector.dataSize    = data_size;
    io_vector.dataOffset  = 0;
    io_vector.readSize    = 0;

    ret_code = clCkptCheckpointRead(ckpt_hdl, &io_vector, 1, &error_index );
    if (ret_code != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, 
                           "alarmClockCkptRead(pid=%d): (error_index=%d) "
                           "Failed to read section [%s]: 0x%x\n", 
                           getpid(), error_index, 
                           section_id_name, ret_code); 
        return ret_code;
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
                "alarmClockCkptRead(pid=%d): read %d bytes from %s\n", 
                getpid(), data_size, section_id_name); 
        }
    }

    return ret_code;
}

ClRcT alarmClockCkptActivate(ClCkptHdlT ckptHdl, ClUint32T numSections)
{
    ClRcT rc = CL_OK;
    ClCkptSectionIdT id;
    ClCharT section_name[CL_MAX_NAME_LENGTH];

    snprintf(section_name, sizeof(section_name), "s%05d", numSections);
    id.idLen = strlen(section_name);
    id.id = (ClUint8T*)section_name;

    rc = clCkptActiveReplicaSet(ckptHdl);
    if(rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR, "alarmClockReplicaSet: returned [%#x]", rc);
        return rc;
    }
    
    rc = clCkptSectionCheck(ckptHdl, &id);
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        ClCkptSectionCreationAttributesT attr;
        attr.sectionId = &id;
        attr.expirationTime = (ClTimeT)CL_TIME_END;
        rc = clCkptSectionCreate(ckptHdl, &attr, NULL, 0);
        if(rc == CL_OK)
        {
            alarmClockLogWrite(CL_LOG_SEV_INFO, "alarmClockActivate: Section [%s] created successfully",
                               section_name);
        }
    }
    
    if(rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_INFO, "alarmClockActivate: Section operation on [%s] "
                           "failed with [%#x]", section_name, rc);
    }
    
    return rc;
}
