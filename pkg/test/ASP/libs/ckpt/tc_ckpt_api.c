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

/* The API defined in this file use gettimeofday() instead of
 * OSAL API because of the need to measure with micro-second
 * granularity for APIs
 */
#include <sys/time.h>

#include <clCkptApi.h>
#include <clHeapApi.h>
#include <tc_ckpt_api.h>

/* This parameter pre-allocates buffers
 * for the sections during creates. Rather
 * than make the API unwieldy I have chosen
 * for now to make this a compile time option
 * that can later be made into a configurable
 * parameter that can be read
 */
#define PRE_ALLOCATE_SECTION 1

static ClCkptSvcHdlT ckpt_svc_hdl = 0;
ClVersionT           ckpt_version = {'B', 1, 1};

/*******************************************************************************
Feature API: TC_Checkpoint_SvcInit

*******************************************************************************/
int
tc_ckpt_svc_init (void)
{
	ClRcT ret_code = CL_OK;

	if (ckpt_svc_hdl == 0)
	{
		ret_code = clCkptInitialize(&ckpt_svc_hdl, NULL, &ckpt_version);	
		if (ret_code != CL_OK)
		{
			printf("Failed to initialize Ckpt Service: %x\n", ret_code);
		}
	}	

	return ret_code;
}

/*******************************************************************************
Utility API: tc_activate_replica

Descrition :

This API is utility related, used to set the local checkpoint as the active
replica.  The checkpoint data is retrieved and replica activate called only if
the checkpoint creation flag is ASYNC and COLLOCATED

*******************************************************************************/
static int
tc_activate_replica ( tc_ckpt_dataT *ckpt_data )
{
	ClRcT ret_code = CL_OK;
	ClCkptCheckpointDescriptorT ckpt_desc;

	ret_code = clCkptCheckpointStatusGet((ClCkptHdlT)ckpt_data->ckpt_hdl,
										 &ckpt_desc);

	if (ret_code != CL_OK)
	{
		printf("tc_activate_replica: could not retrieve ckpt info: 0x%x\n",
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
			printf("tc_activate_replica: Failed to activate replica :0x%x\n", 
				   ret_code);
		}
	}
	return ret_code;
}


