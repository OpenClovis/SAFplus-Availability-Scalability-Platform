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

/*******************************************************************************
 Feature APIs for Checkpoint Subsystem


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <clOsalApi.h>
#include <clCkptApi.h>
#include <clHeapApi.h>
#include <clTcCkptApi.h>

/* This parameter pre-allocates buffers
 * for the sections during creates. Rather
 * than make the API unwieldy I have chosen
 * for now to make this a compile time option
 * that can later be made into a configurable
 * parameter that can be read
 */
#define PRE_ALLOCATE_SECTION 1

/* Do no iterate through all sections, assume user
 * knows the section number
 */
#define GO_TO_SECTION_DIRECTLY 1

static ClCkptSvcHdlT ckpt_svc_hdl = 0;
ClVersionT           ckpt_version = {'B', 1, 1};

/*******************************************************************************
Feature API: clTcCkptSvcInit

*******************************************************************************/
int
clTcCkptSvcInit (void)
{
	ClRcT ret_code = CL_OK;

	if (ckpt_svc_hdl == 0)
	{
		ret_code = clCkptInitialize(&ckpt_svc_hdl, NULL, &ckpt_version);	
		if (ret_code != CL_OK)
		{
			printf("clTcCkptSvcInit: Failed %x\n", ret_code);
		}
	}	

	return ret_code;
}

/*******************************************************************************
Utility API: clTcActivateReplica

Descrition :

This API is utility related, used to set the local checkpoint as the active
replica.  The checkpoint data is retrieved and replica activate called only if
the checkpoint creation flag is ASYNC and COLLOCATED

*******************************************************************************/
static int
clTcActivateReplica ( ClTcCkptDataT *ckpt_data )
{
	ClRcT ret_code = CL_OK;
	ClCkptCheckpointDescriptorT ckpt_desc;

	ret_code = clCkptCheckpointStatusGet((ClCkptHdlT)ckpt_data->ckpt_hdl,
										 &ckpt_desc);

	if (ret_code != CL_OK)
	{
		printf("clTcActivateReplica: could not retrieve ckpt info: 0x%x\n",
			   ret_code);
		return ret_code;
	}

	/* If the check point type is async collocated
	 * then we need to set the active replica to this
	 * node
	 */
	if (ckpt_desc.checkpointCreationAttributes.creationFlags &
		CL_CKPT_CHECKPOINT_COLLOCATED)
	{
    	ret_code = clCkptActiveReplicaSet((ClCkptHdlT)ckpt_data->ckpt_hdl);
		if ( ret_code != CL_OK )
		{
			printf("clTcActivateReplica: Failed to activate replica :0x%x\n", 
				   ret_code);
		}
	}
	return ret_code;
}


/*******************************************************************************
Utility API: clTcIterToSectionId

Descrition :

This API is utility related, used by the write and read feature APIs; in case 
no section ids were used during the create. 

*******************************************************************************/
int
clTcIterToSectionId (
	ClTcCkptDataT* 		ckpt_data,
	int			   		section_num,
    ClCkptSectionIdT*	section_id )
{
	ClRcT 						ret_code = CL_OK;
	int							iter_num;
	ClHandleT					section_iter_hdl;
    ClCkptSectionDescriptorT 	section_desc;

	/* assumption: all sections have same expiration time;
	 * for now set to never expire. We will iterate until
	 * we come to the section number specified
	 */
	ret_code = 
	clCkptSectionIterationInitialize((ClCkptHdlT)ckpt_data->ckpt_hdl,
									 CL_CKPT_SECTIONS_FOREVER,
									 (ClTimeT)-1,
									 &section_iter_hdl);
	if ( ret_code != CL_OK )
	{
		printf("clTcIterToSectionId: Failed initialize iteration: 0x%x\n",
			   ret_code); 
		return ret_code;
	}

	/* iterate until we reach the section number of interest
	 */
	for (iter_num = 0; iter_num < section_num; iter_num++)
	{
		ret_code = clCkptSectionIterationNext(section_iter_hdl,
											  &section_desc);	
		if ( ret_code != CL_OK )
		{
			printf("clTcIterToSectionId: Failed section iteration: 0x%x\n",
				   ret_code); 
			return ret_code;
		}
	}

	/* Finalize the iteration to free up internal resources
	 */
	ret_code = clCkptSectionIterationFinalize(section_iter_hdl);
	if ( ret_code != CL_OK )
	{
		printf("clTcIterToSectionId: Failed iteration finalize: 0x%x\n",
			   ret_code); 
		return ret_code;
	}

	/* assign the section id based on information retured from
	 * the iteration
	 */
	section_id->idLen = section_desc.sectionId.idLen;
	section_id->id    = section_desc.sectionId.id;

	return ret_code;
}

