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
 * File        : clCorUtilityApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *  The file contains utility APIs to modify paths/ids required by core COR
 *  APIs. for e.g APIs to generate ClCorMOId (a unique idenfier for MO)
 *
 *
 *****************************************************************************/

/*********************************************************************************/
/********************************* COR IPIs **************************************/
/*********************************************************************************/
/*                                                                              */
/* clCorProcessClassTable                                                       */
/* clCorOmClassFromInfoModelGet                                                 */
/* clCorDataRestore                                                             */
/* clCorDataSave                                                                */
/* clCorDataFrequentSave                                                        */
/* clCorDataFrequentSaveStop                                                    */
/* clCorClassAttributeWalk                                                      */
/* clCorClassAttributeTypeGet                                                   */
/* clCorClassAttributeValuesGet                                                 */
/* clCorMoClassPathInitialize                                                   */
/* clCorMoClassPathAlloc                                                        */
/* clCorMoClassPathFree                                                         */
/* clCorMoClassPathTruncate                                                     */
/* clCorMoClassPathSet                                                          */
/* clCorMoClassPathAppend                                                       */
/* clCorMoClassPathDepthGet                                                     */
/* clCorMoClassPathToMoClassGet                                                 */
/* clCorMoClassPathFirstChildGet                                                */
/* clCorMoClassPathNextSiblingGet                                               */
/* clCorMoClassPathCompare                                                      */
/* clCorMoClassPathConcatenate                                                  */
/*********************************************************************************/
/********************************* COR APIs **************************************/
/*********************************************************************************/
/* clCorMoIdInitialize                                                           */
/* clCorMoIdAlloc                                                                */
/* clCorMoIdFree                                                                 */
/* clCorMoIdTruncate                                                             */
/* clCorMoIdSet                                                                  */
/* clCorMoIdAppend                                                                */
/* clCorMoIdDepthGet                                                             */
/* clCorMoIdShow                                                                 */
/* clCorMoIdToMoClassGet                                                         */
/* clCorMoIdNameToMoIdGet                                                        */
/* clCorMoIdToMoIdNameGet                                                        */
/* clCorMoIdFirstInstanceGet                                                     */
/* clCorMoIdNextSiblingGet                                                       */
/* clCorMoIdValidate                                                             */
/* clCorMoIdToInstanceGet                                                        */
/* clCorMoIdToMoClassPathGet                                                     */
/* clCorMoIdClone                                                                */
/* clCorMoIdCompare                                                              */
/* clCorMoIdServiceGet                                                           */
/* clCorMoIdServiceSet                                                           */
/* clCorServiceIdValidate                                                        */
/* clCorMoIdInstanceSet                                                          */
/* clCorMoIdConcatenate                                                          */
/* clCorAttrPathAlloc                                                            */
/* clCorAttrPathInitialize                                                       */
/* clCorAttrPathFree                                                             */
/* clCorAttrPathTruncate                                                         */
/* clCorAttrPathSet                                                              */
/* clCorAttrPathAppend                                                           */
/* clCorAttrPathShow                                                             */
/* clCorAttrPathDepthGet                                                         */
/* clCorAttrPathToAttrIdGet                                                      */
/* clCorAttrPathIndexGet                                                         */
/* clCorAttrPathIndexSet                                                         */
/* clCorAttrPathCompare                                                          */
/* clCorAttrPathClone                                                            */
/* clCorUtilMoAndMSOCreate                                                       */
/* clCorUtilMoAndMSODelete                                                       */
/*                                                                               */
/*********************************************************************************/

/**
 *  \file
 *  \brief Header File of COR utility APIs
 *  \ingroup cor_apis
 */

/**
 *  \addtogroup cor_apis
 *  \{
 */

#ifndef _CL_COR_UTILITY_API_H_
#define _CL_COR_UTILITY_API_H_

#ifdef __cplusplus
    extern "C" {
#endif

#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include "clCorMetaData.h"
#include "clCorServiceId.h"


/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/


#define CL_COR_UTILS_UNKNOWN_ATTRIBUTE 0

#define CL_COR_UTILS_UNKNOWN_CLASS  0
    
/**
 *   Pre-provisioning related structures.
 */
typedef enum 
{

/**
 * COR classes data.
 */
   CL_COR_PERS_CLASS_DATA = 1,      

/**
 * COR class relationship data.
 */
   CL_COR_PERS_HIER_DATA,           

/**
 * COR objects data.
 */
   CL_COR_PERS_OBJ_DATA,            

   CL_COR_PERS_RM_DATA,

   CL_COR_PERS_NI_DATA,

   CL_COR_PERS_DATA_MAX   

} ClCorPersDataTypeT;
    
/**
 *   Class specification.
 */
typedef enum ClCorClsCfgType {

    CL_COR_BASE_CLASS,
    CL_COR_MO_CLASS

} ClCorClsCfgTypeT;


struct ClCorPersDataTlv
{

/**
 * Type of the persistent data.
 */
  ClCorPersDataTypeT     type;     

/**
 * Length of the  persistent data.
 */
  ClInt32T             len;      

};

typedef struct ClCorPersDataTlv  ClCorPersDataTlvT;



/*****************************************************************************
 *  COR APIs
 *****************************************************************************/




extern ClRcT clCorOmClassFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass);
extern ClRcT clCorOmClassNameFromInfoModelGet(ClCorClassTypeT moClass, ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass,
                                              ClCharT *pOmClassName, ClUint32T maxClassSize);
