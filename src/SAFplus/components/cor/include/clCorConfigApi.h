/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : cor                                                           
 * File        : clCorConfigApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  The file contains all the MetaData data structures.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Configuration related to COR Transaction
 *  \ingroup cor_apis
 */


/**
 *  \addtogroup cor_apis
 *  \{
 */


#ifndef _CL_COR_CONFIG_API_H_
#define _CL_COR_CONFIG_API_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>

/**
 * Configuration related to COR transaction.
 */

typedef struct ClCorComponentConfig {

/**
 * Either periodically save the data in COR per transaction or do not save at all.
 */
     ClUint32T      saveType;       
     ClUint32T   OHType;
     ClUint8T  *OHMask;       	
} ClCorComponentConfigT;

typedef ClCorComponentConfigT *ClCorComponentConfigPtrT;


/*****  APIs *******/

#ifdef __cplusplus
}
#endif


#endif  /* _CL_COR_CONFIG_API_H_  */


/** \} */