/*******************************************************************************
Utility API: tc_get_section_id

Descrition :

This API is utility related, used by the write and read feature APIs; in case 
no section ids were used during the create. 

*******************************************************************************/
static int
tc_iter_to_section_id (
	tc_ckpt_dataT* 		ckpt_data,
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
		printf("tc_iter_to_section_id: Failed initialize iteration: 0x%x\n",
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
			printf("tc_iter_to_section_id: Failed section iteration: 0x%x\n",
				   ret_code); 
			return ret_code;
		}
	}

	/* Finalize the iteration to free up internal resources
	 */
	ret_code = clCkptSectionIterationFinalize(section_iter_hdl);
	if ( ret_code != CL_OK )
	{
		printf("tc_iter_to_section_id: Failed iteration finalize: 0x%x\n",
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
Feature  API: tc_ckpt_delete

Description : This API will first call Checkpoint Close and then 
              Checkpoint delete. The latter function requires the
			  name given to the checkpoint, during Checkpoint open.

Arguments In: 
	1. tc_ckpt_dataT : contains the ckpt handle
	2. Checkpoint Name

Arguments Out:
	1. tc_ckpt_dataT : returns time taken to write the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int tc_ckpt_delete ( 
	const char*	   ckpt_name,
	tc_ckpt_dataT* ckpt_data )
{
	ClRcT 		ret_code = CL_OK;
	ClNameT 	ckpt_name_t;
	struct timeval			start_time;
	struct timeval			end_time;
	ClUint32T				time_taken_us = 0;

	/* Initiailze name struct for ckpt
	 */
	ckpt_name_t.length = strlen(ckpt_name);
	strcpy(ckpt_name_t.value, ckpt_name);

	/* time check 1 start 
	 */
	gettimeofday(&start_time, NULL );

	ret_code = clCkptCheckpointClose((ClCkptHdlT)ckpt_data->ckpt_hdl);
	if ( ret_code != CL_OK )
	{
		printf("tc_ckpt_delete: Failed to close checkpoint %s: 0x%d \n",
			   ckpt_name, ret_code); 
	}

	/* Delete checkpoint (test: check memory profile to ensure all resources
	 * are actually released)
	 */
	ret_code = clCkptCheckpointDelete( ckpt_svc_hdl, &ckpt_name_t);
	if ( ret_code != CL_OK )
	{
		printf("tc_ckpt_delete: Failed to delete checkpoint %s: 0x%d \n",
			   ckpt_name, ret_code); 
	}

	/* time check 1 end 
	 */
	gettimeofday(&end_time, NULL );
	time_taken_us = TC_TIME_TAKEN_US(end_time) - TC_TIME_TAKEN_US(start_time);

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	return ret_code;
}

/*******************************************************************************
Feature  API: tc_ckpt_create

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
	1. tc_ckpt_dataT : returns time taken to create ckpt, and ckpt_handle

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int
tc_ckpt_create (
	const char 		*ckpt_name,
	const char 		*section_name_prefix,
	int				num_sections,
	int				section_size,
	tc_ckpt_typeE	ckpt_type,
	tc_ckpt_dataT	*ckpt_data )
{

	ClRcT 								ret_code = CL_OK;
	ClNameT 							ckpt_name_t;
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

	struct timeval						start_time;
	struct timeval						end_time;
	ClUint32T							time_taken_us = 0;

	/* First ensure that the Initialize function is called
	 */
	ret_code = tc_ckpt_svc_init();
	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_create: failed to initialze service \n");
		return ret_code;
	}

	/* Initiailze name struct for ckpt
	 */
	ckpt_name_t.length = strlen(ckpt_name);
	strcpy(ckpt_name_t.value, ckpt_name);

	/* Get the max size for a  name of sectionId
	 */
	if (section_name_prefix != NULL)
	{
		sprintf(section_id_name, "%s%d", section_name_prefix, num_sections );
	}
	else
	{
		sprintf(section_id_name, "%d", num_sections);	 
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
			printf("tc_ckpt_create: (warning) invalid checkpoint type\n");
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
	gettimeofday(&start_time, NULL );

	ret_code = clCkptCheckpointOpen(ckpt_svc_hdl, 
									&ckpt_name_t, 
									&ckpt_cr_attr,
									ckpt_open_flags,
									timeout,
									( ClCkptHdlT *)(&ckpt_data->ckpt_hdl));
	/* time check 1 end
	 */
	gettimeofday(&end_time, NULL );

	time_taken_us = TC_TIME_TAKEN_US(end_time) - TC_TIME_TAKEN_US(start_time);

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_create: Failed to create ckpt:0x%x \n", ret_code);
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
		sprintf((ClCharT*)section_id.id, "%s", section_name_prefix);
		/* pointer to the part of the string that the 
	 	 * unique identifier of a section will be placed
	 	 */
		section_name_ptr = section_id.id + strlen(section_name_prefix);
	}
	else
	{
		section_name_ptr = section_id.id; 
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
		printf("tc_create_ckpt: Failed to allocate section data\n");
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
			printf("tc_ckpt_create: Failed to activate replica :0x%x\n", 
			   	   ret_code);
			goto clean_up_code;
		}
	}

	/* Create the sections within the checkpoint
	 */
	for ( section_num = 0; section_num < num_sections; section_num++ )
	{
		sprintf((ClCharT*)section_name_ptr, "%d", section_num);
		section_id.idLen = strlen((ClCharT*)section_id.id)+1;

		/* time check 2 start (cumulative)
	 	 */
		gettimeofday(&start_time, NULL );
		ret_code = clCkptSectionCreate((ClCkptHdlT)ckpt_data->ckpt_hdl,
									   &section_cr_attr,
									   NULL, 0);
		/* time check 2 end (cumulative)
		 */
		gettimeofday(&end_time, NULL );

		/* does not account for overflow
		 */
		time_taken_us = TC_TIME_TAKEN_US(end_time) - TC_TIME_TAKEN_US(start_time);
		ckpt_data->time_taken_us  += time_taken_us;

		if (ret_code != CL_OK)
		{
			printf("tc_ckpt_create: Failed to create section #%d :0x%x \n", 
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
		printf("tc_ckpt_create: Failed to delete checkpoint %s\n", ckpt_name); 
	}

	return ret_code;	
}


/*******************************************************************************
Feature  API: tc_ckpt_write

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
	1. tc_ckpt_dataT : contains the ckpt handle
	2. Section Name Prefix; if NULL no section names; if not NULL then the 
	                        section names will range from 
							<section_name_Prefix>1 to <section_name_prefix>9999
	3. section number
	4. data to write
	5. size of data to write 

Arguments Out:
	1. tc_ckpt_dataT : returns time taken to write the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int tc_ckpt_write (
	tc_ckpt_dataT* ckpt_data,
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

	struct timeval				start_time;
	struct timeval				end_time;
	ClUint32T					time_taken_us = 0;


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
		sprintf(section_id_name, "%s%d", section_name_prefix, section_num);
	}
	else
	{
		sprintf(section_id_name, "%d", section_num);
	}
	section_id.id = (ClUint8T*)section_id_name;
	section_id.idLen = strlen(section_id_name);
#else
	ret_code = tc_iter_to_section_id(ckpt_data, section_num, &section_id);

	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_write: Failed to get section id: 0x%x\n", ret_code); 
		return ret_code;
	}
#endif

	/* Set the local replica to be active 
	 * the api sets it only if required (i.e the type is async collocated)
	 */
	ret_code = tc_activate_replica(ckpt_data);
	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_write: Failed to activate ckpt: 0x%x\n", ret_code); 
		return ret_code;
	}

	/* time check 1 start 
 	 */
	gettimeofday(&start_time, NULL );
	ret_code = clCkptSectionOverwrite((ClCkptHdlT)ckpt_data->ckpt_hdl,
									  &section_id,
									  data, data_size);
	/* time check 1 end 
	 */
	gettimeofday(&end_time, NULL );
	time_taken_us = TC_TIME_TAKEN_US(end_time) - TC_TIME_TAKEN_US(start_time);

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_write: Failed to write: 0x%x\n",
			   ret_code); 
	}

	return ret_code;
}

