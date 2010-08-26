/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorNotifyCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * The file contains all the internal MetaData  definitions.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_INT_META_DATA_H_

#define _CL_COR_INT_META_DATA_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <clCommon.h>
/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
#define CL_COR_CLI_STR_LEN   1024
#define CL_COR_UNKNOWN_ATTRIB  -1
#define CL_COR_MAX_ATTR_PER_OBJ (2048)

/******************************************************************************
 *  Structures
 *****************************************************************************/
 
/* TYPEDEFS */  
typedef struct ClCorAttrBits
{
  ClUint8T  attrBits[CL_COR_MAX_ATTR_PER_OBJ/8];
} ClCorAttrBitsT;
typedef ClCorAttrBitsT * ClCorAttrBitsPtrT;

typedef struct CorEventPattern
{
  	ClCorOpsT         op;            /* change/create/delete */
	ClCorMOIdT	   moId;	
        ClCorAttrPathT    attrPath;      /* containment hierarchy, if any */
	ClCorAttrBitsT    attrBits;         /* change bits, if op is 'change' op */
}CorEventPatternT;

typedef CorEventPatternT* CorEventPatternH;

/*
 * Following macros are used for preparing RBE expression
 * to match moId and changed bits. RBE expression works on
 * bit matching, which means exact position of the data being
 * matched needs to be known. These macros ensure that offset
 * is autmatically is taken care of if there is any change in
 * the definition of COREvt_t.
 */
#define COR_NOTIFY_OPS_OFFSET       ((ClWordT)&(((CorEventPatternH)0)->op))
#define COR_NOTIFY_MOID_OFFSET      ((ClWordT)&(((CorEventPatternH)0)->moId))
#define COR_NOTIFY_ATTRPATH_OFFSET  ((ClWordT)&(((CorEventPatternH)0)->attrPath))
#define COR_NOTIFY_ATTR_BITS_OFFSET ((ClWordT)&(((CorEventPatternH)0)->attrBits))

/*
 *  Internal Utility IPIs
 */
extern ClRcT clCorMoClassPathValidate(ClCorMOClassPathPtrT this);



#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_META_DATA_H_ */