/*******************************************************************************
Feature  API: clTcCkptDelete

Description : This API will first call Checkpoint Close and then 
              Checkpoint delete. The latter function requires the
			  name given to the checkpoint, during Checkpoint open.

Arguments In: 
	1. ClTcCkptDataT : contains the ckpt handle
	2. Checkpoint Name

Arguments Out:
	1. ClTcCkptDataT : returns time taken to write the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int clTcCkptDelete ( 
	const char*	   ckpt_name,
	ClTcCkptDataT* ckpt_data )
{
	ClRcT 		ret_code = CL_OK;
	ClNameT 	ckpt_name_t={0};
    ClTimeT startTime = 0;
    ClTimeT endTime = 0;
	ClTimeT time_taken_us =0;

	/* Initialize name struct for ckpt
	 */
	strncpy(ckpt_name_t.value, ckpt_name,CL_MAX_NAME_LENGTH-1);
	ckpt_name_t.length = strlen(ckpt_name_t.value);

	/* time check 1 start 
	 */
    startTime = clOsalStopWatchTimeGet();

	/* Delete checkpoint (test: check memory profile to ensure all resources
	 * are actually released)
	 */
	ret_code = clCkptCheckpointDelete( ckpt_svc_hdl, &ckpt_name_t);
	if ( ret_code != CL_OK )
	{
		printf("clTcCkptDelete: Failed to delete checkpoint %s: 0x%d \n",
			   ckpt_name, ret_code); 
	}
	ret_code = clCkptCheckpointClose((ClCkptHdlT)ckpt_data->ckpt_hdl);
	if ( ret_code != CL_OK )
	{
		printf("clTcCkptDelete: Failed to close checkpoint %s: 0x%d \n",
			   ckpt_name, ret_code); 
	}


	/* time check 1 end 
	 */
    endTime = clOsalStopWatchTimeGet();
	time_taken_us = endTime - startTime;

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	return ret_code;
}

