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
 * ModuleName  : utils
 * File        : clCksmApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Checksum library.
 *
 *****************************************************************************/
/** @pkg cl.ccsl */


/*********************************************************************************/
/******************************* CheckSum APIs ***********************************/
/*********************************************************************************/
/*                                                                               */
/* pageCksm101 : clCksm16bitCompute                                              */
/* pageCksm102 : clCksm32bitCompute                                              */
/*                                                                               */
/*********************************************************************************/


#ifndef _CL_CKSM_H_
#define _CL_CKSM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/**
 *  \page pageCheckSum Clovis CheckSum Library
 *
 *  \par Overview
 *
 *  The Clovis Checksum library provides a set of APIs for computing the 
 *  CheckSum.
 * 
 *  \par Interaction with other components
 *
 *  This component is used by all the components that need to compute CheckSum.
 *
 *  \par Usage Scenario(s)
 *
 *  Computes a 16-bit CheckSum.
 *
 *  \code
 *  ClUint8T *pData;
 *  ClUint16T checkSum16
 *  ClUint32T checkSum32
 *  ClRcT retCode = CL_OK;
 *
 *  retCode = clCksm16bitCompute (pData, 12, &checkSum16);
 *  retCode = clCksm32bitCompute (pData, 12, &checkSum32);
 *  \endcode
 *
 *
 */
    
/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \page pageCksm101 clCksm16bitCompute
 *
 *  \par Synopsis:
 *   Computes a 16-bit CheckSum.
 *
 *  \par Description:
 *   This API is used to compute a 16-bit CheckSum.
 *
 *  \par Syntax:
 *  \code 	ClRcT clCksm16bitCompute( 
 *                              ClUint8T *pData, 
 *				ClUint32T length, 
 *				ClUint16T *pCheckSum);
 *  \endcode
 *
 *  \param pData: Data for which CheckSum is being computed. 
 *  This must be a valid pointer and cannot be NULL.
 *
 *  \param length: Length of the data.
 *
 *  \param pCheckSum: (out) Location where the computed CheckSum would be stored 
 *  This must be a valid pointer and cannot be NULL.
 *  
 *  \retval CL_OK: The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer. 
 *
 */
ClRcT
clCksm16bitCompute (ClUint8T *pData, ClUint32T length, ClUint16T *pCheckSum);


/**
 ************************************
 *  \page pageCksm102 clCksm32bitCompute
 *
 *  \par Synopsis:
 *   Computes a 32-bit CheckSum.
 *
 *  \par Description:
 *   This API is used to compute a 32-bit CheckSum.
 *
 *  \par Syntax:
 *  \code 	ClRcT clCksm32bitCompute( 
 *                              ClUint8T *pData, 
 *				ClUint32T length, 
 *		        	ClUint32T *pCheckSum)
 *  \endcode
 *
 *  \param pData: Data for which CheckSum is being computed. 
 *  This must be a valid pointer and cannot be NULL.
 *
 *  \param length: Length of the data.
 *
 *  \param pCheckSum: (out) Location where the computed CheckSum would be stored 
 *  This must be a valid pointer and cannot be NULL.
 *
 *  \retval CL_OK: The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer. 
 * 
 */
ClRcT
clCksm32bitCompute (ClUint8T *pData, ClUint32T length, ClUint32T *pCheckSum);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CKSM_H_ */