extern ClRcT clCorConfigLoad(const ClCharT *pConfigFile, const ClCharT *pRouteFile);

extern ClRcT clCorDataRestore();

extern ClRcT clCorDataSave();



extern ClRcT clCorDataFrequentSave(char  *fileName, ClTimerTimeOutT  frequency );



extern ClRcT clCorDataFrequentSaveStop();


/* ------------- Attribute related utility IPIs ------------------ */


extern ClRcT  clCorClassAttributeWalk(ClCorClassTypeT classId,  ClCorClassAttrWalkFunc usrClBck, ClPtrT cookie);




extern ClRcT  clCorClassAttributeTypeGet(ClCorClassTypeT clsHdl, ClCorAttrIdT attrId,  ClCorAttrTypeT* atye);




extern ClRcT  clCorClassAttributeValuesGet(ClCorClassTypeT clsHdl, ClCorAttrIdT attrId,
                             ClInt32T *pInit, ClInt32T *pMin, ClInt32T *pMax);




/* ------------- ClCorMOClassPath and ClCorMOId Utilities -------------------- */


extern ClRcT clCorMoClassPathInitialize(ClCorMOClassPathPtrT pPath);




extern ClRcT clCorMoClassPathAlloc(ClCorMOClassPathPtrT *pPath);




extern ClRcT clCorMoClassPathFree(ClCorMOClassPathPtrT pPath);




extern ClRcT clCorMoClassPathTruncate(ClCorMOClassPathPtrT pPath, ClInt16T level);




extern ClRcT clCorMoClassPathSet(ClCorMOClassPathPtrT pPath, ClUint16T level, ClCorClassTypeT type);




extern ClRcT clCorMoClassPathAppend(ClCorMOClassPathPtrT pPath, ClCorClassTypeT type);



extern ClInt32T clCorMoClassPathDepthGet(ClCorMOClassPathPtrT pPath);



extern ClCorClassTypeT clCorMoClassPathToMoClassGet(ClCorMOClassPathPtrT pPath);




extern ClRcT clCorMoClassPathFirstChildGet(ClCorMOClassPathPtrT pPath);




extern ClRcT clCorMoClassPathNextSiblingGet(ClCorMOClassPathPtrT pPath);  




extern ClRcT clCorMoClassPathCompare(ClCorMOClassPathPtrT hMoPath1, ClCorMOClassPathPtrT hMoPath2);



extern ClRcT clCorMoClassPathConcatenate(ClCorMOClassPathPtrT part1, ClCorMOClassPathPtrT part2, int copyWhere);

/**
 ************************************
 *  \brief Initializes a MOID or resets the content of an existing MOID.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) A pointer to an existing \e MoId structure.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *
 *  \par Description:
 *  This API is used to initialize the ClCorMoId structure. It resets the path information, if
 *  present, and initializes it to make it an empty path. It can be applied on MOIDs that have not
 *  been initialized and used before, and also on MOIDs containing a path. The empty MoIds
 *  can be manipulated using clCorMoIdSet() and clCorMoIdAppend().
 *  \par
 *  This API can also be called when the MoId needs to be reset and begin a fresh operation.
 *
 *  \note
 *  This function need not be called after invoking the clCorMoIdAlloc function, 
 *  since the latter initializes the \e moId.
 * 
 *  \par Library File:
 *   ClCorClient
 * 
 *  \sa clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet, clCorMoIdAppend,
 *  clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet, clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, 
 *  clCorMoIdFirstInstanceGet, clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdInitialize(CL_INOUT ClCorMOIdPtrT pMoId);

/**
 ************************************
 *  \brief Creates an MoId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (out) Handle of new \e MOId.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *  \retval CL_COR_ERR_NULL_PTR \e pMoId a NULL pointer.
 *
 *  \par Description:
 *  This API is used as a constructor for \e ClCorMOId as it creates a \e MoId. It initializes 
 *  the memory and returns an empty \e ClCorMOId.
 *  \par 
 *  By default, the value for both instance ID and class ID is \e -1. This function allocates
 *  memory and initializes the \e MoId structure.
 * 
 *  \par Library File:
 *   ClCorClient
 * 
 *  \note
 *  The memory allocated by this function must be freed after usage, to avoid memory leaks. This memory can
 *  can be freed by calling the clCorMoIdFree API.
 * 
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdAlloc(CL_INOUT ClCorMOIdPtrT *pMoId);

/**
 ************************************
 *  \brief Deletes the \e ClCorMOId handle.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of \e ClCorMOId.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *
 *  \par Description:
 *  This API is used as a destructor of \e ClCorMOId as it deletes the handle of \e MoId.  It 
 *  removes the handle and frees the memory associated with it. To avoid memory leaks the user must 
 *  free the memory using this API whenever he allocates memory using clCorMoIdAlloc() API.
 * 
 *  \par Library File:
 *   ClCorClient
 * 
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdTruncate, clCorMoIdSet, clCorMoIdAppend,
 *  clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdFree(CL_INOUT ClCorMOIdPtrT pMoId);

/**
 ************************************
 *  \brief Removes the node after specified level.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of the \e MoId.
 *  \param level (in) Level to which the \e MoId needs to be truncated.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_INVALID_DEPTH If the level specified is invalid.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *
 *  \par Description:
 *  This API is used to remove all the nodes and reset the \e MoId until a specified level is reached.
 *  The level is specified based on the depth to which it is required to return so that
 *  the operation can continue on the other sibling tree node.
 *
 *  \par Library File:
 *  ClCorClient
 * 
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 * 
 */
