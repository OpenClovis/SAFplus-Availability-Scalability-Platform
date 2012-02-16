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
 * ModuleName  : ckpt
 * File        : saCkpt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*

  Header file of SA Forum AIS CKPT APIs (SAI-AIS-B.01.00.09)
  compiled on 28SEP2004 by sayandeb.saha@motorola.com.
*/

#ifndef _SA_CKPT_H_
#define _SA_CKPT_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <saAis.h>

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
         SaSizeT checkpointSize;
         SaTimeT retentionDuration;
         SaUint32T maxSections;
         SaSizeT maxSectionSize;
         SaSizeT maxSectionIdSize;
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
    SaCkptSectionIdT sectionId;
    SaTimeT expirationTime;
    SaSizeT sectionSize;
    SaCkptSectionStateT sectionState;
    SaTimeT lastUpdate;
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
    void *dataBuffer;
    SaSizeT dataSize;
    SaOffsetT dataOffset;
    SaSizeT readSize;
} SaCkptIOVectorElementT;

typedef struct {
    SaCkptCheckpointCreationAttributesT checkpointCreationAttributes;
    SaUint32T numberOfSections;
    SaUint32T memoryUsed;
} SaCkptCheckpointDescriptorT;

typedef void 
(*SaCkptCheckpointOpenCallbackT)(SaInvocationT invocation,
                                 SaCkptCheckpointHandleT checkpointHandle,
                                 SaAisErrorT error);
typedef void 
(*SaCkptCheckpointSynchronizeCallbackT)(SaInvocationT invocation,
                                        SaAisErrorT error);

typedef struct {
    SaCkptCheckpointOpenCallbackT saCkptCheckpointOpenCallback;
    SaCkptCheckpointSynchronizeCallbackT saCkptCheckpointSynchronizeCallback;
} SaCkptCallbacksT;

    extern SaAisErrorT 
saCkptInitialize(SaCkptHandleT *ckptHandle, 
                 const SaCkptCallbacksT *callbacks,
                 SaVersionT *version);
    extern SaAisErrorT 
saCkptSelectionObjectGet(SaCkptHandleT ckptHandle,
                         SaSelectionObjectT *selectionObject);
    extern SaAisErrorT 
saCkptDispatch(SaCkptHandleT ckptHandle, 
               SaDispatchFlagsT dispatchFlags);
    extern SaAisErrorT 
saCkptFinalize(SaCkptHandleT ckptHandle);
    extern SaAisErrorT
saCkptCheckpointOpen(SaCkptHandleT ckptHandle,
                     const SaNameT *ckeckpointName,
                     const SaCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
                     SaCkptCheckpointOpenFlagsT checkpointOpenFlags,
                     SaTimeT timeout,
                     SaCkptCheckpointHandleT *checkpointHandle);
    extern SaAisErrorT 
saCkptCheckpointOpenAsync(SaCkptHandleT ckptHandle,
                          SaInvocationT invocation,
                          const SaNameT *ckeckpointName,
                          const SaCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
                          SaCkptCheckpointOpenFlagsT checkpointOpenFlags);
    extern SaAisErrorT
saCkptCheckpointClose(SaCkptCheckpointHandleT checkpointHandle);
    extern SaAisErrorT 
saCkptCheckpointUnlink(SaCkptHandleT ckptHandle, 
                       const SaNameT *checkpointName);
    extern SaAisErrorT 
saCkptCheckpointRetentionDurationSet(SaCkptCheckpointHandleT checkpointHandle,
                                     SaTimeT retentionDuration);
    extern SaAisErrorT 
saCkptActiveReplicaSet(SaCkptCheckpointHandleT checkpointHandle);
    extern SaAisErrorT 
saCkptCheckpointStatusGet(SaCkptCheckpointHandleT checkpointHandle,
                          SaCkptCheckpointDescriptorT *checkpointStatus);
    extern SaAisErrorT 
saCkptSectionCreate(SaCkptCheckpointHandleT checkpointHandle,
                    SaCkptSectionCreationAttributesT *sectionCreationAttributes,
                    const SaUint8T *initialData,
                    SaSizeT initialDataSize);
    extern SaAisErrorT 
saCkptSectionDelete(SaCkptCheckpointHandleT checkpointHandle,
                    const SaCkptSectionIdT *sectionId);
    extern SaAisErrorT 
saCkptSectionExpirationTimeSet(SaCkptCheckpointHandleT checkpointHandle,
                               const SaCkptSectionIdT* sectionId,
                               SaTimeT expirationTime);
    extern SaAisErrorT 
saCkptSectionIterationInitialize(SaCkptCheckpointHandleT checkpointHandle,
                                SaCkptSectionsChosenT sectionsChosen,
                                SaTimeT expirationTime,
                                SaCkptSectionIterationHandleT *sectionIterationHandle);
    extern SaAisErrorT 
saCkptSectionIterationNext(SaCkptSectionIterationHandleT sectionIterationHandle,
                                  SaCkptSectionDescriptorT *sectionDescriptor);
    extern SaAisErrorT 
saCkptSectionIterationFinalize(SaCkptSectionIterationHandleT sectionIterationHandle);
    extern SaAisErrorT 
saCkptCheckpointWrite(SaCkptCheckpointHandleT checkpointHandle,
                      const SaCkptIOVectorElementT *ioVector,
                      SaUint32T numberOfElements,
                      SaUint32T *erroneousVectorIndex);
   extern SaAisErrorT 
saCkptSectionOverwrite(SaCkptCheckpointHandleT checkpointHandle,
                       const SaCkptSectionIdT *sectionId,
                       const void *dataBuffer,
                       SaSizeT dataSize);
    extern SaAisErrorT 
saCkptCheckpointRead(SaCkptCheckpointHandleT checkpointHandle,
                     SaCkptIOVectorElementT *ioVector,
                     SaUint32T numberOfElements,
                     SaUint32T *erroneousVectorIndex);
    extern SaAisErrorT 
saCkptCheckpointSynchronize(SaCkptCheckpointHandleT ckeckpointHandle,
                            SaTimeT timeout);
    extern SaAisErrorT 
saCkptCheckpointSynchronizeAsync(SaCkptCheckpointHandleT checkpointHandle,
                                 SaInvocationT invocation);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_CKPT_H_ */