/*******************************************************************************
Feature  API: clTcCkptCreate

Description : Create a checkpoint given a name and the number of sections. In 
addition the nature of the checkpoint needs to be specified, synchronous versus 
asynchronous. If asynchronous collocated versus non collocated.

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
	1. Checkpoint Name
	2. Section Name Prefix; if NULL no section names; if not NULL then the 
	                        section names will range from 
							<section_name_Prefix>1 to <section_name_prefix>9999
	3. Number of Sections
	4. Size of section
	5. Synchronous/Asynchronous Collocated/Asynhrnous Noncollocated
	6. Output data:

Arguments Out:
	1. ClTcCkptDataT : returns time taken to create ckpt, and ckpt_handle

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int
clTcCkptCreate (
	const char 		*ckpt_name,
	const char 		*section_name_prefix,
	int				num_sections,
	int				section_size,
	ClTcCkptTypeE	ckpt_type,
	ClTcCkptDataT	*ckpt_data )
{

	ClRcT 								ret_code = CL_OK;
	ClNameT 							ckpt_name_t = {0};
	ClCkptCheckpointCreationAttributesT	ckpt_cr_attr;
	ClCkptOpenFlagsT					ckpt_open_flags;
	ClTimeT								timeout;

	int									section_num;
	ClCharT								section_id_name[ CL_MAX_NAME_LENGTH ];
	ClUint8T							*section_name_ptr;
    ClCkptSectionIdT          			section_id;
    ClCkptSectionCreationAttributesT    section_cr_attr;
#ifdef PRE_ALLOCATE_SECTION
	ClPtrT							    section_data;
#endif

    ClTimeT startTime = 0;
    ClTimeT endTime = 0;
    ClTimeT time_taken_us = 0;

	/* First ensure that the Initialize function is called
	 */
	ret_code = clTcCkptSvcInit();
	if (ret_code != CL_OK)
	{
		printf("clTcCkptCreate: failed to initialze service \n");
		return ret_code;
	}

	/* Initiailze name struct for ckpt
	 */
	strncpy(ckpt_name_t.value, ckpt_name, CL_MAX_NAME_LENGTH-1);
	ckpt_name_t.length = strlen(ckpt_name_t.value);

	/* Get the max size for a  name of sectionId
	 */
	if (section_name_prefix != NULL)
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "%s%05d", section_name_prefix, num_sections );
	}
	else
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "s%05d", num_sections);	 
	}


	/* Initialize check point creation flags
	 */
	switch (ckpt_type)
	{
		case TC_CKPT_SYNC:
			ckpt_cr_attr.creationFlags = CL_CKPT_WR_ALL_REPLICAS;
			break;

		case TC_CKPT_ASYNC_COLLOC:
			ckpt_cr_attr.creationFlags = CL_CKPT_CHECKPOINT_COLLOCATED;
			break;

		case TC_CKPT_ASYNC_NON_COLLOC:
			ckpt_cr_attr.creationFlags = CL_CKPT_WR_ACTIVE_REPLICA;
			break;

		default:
			printf("clTcCkptCreate: (warning) invalid checkpoint type\n");
			ckpt_cr_attr.creationFlags = ckpt_type;
	}

	/* Maximum checkpoint size = size of all checkpoints combined
	 */
	ckpt_cr_attr.checkpointSize = num_sections * section_size;

	/* Can make this a configurable parameter when reading from 
	 * a file as opposed to API arguments; for now hardcoded to
	 * forever
	 */
	ckpt_cr_attr.retentionDuration = (ClTimeT)-1;

	ckpt_cr_attr.maxSections = num_sections;

	ckpt_cr_attr.maxSectionSize = section_size;

	ckpt_cr_attr.maxSectionIdSize = (ClSizeT)(strlen(section_id_name)+1);

	/* Initialize the checkpoint open flags
	 */
	ckpt_open_flags = (CL_CKPT_CHECKPOINT_READ  |
					   CL_CKPT_CHECKPOINT_WRITE |
					   CL_CKPT_CHECKPOINT_CREATE);

	/* Can make this a configurable parameter when reading from 
	 * a file as opposed to API arguments; for now hardcoded to
	 * forever
	 */
	timeout = (ClTimeT)-1;

	/* time check 1 start 
	 */
    startTime = clOsalStopWatchTimeGet();

	ret_code = clCkptCheckpointOpen(ckpt_svc_hdl, 
									&ckpt_name_t, 
									&ckpt_cr_attr,
									ckpt_open_flags,
									timeout,
									( ClCkptHdlT *)(&ckpt_data->ckpt_hdl));
	/* time check 1 end
	 */
    endTime = clOsalStopWatchTimeGet();

	time_taken_us = endTime - startTime;

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("clTcCkptCreate: Failed to create ckpt:0x%x \n", ret_code);
		return ret_code;
	}

	/* Intialize section create arguments 
	 */
	
	section_id.idLen = (ClUint16T)strlen(section_id_name); 
	section_id.id    = (ClUint8T*)section_id_name;

	/* If there is a section name prefix, then set the
	 * prefix and advance a pointer to affix the unique
	 * id of a section within the section create loop
	 */
	if ( section_name_prefix != NULL )
	{
		snprintf((ClCharT*)section_id.id,CL_MAX_NAME_LENGTH, "%s", section_name_prefix);
		/* pointer to the part of the string that the 
	 	 * unique identifier of a section will be placed
	 	 */
		section_name_ptr = section_id.id + strlen(section_name_prefix);
	}
	else
	{
		snprintf((ClCharT*)section_id.id, CL_MAX_NAME_LENGTH,"s");

		section_name_ptr = section_id.id + 1; 
	}


	/* Expiration time for section can also be varied
	 * once parameters are read from a configuration file
	 */
	section_cr_attr.expirationTime = (ClTimeT)CL_TIME_END;

	/* ensure that the section ids need to be named
	 */
	section_cr_attr.sectionId = &section_id;

