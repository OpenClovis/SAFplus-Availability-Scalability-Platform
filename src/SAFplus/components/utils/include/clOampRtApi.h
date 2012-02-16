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
 * ModuleName  : utils
 * File        : clOampRtApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/




/*************************************************************************/
/************************** Oamp Rt APIs *********************************/
/*************************************************************************/
/*                                                                       */
/* pageOampRt01 : clOampRtResourceInfoGet                                */
/*                                                                       */
/*************************************************************************/


#ifndef _CL_OAMP_RT_API_H_
#define _CL_OAMP_RT_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clParserApi.h>
#include <clList.h>

/**
 *  Resource information.
 */
typedef struct ClOampRtResource
{

/**
 *  It value is \c CL_TRUE, if wildcard is present or else it is \c CL_FALSE.
 */
    ClUint32T wildCardFlag;   

/**
 *  Determines whether to pre-create MO or not.
 */     
    ClUint32T objCreateFlag;   

/*
 * This should be used instead of objCreateFlag which is retained
 * for backward compatibility.
 */
    ClBoolT   autoCreateFlag;

/**
 * Primary Oi flag is true of false.
 */
    ClUint32T primaryOIFlag;    

/**
 *  Resource Name.
 */   
    ClNameT   resourceName;        

/*
 * Depth of the resource in the MO tree.
 */
    ClUint32T depth;

}ClOampRtResourceT;

/**
 *  Resource information array.
 */
typedef struct ClOampRtResourceArray
{

/**
 *  Number of resources.
 */
    ClUint32T noOfResources;  

/**
 *  Pointer to resource information.
 */      
    ClOampRtResourceT* pResources;  

/*
 * Config file pertaining to the resources.
 */
    ClCharT resourceFile[CL_MAX_NAME_LENGTH];

}ClOampRtResourceArrayT;

typedef struct ClOampRtCompResourceArray
{
    ClCharT compName[CL_MAX_NAME_LENGTH];
    ClOampRtResourceArrayT resourceArray;
    ClListHeadT list; /*per component resource table map*/
}ClOampRtComponentResourceArrayT;

/**
 ************************************
 *  \page pageOampRt101 clOampRtResourceInfoGet
 *
 *  \par Synopsis:
 *  Returns the resource information from RT file.
 *
 *  \par Description:
 *  This API used to retrieve the resource information which are controlled by a given component.
 *
 *  \par Syntax:
 *  \code 	ClRcT clOampRtResourceInfoGet(
 *                          CL_IN ClParserPtrT top,
 *                          CL_IN ClNameT* pCompName,
 *                          CL_OUT ClOampRtComponentResourceArrayT* pResourceArray);
 *  \endcode
 *
 *  \param pRtFileName: Pointer to parser structure of resource file name.
 *  \param pCompName: Pointer to component name.
 *  \param pResourceArray: Pointer to resource information array. 
 *   You must free the memory allocated to \c pResourceArray->pResources by the RT library.
 *
 *  \retval CL_OK: The API executed successfully. 
 *  \retval CL_OAMP_RT_INTERNAL_ERROR: An unexpected error occured within the RT parser.
 *  \retval CL_OAMP_RT_FILE_NOT_EXIST: If resource file does not exist.
 *  \retval CL_OAMP_RT_ERR_INVALID_ARG: On passing invalid arguments.
 *  \retval CL_OAMP_RT_ERR_INVALID_CONFIG: If the resource file contains invalid configuration.
 * 
 */


ClRcT clOampRtResourceInfoGet(CL_IN ClParserPtrT top, CL_IN ClNameT* pCompName, CL_OUT ClOampRtResourceArrayT* pResourceArray);

ClRcT clOampRtResourceScanFiles(const ClCharT *dirName, const ClCharT *suffix, 
                                ClInt32T suffixLen, ClOampRtResourceArrayT **ppResourceArray,
                                ClUint32T *pNumScannedResources,
                                ClListHeadT *pCompResourceList);

#ifdef __cplusplus
}
#endif

#endif /* _CL_OAMP_RT_API_H_ */

