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
#ifndef TC_CKPT_API_H
#define TC_CKPT_API_H

/* TC_MAX_CKPTS_TESTABLE needed to keep track of section Id names
 * this can be made a configurable parameter
 */
#define TC_CKPT_MAX_SECTION_ID_SIZE 256

#define TC_TIME_TAKEN_US(time_val)   ((time_val.tv_sec * 1000000) + time_val.tv_usec)

/* enum to keep track of check-point type
 * as internal representation of sync versus
 * async is obfuscated
 */
typedef enum tc_ckpt_typeE
{
	TC_CKPT_SYNC,
	TC_CKPT_ASYNC_COLLOC,
	TC_CKPT_ASYNC_NON_COLLOC,
	TC_CKPT_INVALID = 0xFFFF
} tc_ckpt_typeE;

/* ckpt_hdl returned by Create
 * used as input in subsequent
 * calls to read, write, delete
 */
typedef struct tc_ckpt_dataT
{
	unsigned int  	ckpt_hdl;
	unsigned long	time_taken_ms;
	unsigned long	time_taken_us;
} tc_ckpt_dataT;

int tc_ckpt_delete ( 
	const char*	   ckpt_name,
	tc_ckpt_dataT* ckpt_data );

int tc_ckpt_create (
	const char*		ckpt_name,
	const char*		section_name_prefix,
	int				num_sections,
	int				section_size,
	tc_ckpt_typeE	ckpt_type,
	tc_ckpt_dataT	*ckpt_data );

int tc_ckpt_write (
	tc_ckpt_dataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   size_data);

int tc_ckpt_read (
	tc_ckpt_dataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   size_data);

int tc_ckpt_svc_init (void);

#endif /*TC_CKPT_API_H*/
