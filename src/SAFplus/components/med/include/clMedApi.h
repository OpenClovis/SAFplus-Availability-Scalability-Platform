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
 * Description :
 *
 * This module contains mediation manager APIs.
 *
 *****************************************************************************/

/**
 *  \defgroup group27 Mediation Manager
 *  \ingroup group5
 */

/**
 *  \file
 *  \ingroup group27
 */

/**
 * \name Mediation Manager
 */

/**
 *  \addtogroup group27
 *  \{
 */




#ifndef  _CL_MED_API_H_
#define _CL_MED_API_H_

#ifdef __cplusplus
extern "C" {
#endif



/* INCLUDES */
#include "clCommon.h"
#include "clCntApi.h"
#include "clEoApi.h"
#include <clAlarmDefinitions.h>
#include<clCorApi.h>
#include<clCorUtilityApi.h>
#include<clCorNotifyApi.h>
#include<clCorTxnApi.h>
#include<clCorErrors.h>
#include "clMedErrors.h"


/**
 * \page pageMediation Mediation Manager
 *
 * \par Overview
 * Customer systems can be managed by different external managers like
 * SNMP, CLI, CMIP, XML, WEB, and so on. These managers communicate to their corresponding
 * agents in the system using the specific protocol language (like OID in
 * SNMP agent). The management agents in a system have to translate the
 * attributes and/or commands into OpenClovis run time environment attributes
 * and operations. \n
<BR>
 * The OpenClovis Mediation component acts like a gateway into the OpenClovis
 * runtime  environment on the System Controller. On one side, it interacts
 * with management agent like, SNMP agent, CLI agent, and on the other side
 * it interacts with OpenClovis components or managers. The primary responsibility
 * of the Mediation module is to translate the managed service requests
 * from the management station into requests in the OpenClovis runtime
 * environment. \n
<BR>
 * Mediation Manager runs as a library in the context of a management
 * agent. It maintains the translation between a specific protocol identifier
 * and OpenClovis run time environment identifier(s). It also maintains the
 * translation between a protocol operation and corresponding COR operation. \n
<BR>
 * This component also allows the management agents to register
 * for asynchronous notifications within the system.
 *
 * \par Usage Scenario(s)
 *
 * The following section explains how an SNMP agent can use Mediation
 * component:
 *
 * When SNMP Agent is created it has to initialize the mediation component
 * \code
 *
 *         ClMedHdlPtrT    gMedHdl; // This is a handle returned by the mediation layer
 *
 *         sanmpAgentInit()
 *         {
 *              ......
 *              ......
 *              if (CL_OK != clMedInitialize(&gMedHdl, snmpTrapHandler, snmpInstXlator, snmpInstCompare))
 *              {
 *                 return ERROR;
 *              }
 *              ......
 *        }
 *
 *
 *
 *       SNMP Agent can tell the mediation manager about all its operations and
 *   corresponding COR operation Identifiers as follows. This can be done by
 *   some other entity which understands the semantics. It is also possible to
 *   have one SNMP operation mapping to multiple of COR operations.
 *
 *        buildOpTable()
 *        {
 *           ClMedTgtOpCodeT     corOpCode;
 *           ClMedAgntOpCodeT    agntOpCode;
 *           ClMedCorOpEnumT    corOp = CL_COR_GET;
 *           ClSnmpOpCodeT snmpOpCode = CL_SNMP_GET;
 *
 *           corOpCode.type       = CL_MED_XLN_TYPE_COR;
 *
 *           for (snmpOpCode = CL_SNMP_GET; snmpOpCode < CL_SNMP_MAX; snmpOpCode++, corOp++)
 *           {
 *               corOpCode.opCode        = corOp;
 *               agntOpCode.opCode       = snmpOpCode;
 *               if (CL_OK != clMedOperationCodeTranslationAdd(gMedHdl,
 *                                                   agntOpCode,
 *                                                   &corOpCode,
 *                                                   1))
 *              {
 *                  return ERROR;
 *              }
 *           }
 *        }
 *
 *       SNMP Agent can also delete an operation translation if it
 *   wishes so(may be a rare case).
 *
 *         if (CL_OK != clMedOperationCodeTranslationDelete(gMedHdl,
 *                                             CL_SNMP_SET)
 *
 *        {
 *             return ERROR;
 *        }
 *
 *         SNMP agent can add an OID translation with "clMedIdentifierTranslationInsert"
 *   In the following example interface MTU (ifMtu) is being added to the
 *   translation table.
 *
 *         ifMtu in ASN notation is represented as 1.3.6.1.2.1.2.2.1.4
 *    It is equal to
 *         {iso(1) identified-organization(3) dod(6) internet(1)
 *          mgmt(2) mib-2(1) interface(2) ifTable(2) ifEntry(1) ifMtu(4)}
 *
 *    So if ifMtu of 111th interface has to be added to the translation table
 *    it can be done as follows:
 *
 *
 *      {
 *           ClMedAgentIdT   snmpIfMtu;
 *           char          tmpString[] = "1.3.6.1.2.1.2.2.1.4.111"
 *           ClMedTgtIdT    coIfMtu[1];
 *
 *           strcpy(snmpIfMtu.id, tmpString);
 *           snmpIfMtu.len = strlen(tmpString) + 1;
 *
 *           corIfMtu.info.corId.moId   = myNode;
 *           corIfMtu.info.corId.attrId =  IF_MTU;
 *
 *           if (CL_OK != clMedIdentifierTranslationInsert(gMedHdl,
 *                                            snmpIfMtu,
 *                                            corIfMtu,
 *                                            1))
 *           {
 *              return ERROR;
 *           }
 *      }
 *
 *    When an interface is destroyed SNMP agent can delete the entry as follows:
 *
 *       {
 *           ClMedAgentIdT   snmpIfMtu;
 *           char          tmpString[] = "1.3.6.1.2.1.2.2.1.4.111"
 *
 *           strcpy(snmpIfMtu.id, tmpString);
 *           snmpIfMtu.len = strlen(tmpString) + 1;
 *           if (CL_OK != clMedIdentifierTranslationDelete(gMedHdl, snmpIfMtu))
 *           {
 *              return ERROR;
 *           }
 *       }
 *
 *          When SNMP agent receives a request for GET on ifMtu it can call the
 *    mediation component API to execute the command. The code below explains
 *    how to do it:
 *
 *         {
 *              ClMedOpT        opInfo;
 *              ClMedVarBindT   varBind;
 *              ClInt32T retVal = 0;
 *              ClSnmpTableIdxT *pInst = NULL;
 *              char tmpString[] = "1.3.6.1.2.1.2.2.1.4.111";
 *
 *              opInfo.varCount = 1;
 *              opInfo.varInfo  = &varBind;
 *              opInfo.varInfo[0].pVal = &retVal;
 *              opInfo.varInfo[0].errId = 0;
 *              opInfo.varInfo[0].pInst = pInst; //Fill the instance value in pInst before this assignment
 *
 *              opInfo.opCode = CL_SNMP_GET;
 *              opInfo.varInfo[0].attrId.id = (ClUint8T*)tmpString;
 *              opInfo.varInfo[0].attrId.len = (ClUint32T)strlen(tmpString) + 1;
 *              opInfo.varInfo[0].len = sizeof (CL_COR_UINT32);
 *              varBind.type   = COR_CHAR;
 *
 *              if (CL_OK != clMedOperationExecute(gMedHdl,
 *                                                  &opInfo))
 *             {
 *               return ERROR;
 *             }
 *         }
 *
 *        When the agent/some one on behalf of the agent wants to subscribe
 *   for asynchronous notifications/alarms it can be done
 *   using "clMedNotificationSubscribe" API
 *        The following code block illustrates how it can be done:
 *         {
 *              char  buf[100];
 *
 *              ClMedAgentIdT   snmpIfMtu;
 *              ClMedVarBindT  varBind;
 *              ClMedVarBindT       myOp;
 *              char          tmpString[] = "1.3.6.1.2.1.2.2.1.4.111"
 *
 *              strcpy(snmpIfMtu.id, tmpString);
 *              snmpIfMtu.len = strlen(tmpString) + 1;
 *              if (CL_OK != clMedNotificationSubscribe(gMedHdl,   // Handle
 *                                            snmpIfMtu, // Attribute
 *                                            0))// SessioId(can ignore)
 *              {
 *                  return ERROR;
 *              }
 *         }
 *  \endcode
 *        Similarly the applications can unsubscribe for any notifications
 *   using "clMedNotificationUnsubscribe" API. SNMP agent can unsubscribe to all the
 *   notifications using a single API "clMedAllNotificationUnsubscribe".
 *
 *        When SNMP agent is being destroyed (probably as part of system
 *   shut down) it can destroy the mediation component by calling
 *        clMedFinalize(gMedHdl);
 *
 *
 *
 */

/**
 *  This constant is meant to use for defining attribute path while initializing ClMedTgtIdT.
 */
#define CL_MED_ATTR_VALUE_END    -1

/**
 * Mediation object Handle.
 *
 * The type of an identifier for the mediation module.
 *
 */
typedef void* ClMedHdlPtrT;

/**
 * The structure ClMedAgentId contains the client representation of the attribute(s).
 *
 */
struct ClMedAgentId
{

/**
 * Identifier in agent language.
 * SNMP OID kind of method.
 */
    ClUint8T      *id;

/**
 *  Length of the identifier.
 */
    ClUint32T      len;
};


/**
 * The type of identifier for the Trap/Change Notification handler.
 *
 */
typedef ClRcT ( *ClMedNotifyHandlerCallbackT)(
/**
 * Handle to ClCorMOId.
 */
     ClCorMOIdPtrT,

/**
 * Alarm Probable Cause.
 */
     ClUint32T ,

/**
 * Alarm Specific Problem.
 */
     ClAlarmSpecificProblemT,

/**
 * Optional data.
 */
     ClCharT *,

/**
 *  Data length.
 */
    ClUint32T);

/**
 * Enumeration for operations handled by instance translation.
 */
typedef enum 
{
/**
 * Creation by Object Implementer.
 */ 
    CL_MED_CREATION_BY_OI = 0,

/**
 * Creation by Northbound ( for example: SNMP agent.)
 */ 
    CL_MED_CREATION_BY_NORTHBOUND,

} ClMedInstXlationOpEnumT;

/**
 *  Instance Translator for COR managed attributes. (Optional)
 */
typedef ClRcT ( *ClMedInstXlatorCallbackT) (

/**
 *  Agent Id.
 */
const struct   ClMedAgentId*,

/**
 *  Pointer to ClCorMOId to be modified.
 */
   ClCorMOIdPtrT      hmoId,

/**
 *  Pointer to containment path.
 */
   ClCorAttrPathPtrT containedPath,

/**
 * Instance in agent form.
 */
   void**  pInstance,

/**
 * Instance length.
 */
   ClUint32T  *instLen,

/**
 * List of COR Attributes to be filled
 * when instance is being created by agent.
 */
   ClPtrT    pCorAttrValueDescList,

/**
 * Operation to perform.
 */
  ClMedInstXlationOpEnumT  instXlnOp,

/**
 * Cookie.
 */
  ClPtrT cookie);



/**
 * Enum to represent COR operations in numerics.
 *
 */
typedef enum
{

/**
 * SET command.
 */
   CL_COR_SET =1,

/**
 *  GET command.
 */
    CL_COR_GET,

/**
 * Get Next.
 */
    CL_COR_GET_NEXT,

/**
 * Create.
 */
    CL_COR_CREATE,

/**
 * Delete.
 */
    CL_COR_DELETE,

/**
 * Get ALL.
 */
    CL_COR_WALK,

/**
 * Get First.
 */
    CL_COR_GET_FIRST,

/**
 * Get Services.
 */
    CL_COR_GET_SVCS,

/**
 * Set containment attribute.
 */
    CL_COR_CONTAINMENT_SET,

/**
 * Invalid.
 */
    CL_COR_MAX
} ClMedCorOpEnumT;

/**
 * Enum to represent target identifier type.
 *
 */
typedef enum
{

/**
 *  Identifier is COR attribute.
 */
   CL_MED_XLN_TYPE_COR=1,

/**
 *  Identifier is not COR attribute.
 */
   CL_MED_XLN_TYPE_NON_COR

} ClMedTgtTypeEnumT;

/**
 * Enum to represent bitmap of available services at MO.
 *
 */
typedef enum
{
/**
 * OPENCLOVIS FAULT MANAGER.
 */
   CL_MED_SVC_ID_FM = 0,

/**
 * OPENCLOVIS ALARM AGENT.
 */
   CL_MED_SVC_ID_ALARM,

/**
 * OPENCLOVIS PROVISION MANAGER.
 */
   CL_MED_SVC_ID_PROVISIONING,

/**
 *  OPENCLOVIS DUMMY MSP.
 */
    CL_MED_SVC_ID_DUMMY1,

/**
 *  OPENCLOVIS PERFORMANCE AGENT.
 */
    CL_MED_SVC_ID_PERF,

/**
 *  OPENCLOVIS DUMMY MSP.
 */
    CL_MED_SVC_ID_DUMMY2,

/**
 *  OPENCLOVIS CHASSIS MANAGER.
 */
    CL_MED_SVC_ID_CHASSIS,

/**
 * OPENCLOVIS BOOT MANAGER.
 */
    CL_MED_SVC_ID_BOOT,

/**
 *  OPENCLOVIS REDUNDANCY MANAGER.
 */
    CL_MED_SVC_ID_RMS,

/**
 * OPENCLOVIS SECURITY MANAGER.
 */
    CL_MED_SVC_ID_SECURITY,

/**
 *  OPENCLOVIS DUMMY MSP.
 */
    CL_MED_SVC_ID_DUMMY,

/**
 * OPENCLOVIS EO MANAGER MSP.
 */
    CL_MED_SVC_ID_EO,

/**
 * OPENCLOVIS DUMMY MSP.
 */
    CL_MED_SVC_ID_DUMMY3,

    CL_MED_MAX_SERVICES
} ClMedSvcIdEnumT;



typedef struct ClMedAgentId ClMedAgentIdT;


/**
 * Structure representing the parameters.
 *
 */
typedef struct
{

/**
 * Attribute ID in client language
 */
     ClMedAgentIdT    attrId;

/**
 *  Actual Value.
 */
void           *pVal;

/**
 * Length of the value.
 */
     ClUint32T     len;

/**
 * Result buffer.
 */
void           *outBuf;

/**
 * Length of the output buffer.
 */
     ClUint32T     outBufLen;

/**
 *   Data related to instance.
 */
void           *pInst;

/**
 *  Instance Data Length.
 */
     ClUint32T     instLen;

/**
 *  Error Id return value.
 */
     ClUint32T     errId;

/**
 * Private information to be stored in the agent.
 */
void           *cookie;

/**
 * Index for array/containment/associated attributes. Set -1 if not an array.
 */
    ClUint32T index;

/**
 * Attribute Id of containment or association attribute.
 */
    ClCorAttrIdT    ContAttrId;

}ClMedVarBindT;

/**
 * Structure to represent Operation information.
 */
typedef struct
{

/**
 *  Operation Code: Language - Client.
 */
  ClUint32T       opCode;

/**
 *  Number of variables for this operation.
 */
    ClUint32T       varCount;

/**
 *  Pointer to an array of operands.
 */
    ClMedVarBindT     *varInfo;

}ClMedOpT;


/**
 * Structure to represent Cor Identifier.
 */
typedef struct
{

/** ClCorMOId*/
    ClCorMOIdPtrT          moId;

/**
 * Attribute Id.
 */
    ClCorAttrIdT*     attrId;
}ClMedCorIdT;

/**
 * The structure ClMedAgntOpCodeT represents the operation code on the management side .
 */

typedef  struct
{
/**
 *  OpCode in native form.
 */
    ClUint32T     opCode;
}ClMedAgntOpCodeT;


/**
 * The structure ClMedTgtOpCodeT represents the target operation code.
 */
typedef struct
{
/**
 * Type. COR/NON_COR.
 */
    ClMedTgtTypeEnumT      type;

/**
 *   OpCode in native form.
 */
    ClMedCorOpEnumT     opCode;
}ClMedTgtOpCodeT;


/**
 * The structure ClMedTgtIdT represents the target identifier.
 */
typedef struct
{
    ClMedTgtTypeEnumT  type;

/**
 * EOId.
 */
    ClEoIdT eoId;
    union
    {
/**
 *  Identifier in COR terms.
 */
        ClMedCorIdT  corId;

    }info;
}ClMedTgtIdT;

/**
 * The structure ClMedErrorIdT represents the error returned from COR 
 */
typedef struct
{
/**
 * Error id 
 */
  ClRcT errId; 
/**
 *  Oid info 
 */
   ClMedAgentIdT oidInfo;

}ClMedErrorIdT;

/**
 * The structure ClMedErrorListT represents list of error returned from COR 
 */
typedef struct
{
    ClUint32T   count;
/**
 *  List of errId with oid  
 */
    ClMedErrorIdT  *pErrorList;

}ClMedErrorListT;
/***************************************************************************/


/**
 ************************************
 *  \page pagemed101 clMedInitialize
 *
 *  \par Synopsis:
 *  Initializes the mediation library.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedInitialize(
 *                             CL_OUT ClMedHdlPtrT       *medHdl,
 *                             CL_IN ClMedNotifyHandlerCallbackT  fpTrapHdlr,
 *                             CL_IN ClMedInstXlatorCallbackT  instXlator,
 *                             CL_IN ClCntKeyCompareCallbackT fpInstCompare);
 *  \endcode
 *
 *  \param medHdl:  (out) Handle to mediation.
 *  \param fpTrapHdlr: Function pointer to handle the notifications.
 *  \param instXlator: Function pointer to handle the instance translations.
 *  \param fpInstCompare: Function pointer to compare two instances of objects.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to initialize the mediation library. We need to supply
 *  the callbacks for notification handling, instance translation, and key comparison.
 *  As a part of the initialization, this API will return the mediation handle.
 *
 *  \par Library File:
 *  libClMedClient.a, libClMedClient.so
 *
 *  \note
 *  This API is invoked before performing any operation of the mediation library.
 *
 *  \par Related Function(s):
 *  \ref pagemed102 "clMedFinalize"
 *
 */


 ClRcT  clMedInitialize(CL_OUT ClMedHdlPtrT       *medHdl,
                       CL_IN ClMedNotifyHandlerCallbackT  fpTrapHdlr,
                       CL_IN ClMedInstXlatorCallbackT  instXlator,
                       CL_IN ClCntKeyCompareCallbackT fpInstCompare);




/**
 ************************************
 *  \page pagemed102 clMedFinalize
 *
 *  \par Synopsis:
 *  Destroys the mediation library.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedFinalize(
 *                          CL_IN ClMedHdlPtrT medHdl);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to destroy the mediation library. This must be supplied
 *  with a valid mediation handle which is obtained at the time of initialization of the mediation library. This
 *  is called when there is no further usage of the Mediation Library.
 *
 *  \par Library File:
 *  libClMedClient.a, libClMedClient.so
 *
 *  \note
 *  This API is invoked as a part of the mediation clean up.
 *
 *  \par Related Function(s):
 *  \ref pagemed101 "clMedInitialize"
 *
 */

 ClRcT  clMedFinalize(CL_IN ClMedHdlPtrT medHdl);


/**
 ************************************
 *  \page pagemed103 clMedOperationExecute
 *
 *  \par Synopsis:
 *  Executes a COR operation.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT  clMedOperationExecute(
 *                                  CL_IN ClMedHdlPtrT     medHdl,
 *                                  CL_INOUT ClMedOpT      *opInfo);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param opInfo: (in/out) Pointer to an array of type ClMedOpT.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *  \retval CL_MED_ERR_NO_OPCODE: If passed \e opCode is not been added.
 *
 *  \par Description:
 *  This API is used to execute a COR operation. The Mediation library, which is an
 *  interface to the COR, supplies the operation type based on which the
 *  COR performs the operations for the agent.
 *
 *  \par Library File:
 *  libClMedClient.a
 *
 *  \note
 *  This API is invoked in order to execute a COR operation.
 *
 *  \par Related Function(s):
 *  None.
 *
 */
ClRcT  clMedOperationExecute(CL_IN ClMedHdlPtrT     medHdl,
                             CL_INOUT ClMedOpT      *opInfo);


/**
 ************************************
 *  \page pagemed104 clMedIdentifierTranslationInsert
 *
 *  \par Synopsis:
 *  Adds an entry to the identifier translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedIdentifierTranslationInsert(
 *                              CL_IN ClMedHdlPtrT      medHdl,
 *                              CL_IN ClMedAgentIdT   clientIdentifier,
 *                              CL_IN ClMedTgtIdT    *corIdentifier,
 *                              CL_IN ClUint32T    elemCount);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param clientIdentifier: Identifier in agent language.
 *  \param corIdentifier: Array of COR Identifiers.
 *  \param elemCount: Number of elements.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to add an entry to the identifier translation table. The entries are added
 *  based on the \e clientIdentifier value.
 *
 *  \par Library File:
 *  libClMedClient.a,  libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed105 "clMedIdentifierTranslationDelete"
 *
 */
ClRcT  clMedIdentifierTranslationInsert(CL_IN ClMedHdlPtrT      medHdl,
                               CL_IN ClMedAgentIdT   clientIdentifier,
                               CL_IN ClMedTgtIdT    *corIdentifier,
                               CL_IN ClUint32T    elemCount);

ClRcT  clMedInstanceTranslationInsert(CL_IN ClMedHdlPtrT      medHdl,
                                      CL_IN ClMedAgentIdT   clientIdentifier,
                                      CL_IN ClMedTgtIdT    *corIdentifier,
                                      CL_IN ClUint32T    elemCount);

/**
 ************************************
 *  \page pagemed105 clMedIdentifierTranslationDelete
 *
 *  \par Synopsis:
 *  Deletes an entry from the identifier translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedIdentifierTranslationDelete(
 *                              CL_IN ClMedHdlPtrT       medHdl,
 *                              CL_IN ClMedAgentIdT    clientIdentifier);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param clientIdentifier: Identifier in agent language.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to delete an entry from the identifier translation table. The
 *  the mediation manager adds the entry based on the client identifier, so given a valid mediation handle with
 *  a correct client identifier, the entry will be deleted.
 *
 *  \par Library File:
 *  libClMedClient.a,  libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed108 "clMedIdentifierTranslationInsert"
 */

ClRcT  clMedIdentifierTranslationDelete(CL_IN ClMedHdlPtrT       medHdl,
                               CL_IN ClMedAgentIdT    clientIdentifier);


/**
 ************************************
 *  \page pagemed106 clMedOperationCodeTranslationAdd
 *
 *  \par Synopsis:
 *  Adds an entry to the identifier translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedOperationCodeTranslationAdd(
 *                                  CL_IN ClMedHdlPtrT medHdl,
 *                                  CL_IN ClMedAgntOpCodeT agntOpCode,
 *                                  CL_IN ClMedTgtOpCodeT *tgtOpCode,
 *                                  CL_IN ClUint32T opCount);
 *  \endcode
 *
 *  \param medHdl:  Handle returned as part of initialization.
 *  \param agntOpCode: Operation identifier in agent terminology.
 *  \param tgtOpCode: Operation identifier in COR terminology.
 *  \param opCount: Operation count.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to add an entry to the Operation code translation table for the agent. This is
 *  similar to the mediation identifier translation table.
 *  The agent opCode must have a corresponding Cor opCode for adding it into the opCode
 *  translation table.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed107 "clMedOperationCodeTranslationDelete"
 *
 */
ClRcT  clMedOperationCodeTranslationAdd(CL_IN ClMedHdlPtrT          medHdl,
                                   CL_IN ClMedAgntOpCodeT   agntOpCode,
                                   CL_IN ClMedTgtOpCodeT    *tgtOpCode,
                                   CL_IN ClUint32T        opCount);

/**
 ************************************
 *  \page pagemed107 clMedOperationCodeTranslationDelete
 *
 *  \par Synopsis:
 *  Deletes an entry from the operation translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedOperationCodeTranslationDelete(
 *                                   CL_IN ClMedHdlPtrT         medHdl,
 *                                   CL_IN ClMedAgntOpCodeT  agntOpCode);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param agntOpCode: Operation identifier in agent terminology.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to delete an entry from the operation translation table.
 *  To delete an entry from the opCode table, a valid mediation handle and valid
 *  agent opCode are required.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed104 "clMedIdentifierTranslationInsert"
 *
 */

 ClRcT  clMedOperationCodeTranslationDelete(CL_IN ClMedHdlPtrT         medHdl,
                                   CL_IN ClMedAgntOpCodeT  agntOpCode);

/**
 ************************************
 *  \page pagemed108 clMedErrorIdentifierTranslationInsert
 *
 *  \par Synopsis:
 *  Adds an entry to the Error Id translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedErrorIdentifierTranslationInsert(
 *                                  CL_IN ClMedHdlPtrT    medHdl,
 *                                  CL_IN ClUint32T  sysErrorType,
 *                                  CL_IN ClUint32T  sysErrorId,
 *                                  CL_IN ClUint32T  agntErrorId);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param sysErrorType: System error type.
 *  \param sysErrorId: System error ID.
 *  \param agntErrorId: Agent Error Id.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to add an entry to the \e ErrorId translation table. The agent
 *  Error Id is supplied with corresponding system error ID and system error
 *  type. The API then adds the entry of \e sysErrorId and \e sysErrorType for the value
 *  of the \e agentErrorId.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed109 "clMedErrorIdentifierTranslationDelete"
 *
 */
 ClRcT  clMedErrorIdentifierTranslationInsert(CL_IN ClMedHdlPtrT    medHdl,
                                    CL_IN ClUint32T  sysErrorType,
                                    CL_IN ClUint32T  sysErrorId,
                                    CL_IN ClUint32T  agntErrorId);
/**
 ************************************
 *  \page pagemed109 clMedErrorIdentifierTranslationDelete
 *
 *  \par Synopsis:
 *   Deletes an entry from the Error Id translation table.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedErrorIdentifierTranslationDelete(
 *                                  CL_IN ClMedHdlPtrT    medHdl,
 *                                  CL_IN ClUint32T  sysErrorType,
 *                                  CL_IN ClUint32T  sysErrorId);
 *
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param sysErrorType: System error type.
 *  \param sysErrorId: System error ID.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to delete an entry from the Error Id translation table. So given
 *  a value of the agent \e errorId and mediation handle, the entry in the error identifier table is deleted.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed108 "clMedErrorIdentifierTranslationInsert"
 */
 ClRcT  clMedErrorIdentifierTranslationDelete(CL_IN ClMedHdlPtrT    medHdl,
                                    CL_IN ClUint32T  sysErrorType,
                                    CL_IN ClUint32T  sysErrorId);

/**
 ************************************
 *  \page pagemed110 clMedNotificationSubscribe
 *
 *  \par Synopsis:
 *   Subscribes for change notifications.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedNotificationSubscribe(
 *                                  CL_IN ClMedHdlPtrT    medHdl,
 *                                  CL_IN ClMedAgentIdT   clientIdentifier,
 *                                  CL_IN ClUint32T    sesionId);
 *
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param clientIdentifier: Identifier in agent terminology.
 *  \param sesionId: Session Identifier (optional).
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to subscribe for change notifications. This subscription is
 *  done based on the client Identifier value. So for a change in the client
 *  identifier, the agent is notified.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \note
 *  This API assumes that the Identifier translation is already added
 *  for the identifier being subscribed to.
 *
 *  \par Related Function(s):
 *  \ref clMedNotificationUnsubscribe, \ref clMedAllNotificationUnsubscribe.
 */

 ClRcT  clMedNotificationSubscribe(CL_IN ClMedHdlPtrT      medHdl,
                                  CL_IN ClMedAgentIdT   clientIdentifier,
                                  CL_IN ClUint32T    sesionId);

/**
 ************************************
 *  \page pagemed111 clMedNotificationUnsubscribe
 *
 *  \par Synopsis:
 *   Unsubscribes for change notifications.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedNotificationSubscribe(
 *                                  CL_IN ClMedHdlPtrT    medHdl,
 *                                  CL_IN ClMedAgentIdT   clientIdentifier,
 *                                  CL_IN ClUint32T    sesionId);
 *
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param clientIdentifier: Identifier in agent terminology.
 *  \param sesionId: Session Identifier. This is an optional parameter.
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to unsubscribe the change notifications. The agent will not be notified of
 *  changes in the client specified through the client identifier.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed110 "clMedNotificationSubscribe", \ref pagemed112 "clMedAllNotificationUnsubscribe"
 *
 */
 ClRcT  clMedNotificationUnsubscribe(CL_IN ClMedHdlPtrT      medHdl,
                                    CL_IN ClMedAgentIdT   clientIdentifier,
                                    CL_IN ClUint32T    sesionId);

/**
 ************************************
 *  \page pagemed112 clMedAllNotificationUnsubscribe
 *
 *  \par Synopsis:
 *   Unsubscribes for change notifications.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT  clMedAllNotificationUnsubscribe(
 *                                  CL_IN ClMedHdlPtrT    medHdl,
 *                                  CL_IN ClUint32T    sesionId);
 *  \endcode
 *
 *  \param medHdl: Handle returned as part of initialization.
 *  \param sesionId: Session Identifier (optional).
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to unsubscribe all change notifications. No client identifier needs to be
 *  specified here, since the agent does not receive notifications
 *  regarding changes in any client.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  \ref pagemed110 "clMedNotificationSubscribe", \ref pagemed111 "clMedNotificationUnsubscribe"
 *
 */

 ClRcT  clMedAllNotificationUnsubscribe(CL_IN ClMedHdlPtrT      medHdl,
                                       CL_IN ClUint32T    sesionId);

/**
 ************************************
 *  \page pagemed113 clMedTgtIdGet
 *
 *  \par Synopsis:
 *  Fetches the Target ID i.e. combination of COR MO ID and attribute ID 
 *  corresponding to the attribute in Northbound terminology.
 *
 *  \par Header File:
 *  clMedApi.h
 *
 *  \par Syntax:
 *  \code    ClRcT clMedTgtIdGet(CL_IN  ClMedHdlPtrT *pMedHdl, 
 *                               CL_IN ClMedAgentIdT *pAgntId, 
 *                               CL_OUT ClMedTgtIdT **ppTgtId);
 *  \endcode
 *
 *  \param pMedHdl: Handle to mediation.
 *  \param pAgntId: Attribute in Northbound terminology.
 *  \param ppTgtId: (out) Target ID. 
 *
 *  \retval CL_OK: This API executed successfully.
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid parameter.
 *
 *  \par Description:
 *  This API is used to fetch the Target ID i.e. combination of COR MO ID
 *  and attribute ID corresponding to the attribute in Northbound terminology e.g.
 *  OBJECT IDENTIFIER in SNMP.
 *  Memory allocated to the Target ID is required to be freed by the calling function.
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  none.
 *
 */


 ClRcT clMedTgtIdGet(CL_IN  ClMedHdlPtrT pMedHdl, 
                    CL_IN ClMedAgentIdT *pAgntId, 
                    CL_OUT ClMedTgtIdT **ppTgtId);



#ifdef __cplusplus
}
#endif
#endif /* _CL_MED_API_H_*/


/** \} */
