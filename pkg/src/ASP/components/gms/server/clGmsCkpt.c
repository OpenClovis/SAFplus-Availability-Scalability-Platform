/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <clGmsCkpt.h>

/* We create 1 checkpoint per group and the name
 * of the checkpoint will be GmsCkpt_<groupId>. So for
 * cluster it will be GmsCkpt_0
 */
#define CKPT_NAME "GmsCkpt"
#define METADATA_DSID 100

extern ClGmsNodeT	gmsGlobalInfo;
ClGmsCkptMetaDataT	gmsCkptMetaData;
ClNameT				metaDataCkptName = {
	.value = "GmsCkptMetadata_1",
	.length = 17
};

static ClRcT   clGmsCkptReadOldCheckpoint();
/* Serialize funtion */
static ClRcT   
clGmsTrackCkptSerialize(ClUint32T       dsId,
                        ClAddrT        *data,
                        ClUint32T      *size,
                        ClPtrT          cookie)
{
    ClRcT           rc        = CL_OK;
    ClBufferHandleT message   = NULL;
    ClUint32T       msgLength = 0;
    ClCharT        *tmpStr    = NULL;

    rc = clBufferCreate(&message);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferCreate failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesWrite(message, (ClUint8T *)cookie,sizeof(ClGmsTrackCkptDataT));
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferNBytesWrite failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferLengthGet failed with rc 0x%x\n",rc);
        return rc;
    }

    tmpStr = (ClCharT *) clHeapAllocate(msgLength);
    if (tmpStr == NULL)
    {
        clLog (ERROR,CKP,NA,
                ("clHeapAllocate failed\n"));
        return CL_ERR_NO_SPACE;
    }

    rc = clBufferNBytesRead(message, (ClUint8T *) tmpStr, &msgLength);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferNBytesRead failed with rc 0x%x\n",rc);
        return rc;
    }

    *data = (ClAddrT) tmpStr;
    *size = msgLength;
    
    /* Delete the buffer */
    clBufferDelete(&message);

    return rc;
}

/* Deserialize funtion */
static ClRcT   
clGmsTrackCkptDeSerialize(ClUint32T     dsId,
                          ClAddrT       data,
                          ClUint32T     size,
                          ClPtrT        cookie)
{
    ClRcT               rc        = CL_OK;
    ClBufferHandleT     message   = NULL;
    ClUint32T           msgLength = 0;
    ClGmsTrackCkptDataT trackData = {0};

    rc = clBufferCreate(&message);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferCreate failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesWrite(message, (ClUint8T *)data,size);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferNBytesWrite failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferLengthGet failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesRead(message, (ClUint8T *)&trackData, &msgLength);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferNBytesRead failed with rc 0x%x\n",rc);
        return rc;
    }

    *((ClGmsTrackCkptDataT *)cookie) = trackData;

    clBufferDelete(&message);
    return rc;
}

static ClRcT   
clGmsCkptMetaDataSerialize(ClUint32T    dsId,
                           ClAddrT     *data,
                           ClUint32T   *size,
                           ClPtrT       cookie)
{
    ClRcT           rc        = CL_OK;
    ClBufferHandleT message   = NULL;
    ClUint32T       msgLength = 0;
    ClCharT        *tmpStr    = NULL;

    rc = clBufferCreate(&message);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferCreate failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesWrite(message, 
                             (ClUint8T *)gmsCkptMetaData.perGroupInfo,
                             (sizeof(ClUint32T) * gmsGlobalInfo.config.noOfGroups));
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferNBytesWrite failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesWrite(message,
                             (ClUint8T *)&gmsCkptMetaData.currentNoOfGroups,
                             sizeof(ClUint32T));
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferNBytesWrite failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferLengthGet failed with rc 0x%x\n",rc);
        return rc;
    }

    tmpStr = (ClCharT *) clHeapAllocate(msgLength);
    if (tmpStr == NULL)
    {
        clLog (ERROR,CKP,NA,
                ("clHeapAllocate failed\n"));
        return CL_ERR_NO_SPACE;
    }

    rc = clBufferNBytesRead(message, (ClUint8T *) tmpStr, &msgLength);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferNBytesRead failed with rc 0x%x\n",rc);
        return rc;
    }

    *data = (ClAddrT) tmpStr;
    *size = msgLength;
    
    /* Delete the buffer */
    clBufferDelete(&message);

    return rc;
}