#ifdef PRE_ALLOCATE_SECTION
	/* create a data buffer, because the size given to
	 * the section at creation remains with it forever
	 * not sure if this is a bug
	 */
	section_data = clHeapAllocate(section_size);
	if (section_data == NULL)
	{
		printf("clTcCkptCreate: Failed to allocate section data\n");
		goto clean_up_code;
	}
#endif
	
	/* Set the local replica to be active 
	 * You cannot do any activity on this checkpoint
	 * including a status get (see Bug 6118) unless you 
	 * call this API
	 * 
	 */
	if (ckpt_type == TC_CKPT_ASYNC_COLLOC)
	{
   		ret_code = clCkptActiveReplicaSet((ClCkptHdlT)ckpt_data->ckpt_hdl);
		if (ret_code != CL_OK)
		{
			printf("clTcCkptCreate: Failed to activate replica :0x%x\n", 
			   	   ret_code);
			goto clean_up_code;
		}
	}

	/* Create the sections within the checkpoint
	 */
	for ( section_num = 1; section_num <= num_sections; section_num++ )
	{
		sprintf((ClCharT*)section_name_ptr, "%05d", section_num);
		section_id.idLen = strlen((ClCharT*)section_id.id);

		/* time check 2 start (cumulative)
	 	 */
        startTime = clOsalStopWatchTimeGet();
		ret_code = clCkptSectionCreate((ClCkptHdlT)ckpt_data->ckpt_hdl,
									   &section_cr_attr,
									   NULL, 0);
		/* time check 2 end (cumulative)
		 */
        endTime = clOsalStopWatchTimeGet();

		/* does not account for overflow
		 */
		time_taken_us = endTime - startTime;
		ckpt_data->time_taken_us  += time_taken_us;

		if (ret_code != CL_OK)
		{
			printf("clTcCkptCreate: Failed to create section #%d :0x%x \n", 
					section_num, ret_code);
			goto clean_up_code;
		}
	}

	/* total time taken excluding any intialization code
	 * time check 1 + time check 2
	 */

	/* free up memory allocated to initialize section
	 */
#ifdef PRE_ALLOCATE_SECTION
	clHeapFree(section_data);
#endif
	return ret_code;

	clean_up_code:
	/* Delete checkpoint (test: check memory profile to ensure all resources
	 * are actually released)
	 */
	if ( clCkptCheckpointDelete( ckpt_svc_hdl, &ckpt_name_t) != CL_OK )
	{
		printf("clTcCkptCreate: Failed to delete checkpoint %s\n", ckpt_name); 
	}

	return ret_code;	
}


/*******************************************************************************
Feature  API: clTcCkptWrite

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
	1. ClTcCkptDataT : contains the ckpt handle
	2. Section Name Prefix; if NULL no section names; if not NULL then the 
	                        section names will range from 
							<section_name_Prefix>1 to <section_name_prefix>9999
	3. section number
	4. data to write
	5. size of data to write 

Arguments Out:
	1. ClTcCkptDataT : returns time taken to write the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int clTcCkptWrite (
	ClTcCkptDataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   data_size )
{
	ClRcT 						ret_code = CL_OK;
#if GO_TO_SECTION_DIRECTLY
	ClCharT						section_id_name[ CL_MAX_NAME_LENGTH ];
#endif
    ClCkptSectionIdT    		section_id;

    ClTimeT startTime = 0;
    ClTimeT endTime = 0;
	ClTimeT					time_taken_us = 0;


	/* Does the ckpt_data have a valid chek point handle
	 */

	
	/* we either iterate through the sections until
	 * we get to the section of interest; or go directly
	 * to the section, assuming a known section Id; 
	 * the former is more general purpose and will be used here
	 */
#if GO_TO_SECTION_DIRECTLY
	if ( section_name_prefix != NULL )
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "%s%05d", section_name_prefix, section_num);
	}
	else
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "s%05d", section_num);
	}
	section_id.id = (ClUint8T*)section_id_name;
	section_id.idLen = strlen(section_id_name);
#else
	ret_code = clTcIterToSectionId(ckpt_data, section_num, &section_id);

	if (ret_code != CL_OK)
	{
		printf("clTcCkptWrite: Failed to get section id: 0x%x\n", ret_code); 
		return ret_code;
	}