extern ClRcT clCorMoIdTruncate(CL_INOUT ClCorMOIdPtrT pMoId, CL_IN ClInt16T level);


/**
 ************************************
 *  \brief Sets the class type and \e instanceId at a given node or level. 
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of \e MOId Path.
 *  \param level (in) Level of the node
 *  \param type (in) Class type to be set.
 *  \param instance (in) \e InstanceId to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_INVALID_DEPTH If the level specified is invalid.
 *  \retval CL_COR_ERR_NULL_PTR MOID is a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_CLASS Invalid class type is set. 
 *
 *  \par Description:
 *  This API is used to set the class type and the \e instanceId properties at a given node or level.
 *  This level should be less than the current depth of the moId. This feature can be used to
 *  set the MoId for a level when it is required to operate on that part of the instance tree.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \note
 *  The second parameter level should be always less than the current depth of the moId.
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 * 
 */
extern ClRcT clCorMoIdSet(CL_INOUT ClCorMOIdPtrT pMoId, CL_IN ClUint16T level, 
                                    CL_IN ClCorClassTypeT type, CL_IN ClCorInstanceIdT instance);


/**
 ************************************
 *  \brief Adds an entry to the MoId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of the \e MOId
 *  \param type (in) Node type.
 *  \param instance (in) ID of the node instance.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_MAX_DEPTH If depth exceeded the maximum limit.
 *  \retval CL_COR_ERR_INVALID_CLASS Invalid class to for append.
 *  \retval CL_COR_ERR_INVALID_PARAM Invalid parameter is passed.
 *
 *  \par Description:
 *  This API is used to add an entry to ClCorMOId containing the type and the instance. The
 *  classId and instance ID are appended at the end of the current MOID and the depth of
 *  the MOID is incremented.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 * 
 */
extern ClRcT clCorMoIdAppend(CL_INOUT ClCorMOIdPtrT pMoId, CL_IN ClCorClassTypeT type, CL_IN ClCorInstanceIdT instance);


/**
 ************************************
 *  \brief Returns node depth of the COR \e MoId
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) Handle of the \e MOId.
 * 
 *  \retval ClInt16T The number of elements in the MoId.
 *
 *  \par Description:
 *  This API is used to return the number of nodes in the hierarchy within the COR \e MoId.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdShow clCorMoIdToMoClassGet, clCorMoIdNameToMoIdGet,
 *  clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet, clCorMoIdNextSiblingGet,
 *  clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet, clCorMoIdClone,
 *  clCorMoIdCompare.
 * 
 */
extern ClInt16T clCorMoIdDepthGet(CL_IN ClCorMOIdPtrT pMoId);

/**
 ************************************
 *  \brief Displays the ClCorMOId handle.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId Handle of the \e MOId.
 * 
 *  \retval None
 *
 *  \par Description:
 *  This API is used to display all the entries within the COR MOID. This function displays the
 *  current active nodes (classId:instanceId format) in the MOID and its service ID.
 * 
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 * 
 */
extern void clCorMoIdShow(CL_IN ClCorMOIdPtrT pMoId);


