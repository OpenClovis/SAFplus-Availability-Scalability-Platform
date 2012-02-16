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
 * ModuleName  : alarm
 * File        : clAlarmUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module contains Alarm Service utility related APIs
 *
 *****************************************************************************/

#ifndef _CL_ALARM_UTIL_H
#define _CL_ALARM_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif
   
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clCorMetaData.h>


/**
 *  \file 
 *  \brief Header File containing functions and structures used for alarm payload for traps.
 *  \ingroup alarm_apis
 */

/**
 *  \addtogroup alarm_apis
 *  \{
 */

/**
 * The structure is used to store the information about the variables
 * to be used in the trap. This takes the type of the attribute
 * which is one of the value from the enum \e ClCorTypeT, the length
 * of the atttribute and the value of the variable. The element 
 * \e value is a pointer which should be allocated of size equal
 * to value of the element \e length.
 */ 
typedef struct ClAlarmUtilTlv{
	/**
	 * Data type of the attribute. This type is one 
	 * of the entry of the enum ClCorTypeT.
	 */
    ClCorTypeT	type;
	/**
	 * Number of bytes the attribute would hold. For
	 * simple attribute it is the size of the data type.
	 * For an array attribute, it is the number of 
	 * elements multiplied by size of basic type.
	 */
    ClUint32T   length;
	/**
	 * Value of the attribute. This should be
	 * allocated of the size equal to the value of
	 * "length".
	 */
    ClPtrT      value;
}ClAlarmUtilTlvT;

typedef struct ClAlarmUtilTlv *ClAlarmUtilTlvPtrT;


/**
 * This structure holds the information about the attributes that needs
 * to be provided in the alarm payload. This payload is then used by 
 * the \e SNMP \e subagent for reporting the trap after receiving the
 * event published by alarm server.
 * The structure is used to store the number of attributes and pointer 
 * to the array of structure containing information about attributes 
 * that needs to be part of the trap information. The pointer to the 
 * array of the structure \e ClAlarmUtilTlvT has to be allocated which 
 * has the number of elements equal to the value of \e numTlvs element. 
 */
typedef struct ClAlarmUtilTlvInfo{
	/**
	 * Number of attribute information
	 * needed for the trap generation on resource.
	 */
    ClUint32T		numTlvs;
	/**
	 * Pointer to the structure containing attribute information.
	 * This has to be allocated based on the number of attributes
	 * which is the value of \e numTlvs.
	 */
    ClAlarmUtilTlvPtrT	pTlv;
}ClAlarmUtilTlvInfoT;

typedef struct ClAlarmUtilTlvInfo *ClAlarmUtilTlvInfoPtrT;
    
/**
 * This structure holds the information about the attributes that need
 * to be provided in the alarm payload. This payload is then used by 
 * the SNMP subagent for reporting the trap.
 * This is a structure which contains the information about the 
 * traps. This structure is similar to the \e ClAlarmUtilTlvInfoT but
 * this has the MOId of the resource as an extra field. The SNMP
 * subagent extracts the payload information using this structure
 * So a user planning to report an alarm as trap he/she should 
 * provide the attribute information in this structure only.
 */
typedef struct ClAlarmUtilPayLoad{
	/**
	 * The pointer to the \e MOID of the resource. It is the
	 * identifier of the resource in the COR object tree. This 
	 * needs to be allocated before populating it.
	 */
    ClCorMOIdPtrT	pMoId;
	/**
	 * Number of attribute information needed for the trap 
	 * generation on resource.
	 */
    ClUint32T		numTlvs;
	/**
	 * Pointer to the structure containing attribute information.
	 * This has to be allocated based on the number of attributes
	 * which is the value of \e numTlvs.
	 */
    ClAlarmUtilTlvPtrT	pTlv;
}ClAlarmUtilPayLoadT;

typedef struct ClAlarmUtilPayLoad *ClAlarmUtilPayLoadPtrT;

/**
 * The stucture is used to specify the alarm payload information. The number of
 * resources for which the payload information should be given is specified
 * as the value of \e numPayLoadEnteries. The different entries corresponding
 * to different resources should be specified in the \e pPayload array after
 * allocating it. For each resource there can be multiple attributes whose 
 * information should be specified in the array of \e pTlv element of the
 * \e ClAlarmUtilTlvPtrT.
 */
typedef struct ClAlarmUtilPayLoadList
{
	/**
	 * The number of entries which need to be
	 * specified in the alarm payload.
	 */
    ClUint32T		    numPayLoadEnteries;
	/**
	 * Stores the information about the attributes
	 * corressponding to a particular resource. It
	 * has be allocated based on the value of 
	 * \e numPayLoadEnteries. So each element of the
	 * \e pPayload array has to be filled with appropriate
	 * attribute details.
	 */
    ClAlarmUtilPayLoadPtrT  pPayload;
}ClAlarmUtilPayLoadListT;

typedef struct ClAlarmUtilPayLoadList *ClAlarmUtilPayLoadListPtrT;

