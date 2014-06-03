/*******************************************************************************
**
** FILE:
**   saSec.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Security Service (SEC). It contains all of 
**   the prototypes and type definitions required for SEC. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-SEC-A.01.01
**
** DATE: 
**   Mon  Aug   27  2007  
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2007 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/


#ifndef _SA_SEC_H
#define _SA_SEC_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaSecHandleT;
typedef SaStringT SaSecPkiFileT;
typedef SaAnyT SaSecSecretT;
typedef SaUint64T SaSecUidT;

typedef struct {
    SaStringT *args;
    SaUint16T numberOfArgs;
} SaSecArgsArrayT;

typedef enum {
    SA_SEC_PKI = 1,
    SA_SEC_SHARED_SECRET = 2,
    SA_SEC_LOGIN = 3,
    SA_SEC_UID = 4
} SaSecAuthenticationMechT;

typedef struct {
    SaStringT userName;
    SaStringT userPassword;
} SaSecLoginT;

typedef union {
    SaSecUidT cred;
    SaSecLoginT login;
    SaSecPkiFileT pkiFile;
    SaSecSecretT sharedSecret;
} SaSecAuthenticationTokenT;

typedef enum {
    SA_SEC_AUTH_MECH_ID = 1
} SaSecConfAttrIdT;

typedef struct {
    SaSecConfAttrIdT attributeType;
    SaUint8T *attributeList;
    SaUint32T numberOfAttributes;
} SaSecAttributeArrayT;

typedef struct {
    SaUint64T contextIDSize;
    SaUint8T *contextID;
} SaSecContextT;

typedef enum {
    SA_SEC_ADD_RESOURCE = 1,
    SA_SEC_REMOVE_RESOURCE = 2
} SaSecOperationTypeT;

typedef struct {
    SaServicesT saService;
    SaUint32T saServiceOperation;
} SaSecOperT;

typedef struct {
    SaSecOperT *listOfSecOperations;
    SaUint32T numberOfSecOperations;
} SaSecOperListT;

typedef struct {
    SaSecAuthenticationMechT authenticationMechanism;
    SaSecAuthenticationTokenT authenticationToken;
} SaSecParamsT;

typedef enum {
    SA_SEC_POLICY_CHANGE = 1,
    SA_SEC_TERMINATE = 2,
    SA_SEC_PARAMS_CHANGE = 3
} SaSecContextChangeT;

typedef void (*SaSecContextChangeCallbackT) (
    SaSecHandleT secHandle,
    const SaSecContextT *secContext,
    SaSecContextChangeT changeReason);

typedef struct {
    SaSecContextChangeCallbackT saSecContextChangeCallback;
} SaSecCallbacksT;

/*************************************************/
/******** SEC API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saSecInitialize(
    SaSecHandleT *secHandle, 
    const SaSecCallbacksT *secCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saSecSelectionObjectGet(
    SaSecHandleT secHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saSecDispatch(
    SaSecHandleT secHandle,
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saSecFinalize(
    SaSecHandleT secHandle);

extern SaAisErrorT
saSecAuthenticate(
    SaSecHandleT secHandle,
    const SaSecParamsT *secParams,
    SaSecContextT *secContext);

extern SaAisErrorT
saSecContextFree(
    SaSecHandleT secHandle,
    SaUint8T *contextID);

extern SaAisErrorT
saSecAuthorize(
    SaSecHandleT secHandle,
    const SaSecContextT *secContext,
    SaSecOperT operation,
    const SaSecArgsArrayT *args);

extern SaAisErrorT
saSecConfAttrGet(
    SaSecHandleT secHandle,
    SaSecAttributeArrayT *attributes);

extern SaAisErrorT
saSecConfAttrFree(
    SaSecHandleT secHandle,
    SaUint8T *attributeArray);

extern SaAisErrorT
saSecRightsGet(
    SaSecHandleT secHandle,
    const SaSecContextT *secContext,
    SaServicesT service,
    const SaSecArgsArrayT *secArgs,
    SaSecOperListT *secOperations);

extern SaAisErrorT
saSecRightsFree(
    SaSecHandleT secHandle,
    SaSecOperT *listOfSecOperations);

extern SaAisErrorT
saSecContextChangeSubscribe(
    SaSecHandleT secHandle,
    const SaSecContextT *secContext);

extern SaAisErrorT
saSecContextChangeUnsubscribe(
    SaSecHandleT secHandle,
    const SaSecContextT *secContext);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_SEC_H */