/**
 ************************************
 *  \brief Returns the class type.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) Handle of the \e MOId.
 *  \param flag (in) Specifies type of MO class to be returned. Its value can be
 *  either \c  CL_COR_MO_CLASS_GET or \c CL_COR_MSO_CLASS_GET. 
 *  \param pClassId (out) The \e classId of the class.
 * 
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId or pClassId is a NULL pointer.
 *  \retval CL_COR_ERR_CLASS_INVALID_PATH The MO-path specified in the pMoId is invalid.
 *  \retval CL_COR_SVC_ERR_INVALID_ID The service ID specified in the pMoId is invalid.
 *  \retval CL_COR_ERR_INVALID_PARAM The flag specified is invalid.
 *
 *  \par Description:
 *  This API is used to return the class type within the COR MOID. It refers to the class type at
 *  the end of the hierarchy.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 * 
 */
extern ClRcT clCorMoIdToClassGet(CL_IN ClCorMOIdPtrT pMoId, CL_IN ClCorMoIdClassGetFlagsT flag,
                                                           CL_OUT ClCorClassTypeT *pClassId);
/**
 ************************************
 *  \brief Retrieves \e moId in ClCorMOIdT format, when \e moId is provided in ClNameT format.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param moIdName \e MoId in String format.
 *  \param  moId (out) Returns the \e MoId. It has to be allocated by the user.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_COR_ERR_NULL_PTR moIdName or moId is a NULL pointer.
 *
 *  \par Description:
 *  This function is used to retrieve \e moId in ClCorMOIdT format, when \e moId is given in ClNameT format.
 * 
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, 
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdNameToMoIdGet(CL_IN ClNameT *moIdName, CL_OUT ClCorMOIdT *moId);

/**
 ************************************
 *  \brief Retrieves MoId in ClNameT format, when MoId is provided in ClCorMOIdT format. 
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param moId (in) \e MoId in ClCorMOIdT format.
 *  \param moIdName (out) \e MoId in ClNameT format.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR moIdName or moId is a NULL pointer.
 *
 *  \par Description:
 *  This API is used to retrieve MOID in ClNameT format, when MOID is provided in
 *  ClCorMOIdT format.
 * 
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow,
 *  clCorMoIdNameToMoIdGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdToMoIdNameGet(CL_IN ClCorMOIdT *moId, CL_OUT ClNameT *moIdName);

/**
 ************************************
 *  \brief Returns the first child.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Updated \e ClCorMOId is returned. Memory allocation for the \e MoId must be done by you.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_PARAM pMoId contains an invalid service ID.
 *  \retval CL_COR_MO_TREE_ERR_NODE_NOT_FOUND The node is not found in the MO tree.
 *  \retval CL_COR_INST_ERR_NODE_NOT_FOUND The node is not found in the object instance tree.
 *
 *  \par Description:
 *  This function is used to retrieve the first child instance from the \e MoId tree. On successful execution,
 *  it updates the COR \e MoId.
 *
 *  \par Library File:
 *   ClCorClient
 *
 *  \note
 *  The last node class tag must be completed and \e instanceId must be left at zero which will
 *  be updated by this function.
 *  \sa
 *  clCorMoIdInitialize(),
 *  clCorMoIdAlloc(),
 *  clCorMoIdFree(),
 *  clCorMoIdTruncate(),
 *  clCorMoIdSet(),
 *  clCorMoIdAppend(),
 *  clCorMoIdDepthGet(),
 *  clCorMoIdShow(),
 *  clCorMoIdToMoClassGet(),
 *  clCorMoIdNameToMoIdGet(),
 *  clCorMoIdToMoIdNameGet(),
 *  clCorMoIdNextSiblingGet(),
 *  clCorMoIdValidate(),
 *  clCorMoIdToInstanceGet(),
 *  clCorMoIdToMoClassPathGet(),
 *  clCorMoIdClone(),
 *  clCorMoIdCompare()
 *
 */
extern ClRcT clCorMoIdFirstInstanceGet(CL_INOUT ClCorMOIdPtrT pMoId);  


/**
 ************************************
 *  \brief Returns the next sibling.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Updated \e ClCorMOId is returned. You must allocate the memory for the \e MoId.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \par Description:
 *  This function is used to return the next sibling from the \e MOId tree.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdInitialize(),
 *  clCorMoIdAlloc(),
 *  clCorMoIdFree(),
 *  clCorMoIdTruncate(),
 *  clCorMoIdSet(),
 *  clCorMoIdAppend(),
 *  clCorMoIdDepthGet(),
 *  clCorMoIdShow(),
 *  clCorMoIdToMoClassGet(),
 *  clCorMoIdNameToMoIdGet(),    
 *  clCorMoIdToMoIdNameGet(),    
 *  clCorMoIdFirstInstanceGet(),
 *  clCorMoIdValidate(),
 *  clCorMoIdToInstanceGet(),    
 *  clCorMoIdToMoClassPathGet(),
 *  clCorMoIdClone(),
 *  clCorMoIdCompare()
 *
 */
