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
 * File        : clSafCkptWrapper.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *   This file contains Checkpoint service APIs implementation
 *
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

#include <saAis.h>
#include <saCkpt.h>

#include <clCommonErrors.h>
#include <clCkptApi.h>
#include <clOsalApi.h>
#include "clCkptCommon.h"
#include <clCkptErrors.h>
#include <ipi/clEoIpi.h>
#include <clLogApi.h>

SaCkptCallbacksT gSafCallback;



/*
 * Function to translate clovis error types to SAF error types.
 */
 
void clErrorTxlate(ClRcT        clError,
                   SaAisErrorT  *aisError)
{
    if (clError == CL_OK) 
    {
        *aisError = SA_AIS_OK;
        return;
    }

    clError =  CL_GET_ERROR_CODE(clError);
    switch (clError)
    {
         case CL_ERR_NO_MEMORY:
             *aisError = SA_AIS_ERR_NO_MEMORY;
             break;

         case CL_ERR_INVALID_PARAMETER:
         case CL_ERR_NULL_POINTER:      
             *aisError = SA_AIS_ERR_INVALID_PARAM;
             break;
       
         case CL_IOC_ERR_COMP_UNREACHABLE:
         case CL_IOC_ERR_HOST_UNREACHABLE:
         case CL_ERR_OUT_OF_RANGE:    
         case CL_ERR_NOT_EXIST:
         case CL_ERR_DOESNT_EXIST:
             *aisError = SA_AIS_ERR_NOT_EXIST;
             break;

         case CL_ERR_INVALID_HANDLE:
              *aisError = SA_AIS_ERR_BAD_HANDLE;
              break;

         case CL_ERR_NOT_IMPLEMENTED:
              *aisError = SA_AIS_ERR_NOT_SUPPORTED;
              break;

         case CL_ERR_DUPLICATE:
         case CL_ERR_ALREADY_EXIST:
              *aisError = SA_AIS_ERR_EXIST;
              break;

         case CL_ERR_NO_RESOURCE:
              *aisError = SA_AIS_ERR_NO_RESOURCES;
              break;

         case CL_ERR_INITIALIZED:
              *aisError = SA_AIS_ERR_INIT;
              break;
         case CL_ERR_NOT_INITIALIZED:
              *aisError = SA_AIS_ERR_BAD_HANDLE;
              break;

         case CL_ERR_VERSION_MISMATCH:
              *aisError = SA_AIS_ERR_VERSION;
              break;

         case CL_ERR_BUFFER_OVERRUN :
              *aisError = SA_AIS_ERR_QUEUE_FULL;
              break;

         case CL_ERR_TIMEOUT:
              *aisError = SA_AIS_ERR_TIMEOUT;
              break;

         case CL_ERR_NO_SPACE:
              *aisError = SA_AIS_ERR_NO_SPACE;
              break;

         case CL_ERR_INUSE:
         case CL_ERR_TRY_AGAIN:
              *aisError = SA_AIS_ERR_TRY_AGAIN;
               break;
               
         case CL_ERR_OP_NOT_PERMITTED:
              *aisError = SA_AIS_ERR_ACCESS;
              break;
              
         case CL_ERR_BAD_FLAG:
              *aisError = SA_AIS_ERR_BAD_FLAGS;
              break;
              
         case CL_ERR_BAD_OPERATION:
              *aisError = SA_AIS_ERR_BAD_OPERATION;
              break;
         case CL_CKPT_ERR_NO_SECTIONS:
              *aisError = SA_AIS_ERR_NO_SECTIONS;
              break;
    }
    
    return;
}



/*
 * Clovis Open Calllback wrapper.
 */
 
void ckptWrapOpenCallback( ClInvocationT invocation,
                           ClCkptHdlT    ckptHdl,
                           ClRcT         rc)
{
    SaAisErrorT      safRc = SA_AIS_OK;

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    /*
     * Call the user's callback function.
     */
    if(gSafCallback.saCkptCheckpointOpenCallback != NULL)
    {
        gSafCallback.saCkptCheckpointOpenCallback(invocation,
                ckptHdl,
                safRc);
    }   
    
    return;
}



