/*******************************************************************************
**
** FILE:
**   SaCkpt.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Checkpoint Service (CKPT). It contains all of 
**   the prototypes and type definitions required for CKPT. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-CKPT-B.02.01
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

#ifndef _SA_CKPT_H
#define _SA_CKPT_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaCkptHandleT;
typedef SaUint64T SaCkptCheckpointHandleT;
typedef SaUint64T SaCkptSectionIterationHandleT;

#define SA_CKPT_WR_ALL_REPLICAS 0X1
#define SA_CKPT_WR_ACTIVE_REPLICA 0X2
#define SA_CKPT_WR_ACTIVE_REPLICA_WEAK 0X4
#define SA_CKPT_CHECKPOINT_COLLOCATED 0X8

typedef SaUint32T SaCkptCheckpointCreationFlagsT;

typedef struct {
    SaCkptCheckpointCreationFlagsT creationFlags;
    SaSizeT                        checkpointSize;
    SaTimeT                        retentionDuration;
    SaUint32T                      maxSections;
    SaSizeT                        maxSectionSize;
    SaSizeT                        maxSectionIdSize;
} SaCkptCheckpointCreationAttributesT;

#define SA_CKPT_CHECKPOINT_READ 0X1
#define SA_CKPT_CHECKPOINT_WRITE 0X2
#define SA_CKPT_CHECKPOINT_CREATE 0X4

typedef SaUint32T SaCkptCheckpointOpenFlagsT;

#define SA_CKPT_DEFAULT_SECTION_ID   {0, NULL}
#define SA_CKPT_GENERATED_SECTION_ID {0, NULL}

typedef struct {
    SaUint16T idLen;
    SaUint8T *id;
} SaCkptSectionIdT;

typedef struct {
    SaCkptSectionIdT *sectionId;
    SaTimeT expirationTime;
} SaCkptSectionCreationAttributesT;

typedef enum {
    SA_CKPT_SECTION_VALID = 1,
    SA_CKPT_SECTION_CORRUPTED = 2
} SaCkptSectionStateT;

typedef struct {
    SaCkptSectionIdT    sectionId;
    SaTimeT             expirationTime;
    SaSizeT             sectionSize;
    SaCkptSectionStateT sectionState;
    SaTimeT             lastUpdate;
} SaCkptSectionDescriptorT;

typedef enum {
    SA_CKPT_SECTIONS_FOREVER = 1,
    SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME = 2,
    SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME = 3,
    SA_CKPT_SECTIONS_CORRUPTED = 4,
    SA_CKPT_SECTIONS_ANY = 5
} SaCkptSectionsChosenT;

typedef struct {
    SaCkptSectionIdT sectionId;
    void            *dataBuffer;
    SaSizeT          dataSize;
    SaOffsetT        dataOffset;
    SaSizeT          readSize;
} SaCkptIOVectorElementT;

typedef struct {
    SaCkptCheckpointCreationAttributesT checkpointCreationAttributes;
    SaUint32T                           numberOfSections;
    SaUint32T                           memoryUsed;
} SaCkptCheckpointDescriptorT;

typedef void 
(*SaCkptCheckpointOpenCallbackT)(
    SaInvocationT invocation,
    SaCkptCheckpointHandleT checkpointHandle,
    SaAisErrorT error);

typedef void 
(*SaCkptCheckpointSynchronizeCallbackT)(
    SaInvocationT invocation,
    SaAisErrorT error);

typedef struct {
    SaCkptCheckpointOpenCallbackT        saCkptCheckpointOpenCallback;
    SaCkptCheckpointSynchronizeCallbackT saCkptCheckpointSynchronizeCallback;
} SaCkptCallbacksT;

typedef enum {      
    SA_CKPT_SECTION_RESOURCES_EXHAUSTED = 1,
    SA_CKPT_SECTION_RESOURCES_AVAILABLE = 2
} saCkptCheckpointStatusT;

typedef enum {      
    SA_CKPT_CHECKPOINT_STATUS = 1
} SaCkptStateT;


/*************************************************/
/******** CKPT API function declarations *********/
/*************************************************/
extern SaAisErrorT 
saCkptInitialize(
    SaCkptHandleT *ckptHandle, 
    const SaCkptCallbacksT *callbacks,
    SaVersionT *version);