extern ClRcT clCorMoIdNextSiblingGet(CL_INOUT ClCorMOIdPtrT pMoId);  
 

/**
 ************************************
 *  \brief Validates \e MoId in the input argument.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId Handle of the \e MOId.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \par Description:
 *  This function is used to validate the \e MoId passed in the input argument.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdInitialize(),
 *  clCorMoIdAlloc(),
 *  clCorMoIdFree(),
 *  clCorMoIdTruncate(),
 *  clCorMoIdSet(),
 *  clCorMoIdAppend(),
 *  clCorMoIdDepthGet(),
 *  clCorMoIdShow(),
 *  clCorMoIdToMoClassGet(),
 *  clCorMoIdNameToMoIdGet(),    
 *  clCorMoIdToMoIdNameGet(),    
 *  clCorMoIdFirstInstanceGet(),
 *  clCorMoIdNextSiblingGet(),
 *  clCorMoIdToInstanceGet(),    
 *  clCorMoIdToMoClassPathGet(),
 *  clCorMoIdClone(),
 *  clCorMoIdCompare()
 *
 */
 
extern ClRcT clCorMoIdValidate(CL_IN ClCorMOIdPtrT pMoId);


/**
 ************************************
 *  \brief Returns the instance.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) Handle of the MoId.
 * 
 *  \retval ClCorInstanceIdT Associated Instance ID.
 *
 *  \par Description:
 *  This API is used to return the instance that is being queried. It refers to the class type
 *  and instance ID at the end of the hierarchy. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClCorInstanceIdT clCorMoIdToInstanceGet(CL_IN ClCorMOIdPtrT pMoId);


/**
 ************************************
 *  \brief Derives the COR path from given a \e MoId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param moIdh (in) Handle of the \e MoId.
 *  \param corIdh (out) Handle of the updated COR path. 
 * 
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR moIdh or corIdh is a NULL pointer.
 *
 *  \par Description:
 *  This API is used to obtain the MO-path from the \e MoId. The application should allocate
 *  memory for both MoId and MO path.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdClone, clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdToMoClassPathGet(CL_IN ClCorMOIdPtrT moIdh, CL_OUT ClCorMOClassPathPtrT corIdh);


/**
 ************************************
 *  \brief Clones a particular \e MOId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) Handle of the \e MoId.
 *  \param newH (out) Handle of the new clone.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *
 *  \par Description:
 *  This API is used to clone a particular MOID. It allocates and copies the contents of the given
 *  MoId to a new MoId.
 * 
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdCompare.
 *
 */
extern ClRcT clCorMoIdClone(ClCorMOIdPtrT pMoId, ClCorMOIdPtrT* newH);


ClBoolT
clCorMoIdIsWildCard(ClCorMOIdPtrT pMoId);

/**
 ************************************
 *  \brief Compares two MoIds and verifies if they are equal. 
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId Handle of the first \e ClCorMOId.
 *  \param cmp Handle of the second \e ClCorMOId.
 *
 *  \retval 0 Both COR MOIDs match each other.
 *  \retval -1 MOIDs do not match.
 *  \retval 1 The wildcards of MOIDs match each other. 
 *
 *  \par Description:
 *  This API is used to compare two ClCorMOId and verify if they are equal. This comparison
 *  is performed till the depth specified in the MOID is reached.
 * 
 *  \par Library File:
 *   ClCorClient 
 *
 *  \sa
 *  clCorMoIdInitialize, clCorMoIdAlloc, clCorMoIdFree, clCorMoIdTruncate, clCorMoIdSet,
 *  clCorMoIdAppend, clCorMoIdDepthGet, clCorMoIdShow, clCorMoIdToMoClassGet,
 *  clCorMoIdNameToMoIdGet, clCorMoIdToMoIdNameGet, clCorMoIdFirstInstanceGet,
 *  clCorMoIdNextSiblingGet, clCorMoIdToInstanceGet, clCorMoIdToMoClassPathGet,
 *  clCorMoIdClone.
 *
 */
extern int clCorMoIdCompare(CL_IN ClCorMOIdPtrT pMoId, CL_IN ClCorMOIdPtrT cmp);
extern int clCorMoIdSortCompare(CL_IN ClCorMOIdPtrT pMoId, CL_IN ClCorMOIdPtrT cmp);

/**
 ************************************
 *  \brief Returns the service ID.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) Handle of the \e MoId.
 *
 *  \retval ClCorMOServiceIdT (out) Service Id of the \e MoId specified.
 *
 *  \par Description:
 *  This API is used to return the service ID associated with the COR \e MoId. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdServiceSet(),
 *
 */
extern ClCorMOServiceIdT clCorMoIdServiceGet(CL_IN ClCorMOIdPtrT pMoId);