static ClRcT   
clGmsCkptMetaDataDeSerialize(ClUint32T      dsId,
                             ClAddrT        data,
                             ClUint32T      size,
                             ClPtrT         cookie)
{
    ClRcT           rc        = CL_OK;
    ClBufferHandleT message   = NULL;
    ClUint32T       msgLength = 0;

    rc = clBufferCreate(&message);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferCreate failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferNBytesWrite(message, (ClUint8T *)data,size);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferNBytesWrite failed with rc 0x%x\n",rc);
        return rc;
    }

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clBufferLengthGet failed with rc 0x%x\n",rc);
        return rc;
    }
    msgLength = sizeof(ClUint32T) * gmsGlobalInfo.config.noOfGroups;
    rc = clBufferNBytesRead(message,
                            (ClUint8T *)gmsCkptMetaData.perGroupInfo,
                            &msgLength);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferNBytesRead failed with rc 0x%x\n",rc);
        return rc;
    }

    msgLength = sizeof(ClUint32T);
    rc = clBufferNBytesRead(message,
                             (ClUint8T *)&gmsCkptMetaData.currentNoOfGroups,
                             &msgLength);
    if (rc != CL_OK)
    {
        clLog (ERROR,CKP,NA,
                "clBufferNBytesRead failed with rc 0x%x\n",rc);
        return rc;
    }

    clBufferDelete(&message);
    return rc;
}


ClRcT clGmsCkptInit()
{
    ClRcT       rc         = CL_OK;
    ClBoolT     ckptExists = CL_FALSE; 
    ClUint32T   i          = 0;

    clLog(INFO,CKP,NA,
            "Initializing the track checkpoint for GMS");

    rc = clCkptLibraryInitialize(&gmsGlobalInfo.ckptSvcHandle);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,CKP,NA,
                "clCkptLibraryInitialize failed with rc 0x%x\n",rc);
		return rc;
    }


	/* Create the data structure to store the meta data information */
	gmsCkptMetaData.perGroupInfo = clHeapAllocate((sizeof(ClUint32T) * gmsGlobalInfo.config.noOfGroups));
	if (gmsCkptMetaData.perGroupInfo == NULL)
	{
		clLog(CRITICAL,CKP,NA,
				"Memory allocation failed while initializing GMS CKPT meta data.");
		return CL_ERR_NO_SPACE;
	}
	/* Initialize all the array elements to 100 as this is the value we would
	 * use for dsId */
    for (i = 0; i < gmsGlobalInfo.config.noOfGroups; i++)
    {
        gmsCkptMetaData.perGroupInfo[i] = 100;
    }

	/* Considering cluster group 0 is already present, marking 
	 * persent groups count to 1 */
	gmsCkptMetaData.currentNoOfGroups = 1;

    /* Verify if the metadata checkpoint already exists. If it exists
     * It means that the GMS server was restarted. So we need to read
     * the checkpoint data and populate the values */
    rc = clCkptLibraryDoesCkptExist(gmsGlobalInfo.ckptSvcHandle, &metaDataCkptName, &ckptExists);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clCkptLibraryDoesCkptExist failed with rc 0x%x\n",rc);
        return rc;
    }
    
    /* Create the meta data checkpoint */
    rc = clCkptLibraryCkptCreate(gmsGlobalInfo.ckptSvcHandle, &metaDataCkptName);
    if (rc != CL_OK)
	{
		clLog(ERROR,CKP,NA,
				"metadata checkpoint create failed with rc 0x%x\n",rc);
		return rc;
	}

	/* Create the dataset */
	rc = clCkptLibraryCkptDataSetCreate(gmsGlobalInfo.ckptSvcHandle,
										&metaDataCkptName,
										METADATA_DSID,
										0,
										0,
										clGmsCkptMetaDataSerialize,
										clGmsCkptMetaDataDeSerialize);
	if (rc != CL_OK)
	{
		clLog(ERROR,CKP,NA,
				"clCkptLibraryCkptDataSetCreate failed with rc 0x%x\n",rc);
		return rc;
	}


    if (ckptExists == CL_TRUE)
    {
        /* This was a GMS server restart. So we need to read the
         * checkpoint and populate the data in the server */
        rc = clGmsCkptReadOldCheckpoint();

    }

	return rc;
}


