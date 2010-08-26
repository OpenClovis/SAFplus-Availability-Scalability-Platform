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

#define TC_TIME_TAKEN_US(time_val)   \
		((time_val.tv_sec * 1000000) + time_val.tv_usec)

/* enum to keep track of check-point type
 * as internal representation of sync versus
 * async is obfuscated
 */
typedef enum ClTcCkptTypeE
{
	TC_CKPT_SYNC,
	TC_CKPT_ASYNC_COLLOC,
	TC_CKPT_ASYNC_NON_COLLOC,
	TC_CKPT_INVALID = 0xFFFF
} ClTcCkptTypeE;

/* ckpt_hdl returned by Create
 * used as input in subsequent
 * calls to read, write, delete
 */
typedef struct ClTcCkptDataT
{
	unsigned int  	ckpt_hdl;
	ClTimeT         time_taken_ms;
	ClTimeT         time_taken_us;
} ClTcCkptDataT;

int clTcCkptDelete ( 
	const char*	   ckpt_name,
	ClTcCkptDataT* ckpt_data );

int clTcCkptCreate (
	const char*		ckpt_name,
	const char*		section_name_prefix,
	int				num_sections,
	int				section_size,
	ClTcCkptTypeE	ckpt_type,
	ClTcCkptDataT	*ckpt_data );

int clTcCkptWrite (
	ClTcCkptDataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   size_data);

int clTcCkptRead (
	ClTcCkptDataT* ckpt_data,
	const char*	   section_name_prefix,
	int			   section_num,
	void*		   data,
	int			   size_data);

int clTcCkptSvcInit (void);

extern ClRcT
clTestCkptCreate(ClCkptSvcHdlT svcHandle,
                 ClCkptCreationFlagsT ckptType,
                 ClUint32T     ckptIdx, 
                 ClUint32T     numSections,
                 ClUint32T     size,
                 ClCkptHdlT    *pCkptHdl);

extern ClRcT
clTestCkptRead(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections,
               ClTimeT    *pTime);
extern ClRcT
clTestSectionCreate(ClCkptHdlT  ckptHdl, 
                    ClUint32T   sectionIdx);

extern ClRcT
clTestSectionOverwrite(ClCkptHdlT  ckptHdl,
                      ClUint32T   numSections,
                      ClUint32T   sectionSize,
                      ClTimeT     *pTime);
extern ClRcT
clTestSectionOverwrite_withTime(ClCkptHdlT  ckptHdl,
                      ClUint32T   numSections,
                      ClUint32T   sectionSize);
extern ClRcT
clTestCkptRead_withTime(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections);
extern ClRcT
clTestCheckpointReadWithOffSet(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections);
extern ClRcT
clTestCheckpointWrite(ClCkptHdlT  ckptHdl,
               ClUint32T   numSections);

#endif /*TC_CKPT_API_H*/