/**
 ************************************
 *  \brief Sets the service ID.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of the MoId.
 *  \param svc (in) Service Id to be set.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *  \retval CL_COR_ERR_INVALID_MSP_ID The service ID is invalid.
 *
 *  \par Description:
 *  This API is used to set the service ID for a particular COR \e MoId. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdServiceGet(),
 *
 */
 
extern ClRcT clCorMoIdServiceSet(CL_INOUT ClCorMOIdPtrT pMoId, CL_IN ClCorMOServiceIdT svc);


/**
 ************************************
 *  \brief Validates \e serviceId in the input argument.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param srvcId Argument passed in \e serviceId.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \par Description:
 *  This function is used to validate the \e serviceId passed in the input argument.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdServiceGet(),
 *  clCorMoIdServiceSet()
 *
 */
 
extern ClRcT clCorServiceIdValidate(CL_IN ClCorServiceIdT srvcId);


/**
 ************************************
 *  \brief Sets the instance of the MoId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in/out) Handle of the \e MOId.
 *  \param ndepth (in) Depth at which the instance is to be set.
 *  \param newInstance (in) The instance of the MoId that needs to be set.
 * 
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_INST_ERR_INVALID_MOID The \e ndepth value is greater than the depth of the
 *  MOID.
 *
 *  \par Description:
 *  This API is used to set the instance field at a specified level of \e MoId. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdFirstInstanceGet()
 *
 */
 
extern ClRcT clCorMoIdInstanceSet(CL_INOUT ClCorMOIdPtrT pMoId, CL_IN ClUint16T ndepth,
                           CL_IN ClCorInstanceIdT newInstance);


/**
 ************************************
 *  \brief Concatenates a \e MoId to another \e MoId.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param part1 First \e MoId.
 *  \param part2 Second \e MoId.
 *  \param copyWhere Indicates the place where the concatenated \e MoId has to be copied. 
 *  \arg 0 : part2 is concatenated with part1 and the result is stored in part1.
 *  \arg 1 : part1 is concatenated with part2 and the result is stored in part2.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_ERR_MAX_DEPTH The depth exceeds the maximum limit.
 *  \retval CL_COR_ERR_NULL_PTR part1 or part2 is a NULL pointer.
 *  \retval CL_COR_ERR_MAX_DEPTH The depth of (part1 + part2) exceeds the maximum
 *  depth of \e ClCorMOIdT.
 *
 *  \par Description:
 *  This API is used to concatenate two \e MoIds.
 *  
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorMoIdAppend(), clCorMoIdTruncate()
 * 
 */
 
extern ClRcT clCorMoIdConcatenate(CL_INOUT ClCorMOIdPtrT part1, CL_INOUT ClCorMOIdPtrT part2, CL_IN ClInt32T copyWhere);
extern ClRcT clCorMoIdPack(ClCorMOIdT *pMoId, ClCharT **ppDataBuffer, ClUint32T *pDataSize);
extern ClRcT clCorMoIdUnpack(ClCharT *pData, ClUint32T dataSize, ClCorMOIdT *pMoId);

/*  -- APIs to manipulate the Containment Attribute Path --*/

/**
 ************************************
 *  \brief Creates an attribute path.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath (out) Handle of the new attribute path.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NO_MEM On memory allocation failure. 
 *
 *  \par Description:
 *  This function is used as a constructor for \e ClCorAttrPath as it creates an attribute path. It initializes 
 *  the memory and returns an empty COR path. 
 *  \par
 *  By default, the value for both \e index field and the attribute ID is \e -1. The default depth for the attribute path is 
 *  20. This value is incremented dynamically whenever a new entry is added.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 * 
 */
 
extern ClRcT clCorAttrPathAlloc(CL_INOUT ClCorAttrPathPtrT *pAttrPath);


/**
 ************************************
 *  \brief Initializes the attribute path.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the attribute path.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \par Description:
 *  This function is used to initialize the COR attribute path. It resets the information of the path, if present, 
 *  and re-initializes it as an empty path.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */
 
extern ClRcT clCorAttrPathInitialize(CL_INOUT ClCorAttrPathPtrT pAttrPath);


/**
 ************************************
 *  \brief Deletes the COR attribute path handle.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of \e ClCorAttrPath.
 *
 *  \retval CL_OK The function executed successfully.
 *
 *  \par Description:
 *  This function is used as a destructor for \e ClCorAttrPath as it deletes the handle of the COR attribute path. 
 *  It removes the handle and frees the memory associated with it.  
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */
 
extern ClRcT clCorAttrPathFree(CL_INOUT ClCorAttrPathPtrT  pAttrPath);