#endif

	/* Set the local replica to be active 
	 * the api sets it only if required (i.e the type is async collocated)
	 */
	ret_code = clTcActivateReplica(ckpt_data);
	if (ret_code != CL_OK)
	{
		printf("clTcCkptWrite: Failed to activate ckpt: 0x%x\n", ret_code); 
		return ret_code;
	}

	/* time check 1 start 
 	 */
    startTime = clOsalStopWatchTimeGet();
	ret_code = clCkptSectionOverwrite((ClCkptHdlT)ckpt_data->ckpt_hdl,
									  &section_id,
									  data, data_size);
	/* time check 1 end 
	 */
    endTime = clOsalStopWatchTimeGet();
	time_taken_us = endTime - startTime;

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("clTcCkptWrite: Failed to write: 0x%x\n",
			   ret_code); 
	}

	return ret_code;
}

/*******************************************************************************
Feature  API: clTcCkptRead

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
	1. ClTcCkptDataT : contains the ckpt handle
	2. Section Name Prefix; if NULL no section names; if not NULL then the 
	                        section names will range from 
							<section_name_Prefix>1 to <section_name_prefix>9999
	3. section number
	4. data to read
	5. size of data to read 

Arguments Out:
	1. ClTcCkptDataT : returns time taken to read the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int clTcCkptRead (
	ClTcCkptDataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   data_size )
{
	ClRcT 					ret_code = CL_OK;
	ClUint32T				error_index;
#if GO_TO_SECTION_DIRECTLY
	ClCharT					section_id_name[ CL_MAX_NAME_LENGTH ];
#endif
    ClCkptIOVectorElementT  io_vector;
    ClTimeT startTime = 0;
    ClTimeT endTime = 0;
    ClTimeT time_taken_us = 0;

	/* we either iterate through the sections until
	 * we get to the section of interest; or go directly
	 * to the section, assuming a known section Id; 
	 * the former is more general purpose and will be used here
	 */
#if GO_TO_SECTION_DIRECTLY
	if ( section_name_prefix != NULL )
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "%s%05d", section_name_prefix, section_num);
	}
	else
	{
		snprintf(section_id_name,CL_MAX_NAME_LENGTH, "s%05d", section_num);
	}

	io_vector.sectionId.id = (ClUint8T*)section_id_name;
	io_vector.sectionId.idLen = strlen(section_id_name);
#else
	ret_code = clTcIterToSectionId(ckpt_data, section_num, &io_vector.sectionId);
	if (ret_code != CL_OK)
	{
		printf("clTcCkptRead: Failed to get section id: 0x%x\n", ret_code); 
		return ret_code;
	}
#endif

	/* assign other fields of the IoVector
	 */
	io_vector.dataBuffer  = NULL;
	io_vector.dataSize	  = data_size;
	io_vector.dataOffset  = 0;
	io_vector.readSize	  = 0;

	/* time check 1 start 
 	 */
	startTime = clOsalStopWatchTimeGet();
	ret_code = clCkptCheckpointRead((ClCkptHdlT)ckpt_data->ckpt_hdl,
									&io_vector, 1, &error_index );
	/* time check 1 end 
	 */
    endTime = clOsalStopWatchTimeGet();
	time_taken_us = endTime - startTime;

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("clTcCkptRead: Failed to read: 0x%x\n", ret_code); 
		return ret_code;
	}
	else
	{
		if ( io_vector.readSize != data_size )
		{
			printf("clTcCkptRead: Read %d bytes; expected %d bytes\n", 
				   (int)io_vector.readSize, data_size);
		}

		if (data_size <= io_vector.readSize)
		{
			memcpy(data, io_vector.dataBuffer, data_size);
		}
		else
		{
			memcpy(data, io_vector.dataBuffer, io_vector.readSize);
		}

		/* Free up buffer allocated by Ckpt Service
		 */
		clHeapFree(io_vector.dataBuffer);
	}

	return ret_code;
}