extern SaAisErrorT 
saCkptSelectionObjectGet(
    SaCkptHandleT ckptHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saCkptDispatch(
    SaCkptHandleT ckptHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saCkptFinalize(
    SaCkptHandleT ckptHandle);

extern SaAisErrorT
saCkptCheckpointOpen(
    SaCkptHandleT ckptHandle,
    const SaNameT *ckeckpointName,
    const SaCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
    SaCkptCheckpointOpenFlagsT checkpointOpenFlags,
    SaTimeT timeout,
    SaCkptCheckpointHandleT *checkpointHandle);

extern SaAisErrorT 
saCkptCheckpointOpenAsync(
    SaCkptHandleT ckptHandle,
    SaInvocationT invocation,
    const SaNameT *ckeckpointName,
    const SaCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
    SaCkptCheckpointOpenFlagsT checkpointOpenFlags);

extern SaAisErrorT
saCkptCheckpointClose(
    SaCkptCheckpointHandleT checkpointHandle);

extern SaAisErrorT 
saCkptCheckpointUnlink(
    SaCkptHandleT ckptHandle, 
    const SaNameT *checkpointName);

extern SaAisErrorT 
saCkptCheckpointRetentionDurationSet(
    SaCkptCheckpointHandleT checkpointHandle,
    SaTimeT retentionDuration);

extern SaAisErrorT 
saCkptActiveReplicaSet(
    SaCkptCheckpointHandleT checkpointHandle);

extern SaAisErrorT 
saCkptCheckpointStatusGet(
    SaCkptCheckpointHandleT checkpointHandle,
    SaCkptCheckpointDescriptorT *checkpointStatus);

extern SaAisErrorT 
saCkptSectionCreate(
    SaCkptCheckpointHandleT checkpointHandle,
    SaCkptSectionCreationAttributesT *sectionCreationAttributes,
    const SaUint8T *initialData,
    SaSizeT initialDataSize);

extern SaAisErrorT 
saCkptSectionDelete(
    SaCkptCheckpointHandleT checkpointHandle,
    const SaCkptSectionIdT *sectionId);

extern SaAisErrorT
saCkptSectionIdFree(
    SaCkptCheckpointHandleT checkpointHandle,
    SaUint8T *id);

extern SaAisErrorT 
saCkptSectionExpirationTimeSet(
    SaCkptCheckpointHandleT checkpointHandle,
    const SaCkptSectionIdT* sectionId,
    SaTimeT expirationTime);

extern SaAisErrorT 
saCkptSectionIterationInitialize(
    SaCkptCheckpointHandleT checkpointHandle,
    SaCkptSectionsChosenT sectionsChosen,
    SaTimeT expirationTime,
    SaCkptSectionIterationHandleT *sectionIterationHandle);

extern SaAisErrorT 
saCkptSectionIterationNext(
    SaCkptSectionIterationHandleT sectionIterationHandle,
    SaCkptSectionDescriptorT *sectionDescriptor);

extern SaAisErrorT 
saCkptSectionIterationFinalize(
    SaCkptSectionIterationHandleT sectionIterationHandle);

extern SaAisErrorT 
saCkptCheckpointWrite(
    SaCkptCheckpointHandleT checkpointHandle,
    const SaCkptIOVectorElementT *ioVector,
    SaUint32T numberOfElements,
    SaUint32T *erroneousVectorIndex);

extern SaAisErrorT 
saCkptSectionOverwrite(
    SaCkptCheckpointHandleT checkpointHandle,
    const SaCkptSectionIdT *sectionId,
    const void *dataBuffer,
    SaSizeT dataSize);

extern SaAisErrorT 
saCkptCheckpointRead(
    SaCkptCheckpointHandleT checkpointHandle,
    SaCkptIOVectorElementT *ioVector,
    SaUint32T numberOfElements,
    SaUint32T *erroneousVectorIndex);

extern SaAisErrorT      
saCkptIOVectorElementDataFree(
    SaCkptCheckpointHandleT checkpointHandle,
    void *data);

extern SaAisErrorT 
saCkptCheckpointSynchronize(
    SaCkptCheckpointHandleT ckeckpointHandle,
    SaTimeT timeout);

extern SaAisErrorT 
saCkptCheckpointSynchronizeAsync(
    SaCkptCheckpointHandleT checkpointHandle,
    SaInvocationT invocation);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_CKPT_H */