/**
 ************************************
 *  \brief Removes node after specified level.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the  \e clCorattrPathT.
 *  \param level Level to which \e ClCorAttrPathT needs to be truncated.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_INVALID_DEPTH If the level specified is invalid.
 *
 *  \par Description:
 *  This function is used to remove all the nodes and reset the MoId until the specified level is reached. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */
 
extern ClRcT clCorAttrPathTruncate(CL_INOUT ClCorAttrPathPtrT pAttrPath, CL_IN ClInt16T level);


/**
 ************************************
 *  \brief Sets the attribute ID for a given node.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the attribute path.
 *  \param level Level of the node.
 *  \param attrId Attribute ID to be set.
 *  \param index Index to be set.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_INVALID_DEPTH If the level specified is invalid.
 *
 *  \par Description:
 *  This function is used to set the attribute ID and the index at a specified node or level.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */

extern ClRcT clCorAttrPathSet(CL_INOUT ClCorAttrPathPtrT pAttrPath, CL_IN ClUint16T level, 
                                CL_IN ClCorAttrIdT attrId, CL_IN ClUint32T  index);


/**
 ************************************
 *  \brief Adds an entry to the attribute path.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 *  \param attrId ID of the attribute.
 *  \param index Index of the attribute.
 * 
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_MAX_DEPTH If depth exceeded the maximum limit.
 *
 *  \par Description:
 *  This function is used to add an entry to the attribute path. You must explicitly 
 *  specify the \e attrId and the \e index for the entry.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */

extern ClRcT clCorAttrPathAppend(CL_INOUT ClCorAttrPathPtrT pAttrPath, CL_IN ClCorClassTypeT attrId, 
                                    CL_IN ClCorInstanceIdT index);



/**
 ************************************
 *  \brief Returns the COR attribute path node depth.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 * 
 *  \par Return values:
 *  \e ClInt16T, the number of elements.
 *
 *  \par Description:
 *  This function is used to return the number of nodes in the hierarchy 
 *  within the COR attribute path. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */

extern ClInt16T clCorAttrPathDepthGet(CL_IN ClCorAttrPathPtrT pAttrPath);


/**
 ************************************
 *  \brief Displays the COR attribute path in debug mode only.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 * 
 *  \par Return values:
 *  
 *
 *  \par Description:
 *  This function is used to display all the entries within the COR attribute path.
 * 
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 *
 */

extern void clCorAttrPathShow(CL_IN ClCorAttrPathPtrT  pAttrPath);


/**
 ************************************
 *  \brief Returns the attribute ID.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 * 
 *  \retval ClCorAttrIdT the attribute ID.
 *
 *  \par Description:
 *  This function is used to return the attribute ID within the COR attribute path. It refers 
 *  to the attribute ID at the bottom of the hierarchy.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone(),
 *
 */

extern ClCorAttrIdT clCorAttrPathToAttrIdGet(CL_IN ClCorAttrPathPtrT pAttrPath);


/**
 ************************************
 *  \brief Returns the index of COR attribute path.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 * 
 *  \par Return values:
 *  \e ClUint32T index of last entry in attribute path.
 *
 *  \par Description:
 *  This function is used to retrieve the the index of COR attribute path. This instance refers 
 *  to the attribute ID and its index at the bottom of the hierarchy.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone()
 * 
 */

extern ClUint32T clCorAttrPathIndexGet(CL_IN ClCorAttrPathPtrT pAttrPath);


/**
 ************************************
 *  \brief Sets the index of COR attribute path.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 *  \param ndepth The depth at which new index is to be set.
 *  \param newIndex The new index to be set.
 * 
 *  \retval CL_OK on success.
 *
 *  \par Description:
 *  This function is used to set the index field within the COR attribute hierarchy.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathCompare(),
 *  clCorAttrPathClone(),
 * 
 */

extern ClRcT clCorAttrPathIndexSet(CL_INOUT ClCorAttrPathPtrT pAttrPath, CL_IN ClUint16T ndepth, CL_IN ClUint32T newIndex);


/**
 ************************************
 *  \brief Compares two \e ClCorAttrPath.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath First handle of \e ClCorAttrPath.
 *  \param cmp Second handle of \e ClCorAttrPath.
 *
 *  \retval 0 If both attribute paths are same, even if they have same indices, like wildcards.
 *  \retval -1 If the attribute paths mismatch.
 *  \retval 1 If wildcards are same.
 *
 *  \par Description:
 *  This function is used to make the comparison between two \e ClCorAttrPath and verify
 *  whether they are same. This comparison goes through till depth of both the paths. 
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathClone()
 *
 */

extern ClInt32T clCorAttrPathCompare(CL_IN ClCorAttrPathPtrT pAttrPath, CL_IN ClCorAttrPathPtrT cmp);