/**
 ************************************************
 *  \brief Function to convert the payload information into a flat buffer.
 *
 *  \par Header File:
 *  clAlarmUtils.h
 *
 *  \param pPayLoadList (in) : The structure containing the payload information.
 *  \param pSize       (out) : The size of payload information. The value of this
 *  						   field has to be provided to the \e len element of the 
 *  						   \e ClAlarmInfoT structure.
 *  \param ppBuf       (out) : The pointer is allocated by the function after flattening
 *                             the alarm payload information filled in the \e pPayLoadList.
 *                             This pointer has to be copied to the \e buff element of the
 *                             \e ClAlarmInfoT structure.
 *
 *  \retval CL_ERR_NULL_POINTER : The pointer passed to \e pPayLoadList, \e pSize or \e ppBuf is NULL.
 *  \retval CL_OK 				: The function completed successfully.
 *
 *  \par Description:
 *  This function converts the alarm information into a flat buffer which can be used to provide
 *  as alarm payload while reporting the alarm. The function packs the alarm payload details
 *  into network byte order and then it puts the information into a flat buffer after allocating
 *  it. The size of the buffer is updated into \e pSize and the buffer is allocated and updated 
 *  in the \e ppBuff. The size of the buffer and the buffer could be used to supply it to the 
 *  element \e len and \e buff of the ClAlarmInfoT structure while reporting the alarm.
 *  The buffer allocated here has to be freed after raising the alarm, using \e clAlarmPayloadBufFree().
 *  Also the \e pPayLoadList which contains the alarm payload information should be freed using the
 *  function \e clAlarmUtilPayloadListFree().
 *
 *  \par Library File:
 *  ClAlarmUtils
 *
 *  \sa clAlarmUtilPayloadBufFree(), clAlarmUtilPayloadListFree()
 *
 */

ClRcT clAlarmUtilPayloadFlatten(ClAlarmUtilPayLoadListPtrT pPayLoadList, ClUint32T *pSize, ClUint8T  **ppBuf);

/**
 ************************************************
 *  \brief  The function to extract the payload information sent at the time of alarm raise.
 *
 *  \par Header File:
 *  clAlarmUtils.h
 *
 *  \param pBuf  		 (in) 	: The pointer containing the payload information. This is the pointer
 *  							  of the payload (buff) obtained from the ClAlarmHandleInfoT of the 
 *  							  alarm event delivery callback function.
 *  \param size  	     (in) 	: Size of the payload. This is also obtained from the \e len element 
 *  							  of the \e alarmInfo element of the \e ClAlarmHandleInfoT structure 
 *  							  provided in the event delivery callback function.
 *  \param ppPayloadList (out) 	: This is the pointer to the \e ClAlarmUtilPayLoadListT structure 
 *  					 		  which is populated by the function with the payload information 
 *  					 		  sent by the application reported alarm.
 *                                
 *  \retval CL_ERR_NULL_POINTER : The pointer passed to pBuf or ppPayLoadListare NULL.
 *  \retval CL_ERR_NO_MEMORY    : Failed while allocating the memory for the payload information.
 *  \retval CL_OK 				: The function completed successfully.
 *
 *  \par Description:
 *  This function unpacks the the payload buffer sent by the alarm reporter into host format. 
 *  This buffer contains the payload information packed in certain format. This was done by 
 *  the alarm reported while raising the alarm. The alarm listener or the SNMP subagent does
 *  the job of unpacking the buffer and recreating the payload information. This is done by 
 *  calling this function. 
 *  This function allocates the payload buffer in the \e ppPayloadList argument. This needs to be
 *  freed and this can be done by calling \e clAlarmUtilPayloadListFree().
 *
 *  \par Library File:
 *  ClAlarmUtils
 *
 *  \sa clAlarmUtilPayloadListFree()
 *
 */
ClRcT clAlarmUtilPayLoadExtract(ClUint8T  *pBuf, ClUint32T size, ClAlarmUtilPayLoadListT **ppPayloadList);

/**
 ************************************************
 *  \brief  The function to free the buffer obtained after flattening the payload information.
 *
 *  \par Header File:
 *  clAlarmUtils.h
 *
 *  \param pBuf  (in) 	: The pointer containing the payload information.  
 *
 *  \retval    	 None
 *
 *  \par Description:
 *	The flat buffer allocated and updated on the successfull execution of the 
 *	clAlarmUtilPayloadFlatten() should be freed. This is achieved by calling
 *	this function.
 *
 *  \par Library File:
 *  ClAlarmUtils
 *
 *  \sa clAlarmUtilPayloadFlatten()
 *
 */
void clAlarmUtilPayloadBufFree(ClUint8T  *pBuf);

/**
 ************************************************
 *  \brief Function used to free the payload data-structure allocated during 
 *         alarm reporting or receive.
 *
 *  \par Header File:
 *  clAlarmUtils.h
 *
 *  \param pPayloadList (in)	: This is the pointer to the \e ClAlarmUtilPayLoadListT structure 
 *  					 		  which is allocated after calling \e clAlarmUtilPayLoadExtract().
 *                                
 *  \retval 		None
 *
 *  \par Description:
 *  The pointer \e pPayloadList has the payload information supplied by the alarm 
 *  raiser as well after extracting the payload in the alarm event receive function.
 *  This payload information is allocated using different structures, which has to 
 *  be freed. This can be done by calling this function.
 *
 *  \par Library File:
 *  ClAlarmUtils
 *
 *  \sa clAlarmUtilPayLoadExtract()
 *
 */
void clAlarmUtilPayloadListFree(ClAlarmUtilPayLoadListT   *pPayloadList);


ClRcT 
clAlarmClientDebugRegister (CL_OUT ClHandleT * pAlarmDebugHandle);

ClRcT 
clAlarmClientDebugDeregister (CL_IN ClHandleT alarmDebugHandle);

#ifdef __cplusplus
}
#endif

#endif /*_CL_ALARM_UTIL_H */

/** \} */