ClRcT
clTestCkptCreate(ClCkptSvcHdlT svcHandle,
                 ClCkptCreationFlagsT ckptType,
                 ClUint32T     ckptIdx, 
                 ClUint32T     numSections,
                 ClUint32T     size,
                 ClCkptHdlT    *pCkptHdl)
{
    ClRcT                                rc       = CL_OK;
    ClNameT                              ckptName = {0};
    ClCkptCheckpointCreationAttributesT  creationAtt = {0};
    ClCkptOpenFlagsT                     openFlags = {0};

    creationAtt.creationFlags     = ckptType;
    creationAtt.retentionDuration = 3600000000000ULL;
    creationAtt.maxSections       = numSections;
    creationAtt.maxSectionSize    = size; 
    creationAtt.maxSectionIdSize  = 127;
    openFlags = CL_CKPT_CHECKPOINT_CREATE | CL_CKPT_CHECKPOINT_READ | CL_CKPT_CHECKPOINT_WRITE;

    snprintf(ckptName.value, CL_MAX_NAME_LENGTH,"ckpt%d", ckptIdx);
    ckptName.length = strlen(ckptName.value);

    rc = clCkptCheckpointOpen(svcHandle, &ckptName, &creationAtt, openFlags, 
            0, pCkptHdl);
    if( CL_OK != rc )
    {
        return rc;
    }
    return rc;
}


ClRcT
clTestSectionOverwrite(ClCkptHdlT  ckptHdl,
                      ClUint32T   numSections,
                      ClUint32T   sectionSize,
                      ClTimeT     *pTime)
{
    ClRcT             rc = CL_OK;
    ClUint32T         i  = 0;
    ClUint8T          data[sectionSize];
    ClCkptSectionIdT  secId[numSections];
    ClTimeT oldTime = 0;
    ClTimeT newTime = 0;
    ClCkptSectionIdT  defaultSecId = CL_CKPT_DEFAULT_SECTION_ID;
    ClUint32T         numWrites    = 0;

    memset(data, 'a', sectionSize);

    if( numSections != 1 )
    {
        for( i = 0; i < numSections; i++ )
        {
            secId[i].id = clHeapCalloc(1, 15); 
            if( NULL == secId[i].id )
            {
                return CL_OK;
            }
            snprintf((ClCharT *) secId[i].id,15, "section%d", i);
            secId[i].idLen = strlen((ClCharT *) secId[i].id) + 1;
        }
        numWrites = numSections;
    }
    if( numSections == 1 )
    {
        numWrites = 100;
        oldTime = clOsalStopWatchTimeGet();
        for( i = 0; i < numWrites; i++ )
        {
            rc = clCkptSectionOverwrite(ckptHdl, &defaultSecId, data, sectionSize);
        }
        newTime = clOsalStopWatchTimeGet();
    }
    else
    {
        oldTime = clOsalStopWatchTimeGet();
        for( i = 0; i < numWrites; i++ )
        {
            rc = clCkptSectionOverwrite(ckptHdl, &secId[i], data, sectionSize);
        }
        newTime = clOsalStopWatchTimeGet();
    }

    *pTime = newTime - oldTime;

    if( numSections != 1 )
    {
        for( i = 0; i < numSections; i++ )
        {
            clHeapFree(secId[i].id);
        }
    }
    return CL_OK;
}