/*******************************************************************************
Feature  API: tc_ckpt_read

Description : 

This API will not attempt to validate arguments, the thinking being that 
this is a volitional act by the invoker of this API to ensure that invalid 
parameters can be handled gracefully by the subsystem

Arguments In: 
	1. tc_ckpt_dataT : contains the ckpt handle
	2. Section Name Prefix; if NULL no section names; if not NULL then the 
	                        section names will range from 
							<section_name_Prefix>1 to <section_name_prefix>9999
	3. section number
	4. data to read
	5. size of data to read 

Arguments Out:
	1. tc_ckpt_dataT : returns time taken to read the section

Return Value:
	integer 0 if success, non zero if failure
*******************************************************************************/
int tc_ckpt_read (
	tc_ckpt_dataT* ckpt_data,
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

	struct timeval			start_time;
	struct timeval			end_time;
	ClUint32T				time_taken_us = 0;

	/* we either iterate through the sections until
	 * we get to the section of interest; or go directly
	 * to the section, assuming a known section Id; 
	 * the former is more general purpose and will be used here
	 */
#if GO_TO_SECTION_DIRECTLY
	if ( section_name_prefix != NULL )
	{
		sprintf(section_id_name, "%s%d", section_name_prefix, section_num);
	}
	else
	{
		sprintf(section_id_name, "%d", section_num);
	}

	io_vector.sectionId.id = (ClUint8T*)section_id_name;
	io_vector.sectionId.idLen = strlen(section_id_name);
#else
	ret_code = tc_iter_to_section_id(ckpt_data, section_num, &io_vector.sectionId);
	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_write: Failed to get section id: 0x%x\n", ret_code); 
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
	gettimeofday(&start_time, NULL );
	ret_code = clCkptCheckpointRead((ClCkptHdlT)ckpt_data->ckpt_hdl,
									&io_vector, 1, &error_index );
	/* time check 1 end 
	 */
	gettimeofday(&end_time, NULL );
	time_taken_us = TC_TIME_TAKEN_US(end_time) - TC_TIME_TAKEN_US(start_time);

	ckpt_data->time_taken_ms = 0;
	ckpt_data->time_taken_us  = time_taken_us;

	if (ret_code != CL_OK)
	{
		printf("tc_ckpt_read: Failed to read: 0x%x\n", ret_code); 
		return ret_code;
	}
	else
	{
		if ( io_vector.readSize != data_size )
		{
			printf("tc_ckpt_read: Read %d bytes; expected %d bytes\n", 
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