/**
 ************************************
 *  \brief Clones a particular \e ClCorAttrPath.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pAttrPath Handle of the COR attribute path.
 *  \param newH (out) Handle of the new clone.
 *
 *  \retval CL_OK The function executed successfully.
 *  \retval CL_COR_ERR_NO_MEM On memory allocation failure.
 *
 *  \par Description:
 *  This function is used to clone a particular COR attribute path. It allocates and makes a copy 
 *  of the contents of the given path to a new path.
 * 
 *  \par Library File:
 *   ClCorClient
 *
 *  \sa
 *  clCorAttrPathAlloc(),
 *  clCorAttrPathInitialize(),
 *  clCorAttrPathFree(),
 *  clCorAttrPathTruncate(),
 *  clCorAttrPathSet(),
 *  clCorAttrPathAppend(),
 *  clCorAttrPathDepthGet(),
 *  clCorAttrPathShow(),
 *  clCorAttrPathToAttrIdGet(),
 *  clCorAttrPathIndexGet(),
 *  clCorAttrPathIndexSet(),
 *  clCorAttrPathCompare()
 *
 */

extern ClRcT clCorAttrPathClone(ClCorAttrPathPtrT pAttrPath, ClCorAttrPathPtrT *newH);

/**
 ***********************************************
 *  \brief Creates MO and MSO objects.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoId (in) MOId of the MO to be created.
 *  \param pHandle (out) Handle of the MO object created.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_COR_INST_ERR_MO_ALREADY_PRESENT MO exists in the instance tree.
 *  \retval CL_COR_ERR_NULL_PTR pmoId is a NULL pointer.
 *  \retval CL_COR_ERR_NO_MEM Memory allocation failure.
 *  \retval CL_COR_INST_ERR_INVALID_MOID pMoId is invalid.
 *  \retval CL_COR_ERR_CLASS_NOT_PRESENT The specified class is not present.
 *  \retval CL_COR_INST_ERR_NODE_NOT_FOUND Parent class is not present in the instance
 *  tree.
 *  \retval CL_COR_MO_TREE_ERR_NODE_NOT_FOUND moTree node not found for the class.
 *  \retval CL_COR_INST_ERR_MAX_INSTANCE Maximum Instance count for this class is
 *  reached.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *  
 *  \par Description:
 *  This API creates the MO with its associated MSO objects and returns a handle to it. The
 *  service ID of pMoId must be set to CL_COR_INVALID_SVC_ID.
 *
 *  \par Library File:
 *  ClCorClient
 * 
 *  \sa
 *  clCorUtilMoAndMSODelete.
 *
 */
extern ClRcT clCorUtilMoAndMSOCreate(CL_IN ClCorMOIdPtrT pMoId, CL_OUT ClCorObjectHandleT *pHandle);

/**
 ***********************************************
 *  \brief Deletes MO and MSO objects.
 *
 *  \par Header File:
 *  clCorUtilityApi.h
 *
 *  \param pMoid (in) MOID of the MO to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_EXIST MO class type does not exist.
 *  \retval CL_COR_ERR_INVALID_PARAM pMoId of MSO passed instead of MO.
 *  \retval CL_COR_ERR_NULL_PTR pMoId is a NULL pointer.
 *  \retval CL_COR_INST_ERR_INVALID_MOID MoId is invalid.
 *  \retval CL_COR_INST_ERR_CHILD_MO_EXIST A child MO exists for the MO object node.
 *  \retval CL_COR_INST_ERR_NODE_NOT_FOUND Node not found in the object tree.
 *  \retval CL_COR_ERR_VERSION_UNSUPPORTED Version is not supported.
 *  \retval CL_COR_MO_TREE_ERR_NODE_NOT_FOUND MO tree node not found.
 *
 *  \par Description:
 *  This API is used to delete an MO and its associated MSO objects. The service ID of MOID
 *  must be set to CL_COR_INVALID_SVC_ID and passed to this API.
 *
 *  \par Library File:
 *  ClCorClient
 *
 *  \sa
 *  clCorUtilMoAndMSOCreate.
 *
 */
extern ClRcT clCorUtilMoAndMSODelete(CL_IN ClCorMOIdPtrT pMoId);


extern ClRcT clCorClientDebugCliRegister(ClHandleT *pDbgHandle);

extern ClRcT clCorClientDebugCliDeregister(ClHandleT dbgHandle);

extern ClRcT clCorClassAttrListGet(ClCharT* className,
                                ClCorAttrFlagT attrFlags,
                                ClUint32T* pAttrCount,
                                ClCorAttrDefT** pAttrDefList);


/*
 * MIB Table related APIs.
 */
extern ClRcT clCorMibTableAttrListGet(ClCharT* mibTable,
                                ClCorAttrFlagT attrFlags,
                                ClUint32T* pAttrCount,
                                ClCorAttrDefT** pAttrDefList);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_UTILITY_API_H_ */



/** \} */