ClRcT
clTestSectionCreate(ClCkptHdlT  ckptHdl, 
                    ClUint32T   sectionIdx)
{
    ClRcT                             rc            = CL_OK;
    ClCkptSectionCreationAttributesT  secCreateAttr = {0};

    secCreateAttr.sectionId = clHeapCalloc(1, sizeof(ClCkptSectionIdT));
    if( NULL == secCreateAttr.sectionId ) 
    {
        return CL_OK;
    }
    secCreateAttr.expirationTime = CL_TIME_END;
    secCreateAttr.sectionId->id = clHeapCalloc(1, 15);
    if( NULL == secCreateAttr.sectionId->id ) 
    {
        clHeapFree(secCreateAttr.sectionId);
        return CL_OK;
    }
    snprintf((ClCharT *) secCreateAttr.sectionId->id,15, "section%d", sectionIdx);
    secCreateAttr.sectionId->idLen = strlen((ClCharT *) secCreateAttr.sectionId->id) + 1; 
    rc = clCkptSectionCreate(ckptHdl, &secCreateAttr, NULL, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    clHeapFree(secCreateAttr.sectionId->id);
    clHeapFree(secCreateAttr.sectionId);
    return CL_OK;
}
ClRcT
clTestCkptRead(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections,
               ClTimeT *pTime)
      
{
    ClRcT           rc = CL_OK;
    ClUint32T       i = 0;
    ClCkptIOVectorElementT  iov[numSections];
    ClUint32T               readIdx = 0;
    ClTimeT oldTime = 0;
    ClTimeT newTime = 0;
    ClCkptSectionIdT  defaultSecId = CL_CKPT_DEFAULT_SECTION_ID;

    memset(iov, 0, numSections * sizeof(ClCkptIOVectorElementT));
    for( i = 0; i < numSections; i++ )
    {
        if( numSections != 1 )
        {
            iov[i].sectionId.id = clHeapCalloc(1, 15);
            if( NULL == iov[i].sectionId.id )
            {   
                return CL_OK;
            }
            snprintf( (ClCharT *) iov[i].sectionId.id, 15,"section%d", i);
            iov[i].sectionId.idLen = strlen((ClCharT *) iov[i].sectionId.id) + 1;
        }
        else
        {
            iov[i].sectionId = defaultSecId;
        }
        iov[i].dataSize = 0;
        iov[i].dataBuffer = NULL;
        iov[i].readSize = 0;
        iov[i].dataOffset = 0;
    }
    if( numSections == 1 )
    {
        oldTime = clOsalStopWatchTimeGet();
        for( i = 0; i < 100; i++ )
        {
            rc = clCkptCheckpointRead(ckptHdl, &iov[0], numSections, &readIdx); 
        }
        newTime = clOsalStopWatchTimeGet();
    }
    else
    {
        oldTime = clOsalStopWatchTimeGet();
        rc = clCkptCheckpointRead(ckptHdl, iov, numSections, &readIdx); 
        newTime = clOsalStopWatchTimeGet();
    }

    *pTime = newTime - oldTime;

    for( i = 0; i < numSections; i++ )
    {
        clHeapFree(iov[i].dataBuffer);
        if( NULL != iov[i].sectionId.id ) 
        {
            clHeapFree(iov[i].sectionId.id);
        }
    }
    return CL_OK;
}

ClRcT
clTestCkptRead_withTime(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections)
{
    ClRcT           rc = CL_OK;
    ClUint32T       i = 0, j = 0;
    ClCkptIOVectorElementT  iov[numSections];
    ClUint32T               readIdx = 0;
    ClUint32T         timeDelay[] = {1000000, 100000, 10000, 1000, 100, 10 , 0};

    memset(iov, 0, numSections * sizeof(ClCkptIOVectorElementT));
    for( i = 0; i < numSections; i++ )
    {
        if( numSections != 1 )
        {
            iov[i].sectionId.id = clHeapCalloc(1, 15);
            if( NULL == iov[i].sectionId.id )
            {   
                return CL_OK;
            }
            snprintf( (ClCharT *) iov[i].sectionId.id,15, "section%d", i);
            iov[i].sectionId.idLen = strlen((ClCharT *) iov[i].sectionId.id) + 1;
        }
        iov[i].dataSize = 0;
        iov[i].dataBuffer = NULL;
        iov[i].readSize = 0;
        iov[i].dataOffset = 0;
    }
    for( i = 0; i < (sizeof(timeDelay) / sizeof(sizeof(timeDelay[0]))); i++ )
    {
        for( j = 0; j < 1000; j++ )
        {
            rc = clCkptCheckpointRead(ckptHdl, iov, numSections, &readIdx); 
            for( i = 0; i < numSections; i++ )
            {
                clHeapFree(iov[i].dataBuffer);
                iov[i].dataBuffer = NULL; 
            }
            usleep(timeDelay[i]);
        }
    }
    for( i = 0; i < numSections; i++ )
    {
        if( NULL != iov[i].sectionId.id ) 
        {
            clHeapFree(iov[i].sectionId.id);
        }
    }
    return CL_OK;
}

ClRcT
clTestSectionOverwrite_withTime(ClCkptHdlT  ckptHdl,
                      ClUint32T   numSections,
                      ClUint32T   sectionSize)
{
    ClRcT             rc = CL_OK;
    ClUint32T         i  = 0, j = 0, secNum = 0;
    ClUint8T          data[sectionSize];
    ClCkptSectionIdT  secId[numSections];
    ClUint32T         timeDelay[] = {1000000, 100000, 10000, 1000, 100, 10 , 0};

    memset(data, 'a', sectionSize);
    for( i = 0; i < numSections; i++ )
    {
        secId[i].id = clHeapCalloc(1, 15); 
        if( NULL == secId[i].id )
        {
            return CL_OK;
        }
        snprintf((ClCharT *) secId[i].id, 15, "section%d", i);
        secId[i].idLen = strlen((ClCharT *) secId[i].id) + 1;
    }

    for( i = 0; i < sizeof(timeDelay) / sizeof(timeDelay[0]); i++ )
    {
        for( j = 0; j < 1000; j++ )
        {
            for( secNum = 0; secNum < numSections; secNum++ )
            {
                rc = clCkptSectionOverwrite(ckptHdl, &secId[secNum], data, sectionSize);
                if( CL_OK != rc )
                {
                   printf("Section overwrite failed rc[0x %x]", rc);
                }
                usleep(timeDelay[i]);
            }
        }
    }

    if( numSections != 1 )
    {
        for( i = 0; i < numSections; i++ )
        {
            clHeapFree(secId[i].id);
        }
    }
    return CL_OK;
}

ClRcT
clTestCheckpointWrite(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections)
{
    ClRcT           rc = CL_OK;
    ClUint32T       i = 0;
    ClCkptIOVectorElementT  iov[numSections];
    ClUint32T               readIdx = 0;

    memset(iov, 0, numSections * sizeof(ClCkptIOVectorElementT));
    for( i = 0; i < numSections; i++ )
    {
        if( numSections != 1 )
        {
            iov[i].sectionId.id = clHeapCalloc(1, 15);
            if( NULL == iov[i].sectionId.id )
            {   
                return CL_OK;
            }
            snprintf( (ClCharT *) iov[i].sectionId.id, 15, "section%d", i);
            iov[i].sectionId.idLen = strlen((ClCharT *) iov[i].sectionId.id) + 1;
        }
        iov[i].dataSize = 50;
        if( NULL != (iov[i].dataBuffer = clHeapCalloc(1, iov[i].dataSize)) )
        {
            memset(iov[i].dataBuffer, 'a', iov[i].dataSize);
        }
        iov[i].readSize = 0;
        iov[i].dataOffset = 0;
    }
            rc = clCkptCheckpointWrite(ckptHdl, iov, numSections, &readIdx); 
            if( CL_OK != rc )
            {
                return rc;
            }
    for( i = 0; i < numSections; i++ )
    {
        if( NULL != iov[i].sectionId.id ) 
        {
            clHeapFree(iov[i].sectionId.id);
            clHeapFree(iov[i].dataBuffer);
        }
    }
    return CL_OK;
}

ClRcT
clTestCheckpointReadWithOffSet(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections)
{
    ClRcT           rc = CL_OK;
    ClUint32T       i = 0;
    ClCkptIOVectorElementT  iov[numSections];
    ClUint32T               readIdx = 0;

    memset(iov, 0, numSections * sizeof(ClCkptIOVectorElementT));
    for( i = 0; i < numSections; i++ )
    {
        if( numSections != 1 )
        {
            iov[i].sectionId.id = clHeapCalloc(1, 15);
            if( NULL == iov[i].sectionId.id )
            {   
                return CL_OK;
            }
            snprintf( (ClCharT *) iov[i].sectionId.id, 15,"section%d", i);
            iov[i].sectionId.idLen = strlen((ClCharT *) iov[i].sectionId.id) + 1;
        }
        iov[i].dataSize = 30;
        if( NULL != (iov[i].dataBuffer = clHeapCalloc(1, iov[i].dataSize)) )
        {
            iov[i].dataBuffer = NULL;
        }
        iov[i].readSize = 0;
        iov[i].dataOffset = 20;
    }
            rc = clCkptCheckpointRead(ckptHdl, iov, numSections, &readIdx); 
            if( CL_OK != rc )
            {
                return rc;
            }
    for( i = 0; i < numSections; i++ )
    {
        if( NULL != iov[i].sectionId.id ) 
        {
            clHeapFree(iov[i].sectionId.id);
        }
            clHeapFree(iov[i].dataBuffer);
    }
    return CL_OK;
}