ClRcT 
clGmsCheckpointTrackData(ClGmsTrackCkptDataT *trackData,
                         ClUint32T			  dsId)
{
    /* We create a checkpoint for each group, and inside that
     * checkpoint we create a section for each track node entry
     * with section ID created uniquly in from the global
	 * counter.
     */
    ClNameT         ckptName      = {0};
    ClBoolT         ckptExists    = CL_FALSE;
    ClBoolT         dataSetExists = CL_FALSE;
	ClUint32T		cookie        = 0;
    ClRcT           rc            = CL_OK;

    if (trackData == NULL)
    {
        clLog(ERROR,CKP,NA,
                "trackData is NULL");
        return CL_ERR_NULL_POINTER;
    }

    /* Create checkpoint for the cluster group */
    snprintf(ckptName.value,CL_MAX_NAME_LENGTH,"%s%d",CKPT_NAME,trackData->groupId);
    /* strlen here would not hurt as the value parameter is initialized
     * with null characters.*/
    ckptName.length=strlen(ckptName.value);

    /* Now check if the checkpoint already exists. If so, then dont
     * create it again. */

    rc = clCkptLibraryDoesCkptExist(gmsGlobalInfo.ckptSvcHandle, &ckptName, &ckptExists);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "clCkptLibraryDoesCkptExist failed with rc 0x%x\n",rc);
        return rc;
    }

    if (ckptExists == CL_FALSE)
    {
        /* Create the checkpoint as it doesnt exist */
        rc = clCkptLibraryCkptCreate(gmsGlobalInfo.ckptSvcHandle, &ckptName);
        if (rc != CL_OK)
        {
            clLog(ERROR,CKP,NA,
                    "clCkptLibraryCkptCreate failed with rc 0x%x\n",rc);
			return rc;
        }
        clLog(NOTICE,CKP,NA,
                "Created checkpoint for group id %d",trackData->groupId);
    }

    if (ckptExists == CL_TRUE)
    {
        /* Before creating the dataset with this ID check if it
         * already exists */
        rc = clCkptLibraryDoesDatasetExist(gmsGlobalInfo.ckptSvcHandle,
                                           &ckptName,
                                           dsId,
                                           &dataSetExists);
        if (rc != CL_OK)
        {
            clLog(ERROR,CKP,NA,
                    "clCkptLibraryDoesDatasetExist failed with rc 0x%x\n",rc);
            return rc;
        }
    }

    if (dataSetExists == CL_FALSE)
    {
        /* Create the dataset as it does not exist */
        rc = clCkptLibraryCkptDataSetCreate(gmsGlobalInfo.ckptSvcHandle,
                                            &ckptName,
                                            dsId,
                                            0,
                                            0,
                                            clGmsTrackCkptSerialize,
                                            clGmsTrackCkptDeSerialize);
        if (rc != CL_OK)
        {
            clLog(ERROR,CKP,NA,
                    "clCkptLibraryCkptDataSetCreate failed with rc 0x%x\n",rc);
			return rc;
        }
        clLog(NOTICE,CKP,NA,
                "Created dataset with ID %d for group id %d",dsId,trackData->groupId);
    }

    /* Now write the data into the dataset. Note that in case
     * if the section already existed, then this call for
     * track would be for the user updating the trackflag
     * So we need to still write it to the checkpoint
     */
    rc = clCkptLibraryCkptDataSetWrite(gmsGlobalInfo.ckptSvcHandle,
                                       &ckptName,
                                       dsId,
                                       (ClPtrT)trackData);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
				"clCkptLibraryDataSetWrite for dsId [%d] failed with rc 0x%x\n",rc,dsId);
		return rc;
    }

	/* Now write the about metadata into metadata checkpoint */
	rc = clCkptLibraryCkptDataSetWrite(gmsGlobalInfo.ckptSvcHandle,
			                           &metaDataCkptName,
                                       METADATA_DSID,
                                       (ClPtrT)&cookie);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
				"clCkptLibraryDataSetWrite for metadata failed with rc 0x%x\n",rc);
		return rc;
    }

	return rc;
}