/*
 * Clovis Open Calllback wrapper.
 */
 
void ckptWrapSynchronizeCallback( ClInvocationT invocation,
        ClRcT         rc)
{
    SaAisErrorT      safRc = SA_AIS_OK;
    
    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);

    /*
     * Call the user's callback function.
     */
    if( gSafCallback.saCkptCheckpointSynchronizeCallback != NULL)
        gSafCallback.saCkptCheckpointSynchronizeCallback(invocation,safRc);
        
    return;
}



/*
 * saf ckpt initialize function.
 */

SaAisErrorT saCkptInitialize(SaCkptHandleT           *saCkptHandle,
                             const SaCkptCallbacksT  *callbacks,
                             SaVersionT              *version)
{
    ClRcT            rc            = CL_OK;
    SaAisErrorT      safRc         = SA_AIS_OK;
    ClCkptCallbacksT ckptCallbacks = {0};
    ClCkptSvcHdlT    ckptHandle    = CL_CKPT_INVALID_HDL;

    /*
     * Validate the input parameters.
     */
    if(saCkptHandle == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = clASPInitialize();
    if(CL_OK != rc)
    {
        clLogCritical("CKP", "INI",
                      "ASP initialize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }
    
    /*
     * Copy the callbacks in a global varible and register clovis specific
     * callback functions. These function will inturn call the user 
     * registered callback functions.
     */
    if(callbacks != NULL)
    {
        gSafCallback.saCkptCheckpointOpenCallback = 
            callbacks->saCkptCheckpointOpenCallback; 
        gSafCallback.saCkptCheckpointSynchronizeCallback = 
            callbacks->saCkptCheckpointSynchronizeCallback; 

        if(callbacks->saCkptCheckpointOpenCallback != NULL)
            ckptCallbacks.checkpointOpenCallback = ckptWrapOpenCallback; 
        else
            ckptCallbacks.checkpointOpenCallback = NULL;

        if(callbacks->saCkptCheckpointSynchronizeCallback != NULL)     
            ckptCallbacks.checkpointSynchronizeCallback = 
                ckptWrapSynchronizeCallback; 
        else
            ckptCallbacks.checkpointSynchronizeCallback = NULL;

        /*
         * Call the ckpt client library callback function.
         */
        rc = clCkptInitialize( &ckptHandle,
                &ckptCallbacks, 
                (ClVersionT *) version);
    }    
    else
    {
        /*
         * Call the ckpt client library callback function.
         */
        rc = clCkptInitialize( &ckptHandle,
                NULL, 
                (ClVersionT *) version);

    }

    /*
     * Copy th ckpt handle back to output buffer.
     */
    *saCkptHandle = ckptHandle;                       

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf ckpt finalize function.
 */
 
SaAisErrorT saCkptFinalize (const SaCkptHandleT ckptHandle)
{
    ClRcT            rc    = CL_OK;
    SaAisErrorT      safRc = SA_AIS_OK;

    rc = clCkptFinalize((ClCkptSvcHdlT)ckptHandle);
    
    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    if(CL_OK != clASPFinalize())
    {
        clLogInfo("CKP", "FIN",
                  "ASP finalize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }

    return safRc;
}



/*
 * saf ckpt open function.
 */
 
SaAisErrorT saCkptCheckpointOpen(
        SaCkptHandleT                             ckptHandle,
        const SaNameT                             *checkpointName,
        const SaCkptCheckpointCreationAttributesT *ckptAttributes,
        SaCkptCheckpointOpenFlagsT                checkpointOpenFlags,
        SaTimeT                                   timeout,
        SaCkptCheckpointHandleT                   *checkpointHandle)
{
    ClRcT                               rc         = CL_OK;
    SaAisErrorT                         safRc      = SA_AIS_OK;
    ClCkptHdlT                          tmpCkptHdl = 0;
    ClCkptCheckpointCreationAttributesT ckptAttr   = {0};

    /*
     * Validate the input parameters.
     */
    if(checkpointHandle == NULL)
        return SA_AIS_ERR_INVALID_PARAM;
        
    /*
     * Call the ckpt client open function.
     */
    if(ckptAttributes != NULL)
    {
        if( ckptAttributes->checkpointSize > 
            (ckptAttributes->maxSections * ckptAttributes->maxSectionSize) )
            {
                clLogWarning("CKP", "INI", "Possible inconsistency in SAF checkpoint open.  Your passed configuration parameters should have this relationship: checkpointSize (%d) is less than or equal to maxSections (%d) * maxSectionSize (%d).  Reducing checkpointSize to = maxSections * maxSectionSize (which is %d).",                 (int) ckptAttributes->checkpointSize, (int) ckptAttributes->maxSections, (int) ckptAttributes->maxSectionSize, (int) (ckptAttributes->maxSections * ckptAttributes->maxSectionSize));
                 ckptAttr.checkpointSize = ckptAttributes->maxSections * ckptAttributes->maxSectionSize;
            }
        else ckptAttr.checkpointSize    = ckptAttributes->checkpointSize;

        ckptAttr.creationFlags     = ckptAttributes->creationFlags;
        ckptAttr.retentionDuration = ckptAttributes->retentionDuration;
        ckptAttr.maxSectionSize    = ckptAttributes->maxSectionSize;
        ckptAttr.maxSections       = ckptAttributes->maxSections;
        ckptAttr.maxSectionIdSize  = ckptAttributes->maxSectionIdSize;
        
        rc = clCkptCheckpointOpen( (ClCkptSvcHdlT) ckptHandle,
                (ClNameT *)checkpointName, 
                &ckptAttr,
                (ClCkptOpenFlagsT) checkpointOpenFlags, 
                (ClTimeT)timeout, 
                &tmpCkptHdl);
    }
    else
    {
        rc = clCkptCheckpointOpen( (ClCkptSvcHdlT) ckptHandle,
                (ClNameT *)checkpointName, 
                NULL,
                (ClCkptOpenFlagsT) checkpointOpenFlags, 
                (ClTimeT)timeout, 
                &tmpCkptHdl);
    }
    
    /*
     * Copy the checkpoint handle to the output variable.
     */
    *checkpointHandle = tmpCkptHdl;

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint close function.
 */
 
SaAisErrorT saCkptCheckpointClose(SaCkptCheckpointHandleT checkpointHandle)
{
    ClRcT          rc    = CL_OK;
    SaAisErrorT    safRc = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc =  clCkptCheckpointClose((ClCkptHdlT) checkpointHandle);
    
    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint unlink function.
 */
 
SaAisErrorT saCkptCheckpointUnlink(SaCkptHandleT ckptHandle,
                                   const SaNameT *checkpointName)
{
    ClRcT         rc    = CL_OK;
    SaAisErrorT   safRc = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointDelete((ClCkptSvcHdlT) ckptHandle,
            (ClNameT *)checkpointName);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint status get function.
 */
 
SaAisErrorT saCkptCheckpointStatusGet(
        SaCkptCheckpointHandleT     checkpointHandle,
        SaCkptCheckpointDescriptorT *checkpointStatus)
{
    ClRcT                       rc         = CL_OK;
    SaAisErrorT                 safRc      = SA_AIS_OK;
    ClCkptCheckpointDescriptorT ckptStatus = {{0}};

    /*
     * Validate the input parameters.
     */
    if(checkpointStatus == NULL)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }
    
    memset(&ckptStatus,'\0',sizeof(ClCkptCheckpointDescriptorT));
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointStatusGet((ClCkptHdlT)checkpointHandle,
                                   &ckptStatus);
    
    /*
     * Copy the status info to the output buffer.
     */
    checkpointStatus->checkpointCreationAttributes.creationFlags     = 
                ckptStatus.checkpointCreationAttributes.creationFlags;
    checkpointStatus->checkpointCreationAttributes.checkpointSize    = 
                ckptStatus.checkpointCreationAttributes.checkpointSize;
    checkpointStatus->checkpointCreationAttributes.retentionDuration = 
                ckptStatus.checkpointCreationAttributes.retentionDuration;
    checkpointStatus->checkpointCreationAttributes.maxSectionSize    = 
                ckptStatus.checkpointCreationAttributes.maxSectionSize;
    checkpointStatus->checkpointCreationAttributes.maxSections       = 
                ckptStatus.checkpointCreationAttributes.maxSections;
    checkpointStatus->checkpointCreationAttributes.maxSectionIdSize  = 
                ckptStatus.checkpointCreationAttributes.maxSectionIdSize;
    checkpointStatus->numberOfSections = ckptStatus.numberOfSections;
    checkpointStatus->memoryUsed       = ckptStatus.memoryUsed;

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf section create function.
 */
 
SaAisErrorT saCkptSectionCreate(
        SaCkptCheckpointHandleT          checkpointHandle,
        SaCkptSectionCreationAttributesT *sectionCreationAttributes,
        const SaUint8T                   *initialData,
        SaSizeT                          initialDataSize)
{
    ClRcT            rc = CL_OK;
    SaAisErrorT      safRc = SA_AIS_OK;
    ClCkptSectionCreationAttributesT sectionCreateAttr;


    /*
     * Validate the input parameters.
     */
    if(sectionCreationAttributes == NULL) return SA_AIS_ERR_INVALID_PARAM;

    memset(&sectionCreateAttr ,'\0',sizeof(ClCkptSectionCreationAttributesT));

    /*
     * Copy the section info into clovis format.
     */
    sectionCreateAttr.sectionId = (ClCkptSectionIdT *)clHeapAllocate(
                                          sizeof(ClCkptSectionIdT));
    if(sectionCreateAttr.sectionId == NULL)
    {
        safRc = SA_AIS_ERR_NO_MEMORY;
        return safRc;
    }
    memset(sectionCreateAttr.sectionId,'\0',sizeof(ClCkptSectionIdT));
    sectionCreateAttr.sectionId->idLen = 
              sectionCreationAttributes->sectionId->idLen; 
    sectionCreateAttr.sectionId->id    = (ClUint8T *)clHeapAllocate(
                                sectionCreateAttr.sectionId->idLen);
    if(sectionCreateAttr.sectionId->id == NULL)
    {
        clHeapFree(sectionCreateAttr.sectionId);
        safRc = SA_AIS_ERR_NO_MEMORY;
        return safRc;
    }
    
    memset(sectionCreateAttr.sectionId->id, '\0',
           sectionCreateAttr.sectionId->idLen);
    memcpy(sectionCreateAttr.sectionId->id, 
           sectionCreationAttributes->sectionId->id,
           sectionCreateAttr.sectionId->idLen);
           
    if( sectionCreationAttributes->expirationTime == SA_TIME_END)
    {
        sectionCreateAttr.expirationTime = CL_TIME_END; 
    }    
    else
    {
        sectionCreateAttr.expirationTime = 
            sectionCreationAttributes->expirationTime; 
    }    
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionCreate((ClCkptHdlT) checkpointHandle,
            &sectionCreateAttr,
            (ClUint8T *)initialData,
            (ClSizeT) initialDataSize);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    clHeapFree(sectionCreateAttr.sectionId->id); 
    clHeapFree(sectionCreateAttr.sectionId); 
    
    return safRc;
}



/*
 * saf section delete function.
 */
 
SaAisErrorT saCkptSectionDelete(SaCkptCheckpointHandleT checkpointHandle,
                                const SaCkptSectionIdT  *sectionId)
{
    ClRcT            rc = CL_OK;
    SaAisErrorT      safRc = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionDelete((ClCkptHdlT ) checkpointHandle,
            (ClCkptSectionIdT *)sectionId);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint write function.
 */
 
SaAisErrorT saCkptCheckpointWrite(
        SaCkptCheckpointHandleT      checkpointHandle,
        const SaCkptIOVectorElementT *pIoVector,
        SaUint32T                    numberOfElements,
        SaUint32T                    *erroneousVectorIndex)
{
    ClRcT                   rc       = CL_OK;
    SaAisErrorT             safRc    = SA_AIS_OK;
    ClCkptIOVectorElementT *pTempVec = NULL;
    ClCkptIOVectorElementT *pInVector = NULL; 
    ClUint32T               count     = 0;

    /*
     * Validate the input parameters.
     */
    if(pIoVector == NULL) return SA_AIS_ERR_INVALID_PARAM;

    /*
     * Copy the iovector in clovis format.
     */
    pTempVec = (ClCkptIOVectorElementT *)pIoVector;
    if(pIoVector != NULL)
    { 
        pInVector = (ClCkptIOVectorElementT *)clHeapAllocate(
                sizeof(ClCkptIOVectorElementT) * numberOfElements); 
        memset(pInVector, '\0',
               sizeof(ClCkptIOVectorElementT) * numberOfElements);
        pTempVec = pInVector;
        for(count = 0; count < numberOfElements; count ++)
        {
            pInVector->sectionId.idLen = pIoVector->sectionId.idLen;
            pInVector->sectionId.id = (ClUint8T *) clHeapAllocate(
                            pInVector->sectionId.idLen);
            if(pInVector->sectionId.id == NULL)
            {
                safRc = SA_AIS_ERR_NO_MEMORY;
                return safRc;
            }
            memset(pInVector->sectionId.id,'\0',pInVector->sectionId.idLen);
            memcpy(pInVector->sectionId.id, pIoVector->sectionId.id,
                   pInVector->sectionId.idLen);
            pInVector->dataBuffer = (ClUint8T*)pIoVector->dataBuffer;
            pInVector->dataSize   = pIoVector->dataSize;
            pInVector->dataOffset = pIoVector->dataOffset;
            pInVector->readSize   = pIoVector->readSize;    
            pInVector++;
            pIoVector++;
        }
    }
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointWrite((ClCkptHdlT) checkpointHandle,
            pTempVec,
            (ClUint32T )numberOfElements,
            (ClUint32T *)erroneousVectorIndex);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    if(pTempVec != NULL)
    { 
        pInVector = pTempVec;
        for(count = 0; count < numberOfElements; count++)
        {
            clHeapFree(pInVector->sectionId.id);
            pInVector++;
        }
        clHeapFree(pTempVec);
    }
    
    return safRc;
}



/*
 * saf section write function.
 */
 
SaAisErrorT saCkptSectionOverwrite(
        SaCkptCheckpointHandleT checkpointHandle,
        const SaCkptSectionIdT  *sectionId,
        const void              *dataBuffer,
        SaSizeT                 dataSize)
{
    ClRcT          rc    = CL_OK;
    SaAisErrorT    safRc = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionOverwrite( (ClCkptHdlT) checkpointHandle,
            (ClCkptSectionIdT   *)sectionId,
            (void *)dataBuffer,
            (ClSizeT)dataSize);


    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint read function.
 */
 
SaAisErrorT saCkptCheckpointRead(SaCkptCheckpointHandleT checkpointHandle,
                                 SaCkptIOVectorElementT  *pIoVector,
                                 SaUint32T               numberOfElements,
                                 SaUint32T              *erroneousVectorIndex)
{
    ClRcT                   rc          = CL_OK;
    SaAisErrorT             safRc       = SA_AIS_OK;
    ClCkptIOVectorElementT *pTempVec    = NULL;
    SaCkptIOVectorElementT *pTempOutVec = NULL;
    ClCkptIOVectorElementT *pInVector   = NULL; 
    ClUint32T               count       = 0;

    /*
     * Validate the input parameters.
     */
    if(pIoVector == NULL) return SA_AIS_ERR_INVALID_PARAM;

    /*
     * Copy the iovector in clovis format.
     */
    pTempVec = (ClCkptIOVectorElementT *)pIoVector;
    if(pIoVector != NULL)
    { 
        pInVector = (ClCkptIOVectorElementT *)clHeapAllocate(
                    sizeof(ClCkptIOVectorElementT) * numberOfElements); 
        if(pInVector == NULL)
        {
            safRc = SA_AIS_ERR_NO_MEMORY;
            return safRc;
        }
        
        memset(pInVector, '\0',
              sizeof(ClCkptIOVectorElementT) * numberOfElements);
        pTempVec    = pInVector;
        pTempOutVec = pIoVector;
        for(count = 0; count < numberOfElements; count ++)
        {
            pInVector->sectionId.idLen = pIoVector->sectionId.idLen;
            pInVector->sectionId.id = (ClUint8T *) clHeapAllocate(
                                pIoVector->sectionId.idLen);
            if(pInVector->sectionId.id == NULL)
            {
                safRc = SA_AIS_ERR_NO_MEMORY;
                return safRc;
            }
            memset(pInVector->sectionId.id,'\0',pInVector->sectionId.idLen);
            memcpy(pInVector->sectionId.id, pIoVector->sectionId.id, 
                   pInVector->sectionId.idLen);
            pInVector->dataBuffer = (ClUint8T*)pIoVector->dataBuffer;
            pInVector->dataSize   = pIoVector->dataSize;
            pInVector->dataOffset = pIoVector->dataOffset;
            pInVector->readSize   = pIoVector->readSize;    
            pInVector++;
            pIoVector++;
        }
    }
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointRead( (ClCkptHdlT) checkpointHandle,
            pTempVec,
            (ClUint32T) numberOfElements,
            (ClUint32T *)erroneousVectorIndex);
            
    if(pTempVec != NULL)
    { 
        pInVector = pTempVec;
        pIoVector = pTempOutVec;
        for(count = 0; count < numberOfElements; count++)
        {
            pIoVector->dataBuffer = pInVector->dataBuffer; 
            pIoVector->readSize = pInVector->readSize;
            clHeapFree(pInVector->sectionId.id);
            pInVector++;
            pIoVector++;
        }
        clHeapFree(pTempVec);
    }
    pIoVector = pTempOutVec;

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf selection object get function.
 */
 
SaAisErrorT saCkptSelectionObjectGet(const SaCkptHandleT ckptHandle,
                                     SaSelectionObjectT  *selectionObject)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSelectionObjectGet((ClCkptHdlT)ckptHandle,
            (ClSelectionObjectT *)selectionObject);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf selection object get function.
 */
 
SaAisErrorT saCkptDispatch(const SaCkptHandleT ckptHandle,
                           SaDispatchFlagsT    dispatchFlags)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptDispatch( (ClCkptHdlT)ckptHandle,
            (ClDispatchFlagsT)dispatchFlags);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint open async function
 */
 
SaAisErrorT saCkptCheckpointOpenAsync (
        const SaCkptHandleT                       ckptHandle,
        SaInvocationT                             invocation,    
        const SaNameT                             *checkpointName,
        const SaCkptCheckpointCreationAttributesT *ckptAttributes,
        SaCkptCheckpointOpenFlagsT                checkpointOpenFlags)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;
    ClCkptCheckpointCreationAttributesT ckptAttr;

    /*
     * Validate the input parameters.
     */
    if(ckptAttributes == NULL && 
       checkpointOpenFlags == CL_CKPT_CHECKPOINT_CREATE) 
        return SA_AIS_ERR_INVALID_PARAM;
        
    /*
     * Call the corresponding ckpt client library function.
     */
    if(ckptAttributes != NULL)
    {
        ckptAttr.creationFlags = ckptAttributes->creationFlags;
        ckptAttr.checkpointSize = ckptAttributes->checkpointSize;
        ckptAttr.retentionDuration = ckptAttributes->retentionDuration;
        ckptAttr.maxSectionSize = ckptAttributes->maxSectionSize;
        ckptAttr.maxSections = ckptAttributes->maxSections;
        ckptAttr.maxSectionIdSize = ckptAttributes->maxSectionIdSize;
        rc = clCkptCheckpointOpenAsync( (ClCkptSvcHdlT)ckptHandle,
                (ClInvocationT) invocation,
                (ClNameT *)checkpointName,
                &ckptAttr,
                (ClCkptOpenFlagsT)checkpointOpenFlags);
    }
    else
    {
        rc = clCkptCheckpointOpenAsync( (ClCkptSvcHdlT)ckptHandle,
                (ClInvocationT) invocation,
                (ClNameT *)checkpointName,
                NULL,
                (ClCkptOpenFlagsT)checkpointOpenFlags);

    }

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint retention duration set function.
 */
 
SaAisErrorT saCkptCheckpointRetentionDurationSet (
        SaCkptCheckpointHandleT checkpointHandle,
        SaTimeT                  retentionDuration)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointRetentionDurationSet( (ClCkptHdlT)checkpointHandle,
            (ClTimeT)retentionDuration);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint active replica set function.
 */
 
SaAisErrorT saCkptActiveReplicaSet(SaCkptCheckpointHandleT checkpointHandle)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptActiveReplicaSet((ClCkptHdlT) checkpointHandle);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf section iteration initialize function.
 */
 
SaAisErrorT saCkptSectionIterationInitialize(
        SaCkptCheckpointHandleT       checkpointHandle,
        SaCkptSectionsChosenT         sectionsChosen,
        SaTimeT                       expirationTime,
        SaCkptSectionIterationHandleT *sectionIterationHandle)
{
    ClRcT          rc      = CL_OK;
    SaAisErrorT    safRc   = SA_AIS_OK;
    ClHandleT      secHdl  = CL_CKPT_INVALID_HDL;

    /*
     * Validate the input parameters.
     */
    if( NULL == sectionIterationHandle)
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionIterationInitialize((ClCkptHdlT)checkpointHandle,
            (ClCkptSectionsChosenT)sectionsChosen,
            (ClTimeT)expirationTime,
            &secHdl);

    /*
     * Copy th iteration handle to th eoutput buffer.
     */
    *sectionIterationHandle = (SaCkptSectionIterationHandleT)secHdl; 

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf section iteration next function.
 */
 
SaAisErrorT saCkptSectionIterationNext(
        SaCkptSectionIterationHandleT sectionIterationHandle,
        SaCkptSectionDescriptorT      *sectionDescriptor)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Validate the input parameters.
     */
    if(sectionDescriptor == NULL) 
    {
        return SA_AIS_ERR_INVALID_PARAM;
    }
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionIterationNext( (ClHandleT)sectionIterationHandle,
            (ClCkptSectionDescriptorT *)sectionDescriptor);
    if( rc == CL_OK)
    {
        if(sectionDescriptor->expirationTime == CL_TIME_END)
        {
            sectionDescriptor->expirationTime = SA_TIME_END;
        }	
    }

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf section iteration finalize function.
 */
 
SaAisErrorT saCkptSectionIterationFinalize (
        SaCkptSectionIterationHandleT sectionIterationHandle)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionIterationFinalize((ClHandleT)sectionIterationHandle);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint synchronize async function.
 */

SaAisErrorT saCkptCheckpointSynchronizeAsync(
        SaCkptCheckpointHandleT checkpointHandle,
        SaInvocationT           invocation)
{
    ClRcT          rc     = CL_OK;
    SaAisErrorT    safRc  = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointSynchronizeAsync((ClCkptHdlT)checkpointHandle,
            (ClInvocationT) invocation);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf section expiration set function.
 */

SaAisErrorT saCkptSectionExpirationTimeSet (
        SaCkptCheckpointHandleT checkpointHandle,
        const SaCkptSectionIdT  *sectionId,
        SaTimeT                 expirationTime)
{
    ClRcT            rc        = CL_OK;
    SaAisErrorT      safRc     = SA_AIS_OK;
    ClTimeT          expryTime = 0;
    
    if( expirationTime == SA_TIME_END )
    {
        expryTime = CL_TIME_END;
    }
    else
    {
        expryTime = expirationTime; 
    }
    
    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptSectionExpirationTimeSet((ClCkptHdlT) checkpointHandle,
            (ClCkptSectionIdT *)sectionId,
            expryTime);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}



/*
 * saf checkpoint synchronize function.
 */

SaAisErrorT saCkptCheckpointSynchronize(
        SaCkptCheckpointHandleT checkpointHandle,
        SaTimeT                 timeout)
{
    ClRcT            rc = CL_OK;
    SaAisErrorT      safRc = SA_AIS_OK;

    /*
     * Call the corresponding ckpt client library function.
     */
    rc = clCkptCheckpointSynchronize((ClCkptHdlT)checkpointHandle,
            (ClTimeT)timeout);

    /*
     * Translate the clovis error type to SAF error type.
     */
    clErrorTxlate(rc, &safRc);
    
    return safRc;
}