ClRcT
clGmsCkptReadOldCheckpoint()
{
    ClRcT               rc            = CL_OK;
    ClGmsGroupIdT       groupId       = 0;
    ClUint32T           dsId          = 0;
    ClGmsTrackCkptDataT trackData     = {0};
    ClNameT             ckptName      = {0};
    ClUint32T           cookie        = 0;
    ClBoolT             ckptExists    = CL_FALSE;
    ClBoolT             dataSetExists = CL_FALSE;
    ClGmsTrackNodeKeyT  track_key     = {0};
    ClGmsTrackNodeT    *subscriber    = NULL;

    /* Read the metadata */
    rc = clCkptLibraryCkptDataSetRead(gmsGlobalInfo.ckptSvcHandle,
            &metaDataCkptName,
            METADATA_DSID,
            (ClPtrT)&cookie);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "Reading checkpoint metadata failed with rc [0x%x]",rc);
        return rc;
    }

    /* For all the groups read the track data and add the track nodes */
    for (groupId = 0; groupId < gmsCkptMetaData.currentNoOfGroups; groupId++)
    {
        memset(&ckptName, 0, sizeof(ClNameT));

        /* Formulate the checkpoint name for the group */
        snprintf(ckptName.value,CL_MAX_NAME_LENGTH,"%s%d",CKPT_NAME,trackData.groupId);
        /* strlen here would not hurt as the value parameter is initialized
         * with null characters.*/
        ckptName.length=strlen(ckptName.value);

        /* Verify if the checkpoint exists */
        rc = clCkptLibraryDoesCkptExist(gmsGlobalInfo.ckptSvcHandle, &ckptName, &ckptExists);
        if (rc != CL_OK)
        {
            clLog(ERROR,CKP,NA,
                    "clCkptLibraryDoesCkptExist failed with rc 0x%x\n",rc);
            return rc;
        }

        if (ckptExists != CL_TRUE)
        {
            clLog(WARN,CKP,NA,
                    "Checkpoint corresponding to group %d does not exist",groupId);
            continue;
        }
        /*
         * This would open the checkpoint. as it already exists in dbal.
         */
        rc = clCkptLibraryCkptCreate(gmsGlobalInfo.ckptSvcHandle, &ckptName);
        if (rc != CL_OK)
        {
            clLog(ERROR,CKP,NA,
                    "clCkptLibraryCkptCreate failed with rc 0x%x\n",rc);
            continue;
        }

        for (dsId = 100; dsId < gmsCkptMetaData.perGroupInfo[groupId] ; dsId++)
        {
            rc = clCkptLibraryDoesDatasetExist(gmsGlobalInfo.ckptSvcHandle,
                    &ckptName,
                    dsId,
                    &dataSetExists);

            if (rc != CL_OK)
            {
                clLog(ERROR,CKP,NA,
                        "clCkptLibraryDoesDatasetExist failed with rc 0x%x\n",rc);
                return rc;
            }

            if (dataSetExists != CL_TRUE)
            {
                clLog(WARN,CKP,NA,
                        "Dataset corresponding to dsid %d does not exist",dsId);
                continue;
            }
            /*
             * Initialize the dataset serialize/deserialize table before reading the dataset.
             */
            rc = clCkptLibraryCkptDataSetCreate(gmsGlobalInfo.ckptSvcHandle,
                                                &ckptName,
                                                dsId,
                                                0,
                                                0,
                                                clGmsTrackCkptSerialize,
                                                clGmsTrackCkptDeSerialize);
            if (rc != CL_OK)
            {
                clLog(ERROR,CKP,NA,
                      "clCkptLibraryCkptDataSetCreate failed with rc 0x%x\n",rc);
                continue;
            }

            /* Read the dataset */
            rc = clCkptLibraryCkptDataSetRead(gmsGlobalInfo.ckptSvcHandle,
                    &ckptName,
                    dsId,
                    (ClPtrT)&trackData);
            if (rc != CL_OK)
            {
                clLogMultiline(ERROR,CKP,NA,
                        "Reading checkpoint data for DsId [%d] of groupId "
                        "[%d] failed with rc [0x%x]",dsId, groupId, rc);
                continue;
            }

            clLogMultiline(INFO,CKP,NA,
                    "Checkpoint read for old track records: gid %d, "
                    "handle %lld trackflag %d iocaddr %d iocport %x\n",
                    trackData.groupId, trackData.gmsHandle, trackData.trackFlag,
                    trackData.iocAddress.iocPhyAddress.nodeAddress, 
                    trackData.iocAddress.iocPhyAddress.portId);

            /* Add the track nodes corresponding to the checkpoint */

            track_key.handle = trackData.gmsHandle;
            track_key.address = trackData.iocAddress;

            subscriber = (ClGmsTrackNodeT*)clHeapAllocate(sizeof(ClGmsTrackNodeT));
            if (subscriber == NULL)
            {
                printf("Failed alloc\n");
                return CL_ERR_NO_MEMORY;
            }
            subscriber->handle     = trackData.gmsHandle;
            subscriber->address    = trackData.iocAddress;
            subscriber->trackFlags = trackData.trackFlag;
            subscriber->dsId       = dsId;

            rc = _clGmsTrackAddNode(trackData.groupId, track_key, subscriber,&dsId);
            if (rc != CL_OK)
            {
                clHeapFree((void*)subscriber);
                printf("Failed to add track node with rc 0x%x\n",rc);
            }


        }
    }
    return rc;
}

ClRcT   
clGmsCheckpointTrackDataDelete(ClGmsGroupIdT    groupId,
                               ClUint32T        dsId)
{
    ClNameT         ckptName = {0};
    ClRcT           rc       = CL_OK;

    /* Create checkpoint for the cluster group */
    snprintf(ckptName.value,CL_MAX_NAME_LENGTH,"%s%d",CKPT_NAME,groupId);
    /* strlen here would not hurt as the value parameter is initialized
     * with null characters.*/
    ckptName.length=strlen(ckptName.value);

    rc = clCkptLibraryCkptDataSetDelete (gmsGlobalInfo.ckptSvcHandle,
                                         &ckptName,
                                         dsId);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "Failed to delete the track dataset id [%d]. rc = [0x%x]",dsId,rc);
        return rc;
    }
    
    return rc;
}
