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
 * ModuleName  : amf
 * File        : clAmsCkpt.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the AMS server file relating to AMS checkpoint.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <clAmsServerUtils.h>
#include <clAmsCkpt.h>
#include <clJobQueue.h>
#include <clCkptExtApi.h>
#include <ipi/clCkptIpi.h>
#include <clAmsErrors.h>
#include <clAms.h>
#include <clDebugApi.h>
#include <clAmsModify.h>
#include <clAmsDBPackUnpack.h>
#include <clAmsSAServerApi.h>
#include <clVersionApi.h>
#include <clDifferenceVector.h>
#include <clCkptIpi.h>
#include <clNodeCache.h>

#define CL_AMS_INVOCATION_CKPT  0x10 
#define CL_AMS_DB_CKPT  ( CL_AMS_INVOCATION_CKPT + 1 )
#define CL_AMS_CKPT_GROUP_ID  0x0
#define CL_AMS_CKPT_ORDER  0x00
#define CL_AMS_CKPT_NAME  "AMS_CKPT"
#define AMS_CKPT_DB_SECTION  "AMS_CKPT_DB_SECTION"
#define AMS_CKPT_DIRTY_SECTION "AMS_CKPT_DIRTY_SECTION"
#define AMS_CKPT_INVOCATION_SECTION  "AMS_CKPT_INVOCATION_SECTION"
#define AMS_CKPT_VERSION_SECTION  "AMS_CKPT_VERSION_SECTION"
#define AMS_CKPT_CURRENT_SECTION  "AMS_CKPT_CURRENT_SECTION"
#define AMS_CKPT_SIZE  0
#define AMS_CKPT_MAX_SECTION_SIZE  0
#define AMS_CKPT_MAX_SECTION_ID_SIZE  256 
#define AMS_CKPT_RETENTION_DURATION  (0)
#define AMS_CKPT_MAX_SECTIONS           ((CL_AMS_DB_INVOCATION_PAIRS<<1)+2)
#define CL_AMS_CKPT_VERSION  "B.03.01"
#define CL_AMS_CKPT_GET_DB_INVOCATION_PAIR(pair)                        \
    (pair = gClAmsCkptCurrentDbInvocationPair++,                        \
     gClAmsCkptCurrentDbInvocationPair %= 2,   \
     pair)


#define AMS_CKPT_FREQUENCY (gClAmsCkptFrequency)
#define AMS_CKPT_FREQUENCY_MSEC (AMS_CKPT_FREQUENCY * 1000LL)
#define AMS_CKPT_FREQUENCY_USEC (AMS_CKPT_FREQUENCY_MSEC * 1000LL)
#define AMS_CKPT_WRITE_PAUSE_MSEC (200)
#define AMS_CKPT_MIN_FREQUENCY_USEC (5000000LL)
#define AMS_CKPT_WRITE_THRESHOLD_FAST (5)
#define AMS_CKPT_WRITE_FREQUENCY_USEC (1000000LL)
#define AMS_CKPT_WRITE_THRESHOLD_SLOW (10)

static ClUint32T gClAmsCkptFrequency;
static ClBoolT gClAmsCkptDifferential;
static ClCharT gClAmsCkptVersionBuf[CL_MAX_NAME_LENGTH];
static ClJobQueueT gClAmsCkptJobQueue;
static ClCkptSvcHdlT gClAmsCkptDBHdl;
static ClBoolT gClAmsCkptDBInitialized = CL_FALSE;
static ClBoolT gClAmsCkptDBDatasetInitialized = CL_FALSE;
static ClNameT gClAmsCkptDBName ;
static ClRcT clAmsCkptDBConfigSerialize(ClUint32T dsId, ClAddrT *pData, ClUint32T *pDataLen, ClPtrT cookie);
static ClRcT clAmsCkptDBConfigDeserialize(ClUint32T dsId, ClAddrT pData, ClUint32T dataLen, ClPtrT cookie);
                                       
static ClUint32T gClAmsCkptCurrentDbInvocationPair;
static ClUint32T gClAmsCkptLastDbInvocationPair;
static ClNameT gClAmsCkptDBSectionCache[2];
static ClNameT gClAmsCkptDirtySectionCache[2];
static ClNameT gClAmsCkptInvocationSectionCache[2];
static ClNameT gClAmsCkptCurrentSectionCache;
static ClInt32T gClAmsPersistentDBDisabled = -1;

static ClRcT
clAmsCkptNotifyCallback(ClCkptHdlT              ckptHdl,
                        ClNameT                 *pName,
                        ClCkptIOVectorElementT  *pIOVector,
                        ClUint32T               numSections,
                        ClPtrT                  pCookie);

static ClRcT clAmsCkptCheckpointRead(
                                     ClAmsT *ams,
                                     ClNameT *pSection,
                                     ClCkptIOVectorElementT *pIOVector
                                     )
{
    ClRcT rc = CL_OK;
    ClUint32T erroneousVectorIndex;
    
    AMS_CHECKPTR( !ams);
    AMS_CHECKPTR( !pSection);
    AMS_CHECKPTR( !pIOVector);

    /*
     * Read the AMS ckpt version section
     */
    pIOVector->sectionId.idLen = strlen(pSection->value);
    rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
    pIOVector->sectionId.id=(ClUint8T *)clHeapAllocate(pIOVector->sectionId.idLen);
    if(pIOVector->sectionId.id == NULL)
    {
        goto error;
    }
    memcpy(pIOVector->sectionId.id,(ClUint8T*)pSection->value,pIOVector->sectionId.idLen);
    if ( ( rc = clCkptCheckpointRead(
                                     ams->ckptOpenHandle,
                                     pIOVector,
                                     1,
                                     &erroneousVectorIndex)) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,("AMS Ckpt Read Error: Reading checkpoint for section [%s],rc [0x%x]\n",pSection->value,rc));
        clAmsFreeMemory(pIOVector->sectionId.id);
        goto error;
    }

    clAmsFreeMemory(pIOVector->sectionId.id);

    error:
    return rc;
}

static void amsCkptDifferenceVectorKeyGet(ClDifferenceVectorKeyT *key, ClNameT *pSection)
{
    key->groupKey = clHeapCalloc(1, sizeof(*key->groupKey));
    CL_ASSERT(key->groupKey != NULL);
    key->sectionKey = clHeapCalloc(1, sizeof(*key->sectionKey));
    CL_ASSERT(key->sectionKey != NULL);
    key->groupKey->pValue = clStrdup("AMS_CKPT");
    CL_ASSERT(key->groupKey->pValue != NULL);
    key->groupKey->length = strlen(key->groupKey->pValue);
    key->sectionKey->pValue = clStrdup((const ClCharT*)pSection->value);
    CL_ASSERT(key->sectionKey->pValue != NULL);
    key->sectionKey->length = strlen(key->sectionKey->pValue);
}

static ClRcT clAmsCkptSectionOverwriteNoLock(ClCkptHdlT ckptHandle,
                                             ClNameT *pSection,
                                             ClUint8T *pData,
                                             ClUint32T dataLen,
                                             ClUint32T mode
                                             )
{
    ClCkptSectionCreationAttributesT sectionAttribs;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR(!pSection);
    AMS_CHECKPTR(!pData);

    memset(&sectionAttribs,0,sizeof(sectionAttribs));
    rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
    sectionAttribs.sectionId=(ClCkptSectionIdT*)clHeapAllocate(sizeof(ClCkptSectionIdT));
    if(sectionAttribs.sectionId == NULL)
    {
        goto error;
    }
    sectionAttribs.sectionId->idLen = strlen(pSection->value);
    sectionAttribs.sectionId->id=(ClUint8T *) clHeapAllocate(sectionAttribs.sectionId->idLen);
    if(sectionAttribs.sectionId->id == NULL)
    {
        clAmsFreeMemory(sectionAttribs.sectionId);
        goto error;
    }
    memcpy(sectionAttribs.sectionId->id,(ClUint8T*)pSection->value,sectionAttribs.sectionId->idLen);

    rc = clCkptSectionOverwriteLinear(
                                      ckptHandle,
                                      sectionAttribs.sectionId,
                                      pData,
                                      dataLen);

    if(rc != CL_OK )
    { 
        AMS_LOG(CL_DEBUG_ERROR,("AMS Ckpt section overwrite failed for Section [%s] with error [0x%x]\n",
                                pSection->value, rc));
        goto out_free;
    }

    out_free:
    clAmsFreeMemory(sectionAttribs.sectionId->id);
    clAmsFreeMemory(sectionAttribs.sectionId);

    error:
    return rc;
}

static ClRcT clAmsCkptSectionOverwrite(ClAmsT *ams,
                                       ClNameT *pSection,
                                       ClDifferenceVectorKeyT *key,
                                       ClUint8T *pData,
                                       ClUint32T dataLen,
                                       ClUint32T mode
                                       )
{
    ClCkptSectionCreationAttributesT sectionAttribs;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR(!ams);
    AMS_CHECKPTR(!pSection);
    AMS_CHECKPTR(!pData);

    memset(&sectionAttribs,0,sizeof(sectionAttribs));
    rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
    sectionAttribs.sectionId=(ClCkptSectionIdT*)clHeapAllocate(sizeof(ClCkptSectionIdT));
    if(sectionAttribs.sectionId == NULL)
    {
        goto error;
    }
    sectionAttribs.sectionId->idLen = strlen(pSection->value);
    sectionAttribs.sectionId->id=(ClUint8T *) clHeapAllocate(sectionAttribs.sectionId->idLen);
    if(sectionAttribs.sectionId->id == NULL)
    {
        clAmsFreeMemory(sectionAttribs.sectionId);
        goto error;
    }
    memcpy(sectionAttribs.sectionId->id,(ClUint8T*)pSection->value,sectionAttribs.sectionId->idLen);

    if(mode == CL_AMS_CKPT_WRITE_DB && gClAmsCkptDifferential && key)
    {
        static ClBoolT differenceVectorKeyDeleted = CL_TRUE;
        ClUint32T versionCode = CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE);
        clNodeCacheMinVersionGet(NULL, &versionCode);
        if(versionCode >= CL_VERSION_CODE(5, 1, 0))
        {
                ClDifferenceVectorT differenceVector = {0};
                clDifferenceVectorGetWithReset(key, pData, 0, dataLen, CL_FALSE, &differenceVector);
                if(differenceVectorKeyDeleted) 
                    differenceVectorKeyDeleted = CL_FALSE;
                /*
                 *Send incase there is any difference to checkpoint.
                 */
                rc = CL_OK;
                if(differenceVector.numDataVectors > 0)
                {
                    rc = clCkptSectionOverwriteVector(
                                                      ams->ckptOpenHandle,
                                                      sectionAttribs.sectionId,
                                                      dataLen,
                                                      &differenceVector);
                }
                clDifferenceVectorFree(&differenceVector, CL_FALSE);
        }
        else
        {
            /*
             * Fallback to full section overwrite.
             */
            if(!differenceVectorKeyDeleted)
            {
                clDifferenceVectorDelete(key);
                differenceVectorKeyDeleted = CL_TRUE;
            }
            rc = clCkptSectionOverwriteLinear(ams->ckptOpenHandle,
                                              sectionAttribs.sectionId,
                                              pData,
                                              dataLen);
        }
    }
    else
    {
        rc = clCkptSectionOverwriteLinear(
                                          ams->ckptOpenHandle,
                                          sectionAttribs.sectionId,
                                          pData,
                                          dataLen);
    }

    if(rc != CL_OK )
    { 
        AMS_LOG(CL_DEBUG_ERROR,("AMS Ckpt section overwrite failed for Section [%s] with error [0x%x]\n",
                                pSection->value, rc));
        goto out_free;
    }

    out_free:
    clAmsFreeMemory(sectionAttribs.sectionId->id);
    clAmsFreeMemory(sectionAttribs.sectionId);

    error:
    return rc;
}

static ClRcT clAmsCkptSectionCreate(ClAmsT  *ams,
                                    ClNameT *pSection,
                                    ClCharT *pData,
                                    ClUint32T dataLen
                                    )
{                                    
    ClCkptSectionCreationAttributesT sectionAttribs;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR(!ams);
    AMS_CHECKPTR(!pSection);
    AMS_CHECKPTR(!pData);
    
    memset(&sectionAttribs,0,sizeof(sectionAttribs));
    sectionAttribs.sectionId=(ClCkptSectionIdT*)clHeapAllocate(sizeof(ClCkptSectionIdT));
    rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
    if(sectionAttribs.sectionId == NULL)
    {
        goto error;
    }
    sectionAttribs.sectionId->idLen = strlen (pSection->value);
    sectionAttribs.sectionId->id=(ClUint8T *) clHeapAllocate(sectionAttribs.sectionId->idLen);
    if(sectionAttribs.sectionId->id == NULL)
    {
        clAmsFreeMemory(sectionAttribs.sectionId);
        goto error;
    }
    memcpy(sectionAttribs.sectionId->id,(ClUint8T*)pSection->value,sectionAttribs.sectionId->idLen);
    sectionAttribs.expirationTime = CL_TIME_END;
    if ( ( rc = clCkptSectionCreate(
                                    ams->ckptOpenHandle,
                                    &sectionAttribs,
                                    (ClUint8T *)pData,
                                    dataLen)) != CL_OK)
    {
        clAmsFreeMemory(sectionAttribs.sectionId->id);
        clAmsFreeMemory(sectionAttribs.sectionId);
        goto error;
    }
    clAmsFreeMemory(sectionAttribs.sectionId->id);
    clAmsFreeMemory(sectionAttribs.sectionId);

    error:
    return rc;
}

static ClRcT clAmsCkptSectionDelete(ClAmsT *ams,
                                    ClNameT *pSection
                                    )
                
{
    ClRcT rc = CL_OK;
    ClCkptSectionIdT  sectionId;

    memset(&sectionId,0,sizeof(sectionId));
    sectionId.idLen = strlen (pSection->value);

    rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
    sectionId.id=(ClUint8T *) clHeapAllocate(sectionId.idLen);
    if(sectionId.id == NULL)
    {
        goto error;
    }
    memcpy(sectionId.id,(ClUint8T*)pSection->value,sectionId.idLen);
    if((rc = clCkptSectionDelete(ams->ckptOpenHandle, &sectionId)) != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR,("AMS Ckpt Delete:Failed to delete section [%s]\n",pSection->value));
        clAmsFreeMemory(sectionId.id);
        goto error;
    }

    clAmsFreeMemory(sectionId.id);
    error:
    return rc;
}

static ClRcT clAmsCkptDBDatasetInitialize(void)
{
    ClRcT rc = CL_OK;

    rc = clCkptLibraryCkptCreate( gClAmsCkptDBHdl, &gClAmsCkptDBName);
    
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Ckpt library create returned [%#x] "\
                                 "AMS configuration persistency disabled\n", rc));
        goto out;
    }
    
    /*
     * Create the dataset to store AMS configuration.
     */

    rc = clCkptLibraryCkptDataSetCreate(gClAmsCkptDBHdl, &gClAmsCkptDBName, CL_AMS_CKPT_CONFIG_DS_ID,
                                        CL_AMS_CKPT_CONFIG_GRP_ID, CL_AMS_CKPT_CONFIG_ORDER_ID,
                                        clAmsCkptDBConfigSerialize,
                                        clAmsCkptDBConfigDeserialize);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Ckpt dataset create returned [%#x] "\
                                 "AMS configuration persistency disabled\n", rc));
        clCkptLibraryCkptDelete(gClAmsCkptDBHdl, &gClAmsCkptDBName);
        goto out;
    }
    
    gClAmsCkptDBDatasetInitialized = CL_TRUE;

    out:
    return rc;
}

static ClRcT clAmsCkptDBInitialize(void)
{
    ClRcT rc = CL_OK;
    ClCharT dbPath[CL_MAX_NAME_LENGTH+1];
    ClCharT ckptCtrlDBName[CL_MAX_NAME_LENGTH+1];
    ClInt32T err = 0;
    
    gClAmsPersistentDBDisabled = (ClInt32T)clParseEnvBoolean("CL_AMF_PERSISTENT_DB_DISABLED");
    
    if(gClAmsPersistentDBDisabled)
    {
        clLogNotice("PERSISTENT", "DB", "AMF persistent db disabled");
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    if(gClAmsCkptDBInitialized == CL_TRUE) return CL_OK;

    memset(dbPath, 0, sizeof(dbPath));

    snprintf(dbPath, sizeof(dbPath), "%s/%s", 
             getenv("ASP_DBDIR") ? getenv("ASP_DBDIR") : ".",
             getenv("ASP_DBDIR") ? "ams" : ".");

    if( (err = mkdir(dbPath, 0755)) < 0 && errno != EEXIST)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Unable to create db directory [%s]. Creation error [%s]\n",
                                 dbPath, strerror(errno)));
        rc = CL_AMS_RC(CL_ERR_LIBRARY);
        goto out;
    }

    snprintf(ckptCtrlDBName, sizeof(ckptCtrlDBName), "%s/%s",
             dbPath, CL_AMS_CKPT_CTRL_DB_NAME);
             
    /*
     * Initialize file based checkpointing variables.
     */
    rc = clCkptLibraryInitializeDB(&gClAmsCkptDBHdl, ckptCtrlDBName);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Ckpt library initialize returned [%#x]. "\
                                 "AMS configuration persistency disabled\n", rc));
        goto out;
    }

    snprintf(gClAmsCkptDBName.value, sizeof(gClAmsCkptDBName.value),
             "%s/%s", dbPath, CL_AMS_CKPT_DB_NAME);

    gClAmsCkptDBName.length = strlen(gClAmsCkptDBName.value);

    gClAmsCkptDBInitialized = CL_TRUE;

    out:
    return rc;
}

static ClRcT 
clAmsCkptDBConfigSerialize(ClUint32T dsId, ClAddrT *pData, ClUint32T *pDataLength, ClPtrT cookie)
{
    ClBufferHandleT msgHdl = CL_HANDLE_INVALID_VALUE;
    ClRcT rc = CL_OK;

    if(!pData || !pDataLength) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(dsId != CL_AMS_CKPT_CONFIG_DS_ID)
    {
        AMS_LOG(CL_DEBUG_CRITICAL, ("Ckpt config serialize invoked with invalid data set id [%d]\n", dsId));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clBufferCreate(&msgHdl);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Buffer create in ckpt config serialize returned [%#x]\n", rc));
        goto out;
    }

    rc = clAmsDBConfigSerialize(&gAms.db, msgHdl);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("DB config serialize returned [%#x]\n", rc));
        goto out_free;
    }
    rc = clBufferLengthGet(msgHdl, pDataLength);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Buffer length get in ckpt config serialize returned [%#x]\n", rc));
        goto out_free;
    }
    
    rc = clBufferFlatten(msgHdl, (ClUint8T**)pData);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Buffer flatten in ckpt config serialize returned [%#x]\n", rc));
        goto out_free;
    }

    AMS_LOG(CL_DEBUG_INFO, ("AMS ckpt config [%d] bytes written to [%s]\n", 
                            *pDataLength, CL_AMS_CKPT_DB_NAME));

    out_free:
    clBufferDelete(&msgHdl);
    
    out:
    return rc;
}

static ClRcT
clAmsCkptDBConfigDeserialize(ClUint32T dsId, ClAddrT pData, ClUint32T dataLength, ClPtrT cookie)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msgHdl = CL_HANDLE_INVALID_VALUE;

    if(!pData || !dataLength) return CL_OK;

    AMS_LOG(CL_DEBUG_INFO, ("AMS ckpt loading [%d] bytes of config from [%s]\n", dataLength, CL_AMS_CKPT_DB_NAME));

    rc = clBufferCreate(&msgHdl);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Buffer create in ckpt config deserialize returned [%#x]\n", rc));
        goto out;
    }

    rc = clBufferNBytesWrite(msgHdl, (ClUint8T*)pData, dataLength);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Buffer write in ckpt config deserialize returned [%#x]\n", rc));
        goto out_free;
    }

    rc = clAmsDBConfigDeserialize(msgHdl);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS DB deserialize returned [%#x]\n", rc));
        goto out_free;
    }

    out_free:
    clBufferDelete(&msgHdl);

    out:
    return rc;
}


ClRcT
clAmsCkptDBWrite(void)
{
    ClRcT rc = CL_OK;
    static ClRcT dbInitialized = CL_OK;
    static ClRcT datasetInitialized = CL_OK;

    if(gClAmsPersistentDBDisabled == -1)
    {
        gClAmsPersistentDBDisabled = (ClInt32T)clParseEnvBoolean("CL_AMF_PERSISTENT_DB_DISABLED");
    }

    if(gClAmsPersistentDBDisabled)
    {
        return CL_OK;
    }

    if(gClAmsCkptDBInitialized == CL_FALSE)
    {
        if( (dbInitialized == CL_OK) && 
            (rc = clAmsCkptDBInitialize()) != CL_OK)
        {
            dbInitialized = rc;
        }
        if(dbInitialized != CL_OK)
            return dbInitialized;
    }
    if(gClAmsCkptDBDatasetInitialized == CL_FALSE)
    {
        if( (datasetInitialized == CL_OK) && 
            (rc = clAmsCkptDBDatasetInitialize() ) != CL_OK)
        {
            datasetInitialized = rc;
        }
        if(datasetInitialized != CL_OK)
            return datasetInitialized;
    }
    return clCkptLibraryCkptDataSetWrite(gClAmsCkptDBHdl, &gClAmsCkptDBName, CL_AMS_CKPT_CONFIG_DS_ID, NULL);
}

ClRcT
clAmsCkptDBRead(void)
{
    ClRcT rc = CL_OK;
    ClBoolT present = CL_FALSE;

    rc = clAmsCkptDBInitialize();
    if(rc != CL_OK)
    {
        goto out;
    }
    rc = clCkptLibraryDoesCkptExist(gClAmsCkptDBHdl, &gClAmsCkptDBName, &present);
    if(rc != CL_OK)
    {
        goto out;
    }
    if(present == CL_FALSE)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out;
    }
    present = CL_FALSE;
    rc = clCkptLibraryDoesDatasetExist(gClAmsCkptDBHdl, &gClAmsCkptDBName,
                                       CL_AMS_CKPT_CONFIG_DS_ID, &present);
    if(rc != CL_OK)
    {
        goto out;
    }

    if(present == CL_FALSE)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out;
    }

    if( (rc = clAmsCkptDBDatasetInitialize() ) != CL_OK)
    {
        goto out;
    }
    
    /*
     * Now read the AMS config dataset and load the db.
     */
    rc = clCkptLibraryCkptDataSetRead(gClAmsCkptDBHdl, &gClAmsCkptDBName, CL_AMS_CKPT_CONFIG_DS_ID, NULL);
    if(rc != CL_OK)
    {
		ClRcT ret;
        clAmsDbTerminate(&gAms.db);
        AMS_LOG(CL_DEBUG_ERROR, ("AMS config data set read returned [%#x]\n", rc));
        if ((ret = clAmsDbInstantiate(&gAms.db)) != CL_OK) 
            clLogError("CKPT", "NOTIFY", "AMS db instantiate returned [%#x]", ret);
        goto out;
    }

    AMS_LOG(CL_DEBUG_INFO, ("AMS config loaded from DB [%s]\n", CL_AMS_CKPT_DB_NAME));

    out:
    return rc;
}

static ClRcT clAmsCkptDBReadVersion(ClAmsT *ams, ClCharT *versionBuf, ClUint32T versionSize)
{
    ClCkptIOVectorElementT iovector;
    ClRcT rc = CL_OK;
    memset(&iovector, 0, sizeof(iovector));
    iovector.dataSize = AMS_CKPT_MAX_SECTION_SIZE;
    rc = clAmsCkptCheckpointRead(ams, &ams->ckptVersionSection, &iovector);
    if(rc != CL_OK)
    {
        clLogError("CKPT", "READ", "Unable to read ams ckpt version. Fatal error [%#x]", rc);
        return rc;
    }
    strncpy(versionBuf, (const ClCharT*)iovector.dataBuffer, CL_MIN(versionSize, iovector.dataSize));
    versionBuf[CL_MIN(versionSize, iovector.dataSize)] = 0;
    clAmsFreeMemory(iovector.dataBuffer);
    return CL_OK;
}

static ClRcT 
amsCkptStandbyInitialize(ClAmsT *ams)
{
    ClRcT rc = CL_OK;

    /*
     * Mark ourselves as ready if applicable
     */
    if(!ams->isEnabled)
    {
        clLogNotice("STANDBY", "ENABLE", "Enabling AMF whose service was disabled on standby");
        ams->isEnabled = CL_TRUE;
    }

    /*
     * Do a ckpt db write on failover to advance the reference for full ckpt
     */
    ams->mode |= CL_AMS_INSTANTIATE_MODE_CKPT_ALL; 

    AMS_CHECK_RC_ERROR(clCkptImmediateConsumptionRegister(ams->ckptOpenHandle, 
                                                          clAmsCkptNotifyCallback, NULL) );

    exitfn:
    return rc;
}

ClRcT
clAmsCkptInitialize(
                    CL_INOUT  ClAmsT  *ams, 
                    CL_IN  ClCkptHdlT  ckptInitHandle, 
                    CL_IN  ClUint32T  mode )
{
    ClRcT  rc = CL_OK ;
    ClCkptHdlT  ckptOpenHandle = -1;
    ClNameT  ckptName = {0};
    const ClTimeT  timeout = CL_TIME_END;
    const ClCkptOpenFlagsT  flags = 
        CL_CKPT_CHECKPOINT_CREATE|CL_CKPT_CHECKPOINT_WRITE|CL_CKPT_CHECKPOINT_READ;
    ClCkptCheckpointCreationAttributesT  ckptAttributes = 
        {
            CL_CKPT_DISTRIBUTED  | CL_CKPT_PEER_TO_PEER_REPLICA,
            AMS_CKPT_SIZE,
            AMS_CKPT_RETENTION_DURATION,
            AMS_CKPT_MAX_SECTIONS, 
            AMS_CKPT_MAX_SECTION_SIZE,
            AMS_CKPT_MAX_SECTION_ID_SIZE
        };
    ClInt32T i;
    ClCharT *freq;
    gClAmsCkptDifferential = clCkptDifferentialCheckpointStatusGet();
    if( (freq = getenv("CL_AMF_CKPT_FREQUENCY") ) )
    {
        gClAmsCkptFrequency = atoi(freq);
    }
#ifdef QNX_BUILD
    if(!gClAmsCkptFrequency)
    {
        gClAmsCkptFrequency = 3; /* set it to 3 seconds for qnx */
    }
#endif
    rc = clJobQueueInit(&gClAmsCkptJobQueue, 0, 1);
    if(rc != CL_OK)
        return rc;

    if( clParseEnvBoolean("CL_AMF_CKPT_ASYNC") )
    {
        clLogNotice("CKPT", "INIT", "Setting up AMF checkpointing in async distributed mode");
        ckptAttributes.creationFlags &= ~CL_CKPT_WR_ALL_REPLICAS;
    }

    memset (&ckptName,0,sizeof (ClNameT));
    strcpy (ckptName.value,CL_AMS_CKPT_NAME);
    ckptName.length = strlen (CL_AMS_CKPT_NAME) ;
    memcpy (&ams->ckptName, &ckptName, sizeof (ClNameT));

    for(i = 0; i < 2; ++i)
    {
        ClCharT buf[sizeof(ClNameT)];
        snprintf(buf,sizeof(buf),"%s_%d",AMS_CKPT_DB_SECTION,i+1);
        memset (&ams->ckptDBSections[i],0,sizeof (ClNameT));
        strncpy(ams->ckptDBSections[i].value,buf,
                sizeof(ams->ckptDBSections[i].value)-1);
        ams->ckptDBSections[i].length = strlen(buf)+1;
        memset(&ams->ckptDirtySections[i], 0, sizeof(ClNameT));
        snprintf(buf, sizeof(buf), "%s_%d", AMS_CKPT_DIRTY_SECTION, i+1);
        strncpy(ams->ckptDirtySections[i].value, buf, sizeof(ams->ckptDirtySections[i].value)-1);
        ams->ckptDirtySections[i].length = strlen(buf)+1;
        memset (&ams->ckptInvocationSections[i],0,sizeof (ClNameT));
        snprintf(buf,sizeof(buf),"%s_%d",AMS_CKPT_INVOCATION_SECTION,i+1);
        strncpy (ams->ckptInvocationSections[i].value,buf,
                 sizeof(ams->ckptInvocationSections[i].value)-1);
        ams->ckptInvocationSections[i].length = strlen(buf)+1;
        memcpy(&gClAmsCkptDBSectionCache[i], &ams->ckptDBSections[i], 
               sizeof(gClAmsCkptDBSectionCache[i]));
        memcpy(&gClAmsCkptDirtySectionCache[i], &ams->ckptDirtySections[i],
               sizeof(gClAmsCkptDirtySectionCache[i]));
        memcpy(&gClAmsCkptInvocationSectionCache[i], &ams->ckptInvocationSections[i],
               sizeof(gClAmsCkptInvocationSectionCache[i]));
        amsCkptDifferenceVectorKeyGet(&ams->ckptDifferenceVectorKeys[i],
                                      &ams->ckptDBSections[i]);
    }

    /*Make the current section as the first DB INVOCATION PAIR*/
    memset(&ams->ckptCurrentSection,0,sizeof(ams->ckptCurrentSection));
    strncpy(ams->ckptCurrentSection.value,AMS_CKPT_CURRENT_SECTION,
            sizeof(ams->ckptCurrentSection.value)-1);
    ams->ckptCurrentSection.length = strlen(AMS_CKPT_CURRENT_SECTION) + 1;
    memcpy(&gClAmsCkptCurrentSectionCache, &ams->ckptCurrentSection, 
           sizeof(gClAmsCkptCurrentSectionCache));
    memset (&ams->ckptVersionSection,0,sizeof (ClNameT));
    strncpy (ams->ckptVersionSection.value, AMS_CKPT_VERSION_SECTION,
             sizeof(ams->ckptVersionSection.value)-1);
    ams->ckptVersionSection.length = strlen (AMS_CKPT_VERSION_SECTION) + 1;

    ams->ckptInitHandle = ckptInitHandle;
    AMS_CHECK_RC_ERROR ( clCkptCheckpointOpen(
                                              ckptInitHandle,
                                              &ckptName,
                                              &ckptAttributes,
                                              flags,
                                              timeout,
                                              &ckptOpenHandle));
    
    ams->ckptOpenHandle = ckptOpenHandle;

    if ( (mode&CL_AMS_INSTANTIATE_MODE_ACTIVE ) )
    {
        ClCharT    *initialData = "SECTION-START";

        /*
         * Create the DB and Invocation Pairs first
         */
        for(i = 0; i < 2; ++i)
        {
            AMS_CHECK_RC_ERROR(
                               clAmsCkptSectionCreate(ams,
                                                      &ams->ckptDBSections[i],
                                                      initialData,
                                                      strlen(initialData)));

            AMS_CHECK_RC_ERROR(
                               clAmsCkptSectionCreate(ams,
                                                      &ams->ckptDirtySections[i],
                                                      initialData,
                                                      strlen(initialData)));

            AMS_CHECK_RC_ERROR(
                               clAmsCkptSectionCreate(ams,
                                                      &ams->ckptInvocationSections[i],
                                                      initialData,
                                                      strlen(initialData)));
        }

        /*
         * Create the AMS current active and version section
         * Current created with invocationPair 0 as active
         */
        AMS_CHECK_RC_ERROR(clAmsCkptSectionCreate(ams,
                                                  &ams->ckptCurrentSection,
                                                  (ClCharT*)&gClAmsCkptCurrentDbInvocationPair,
                                                  sizeof(gClAmsCkptCurrentDbInvocationPair)));
        
        AMS_CHECK_RC_ERROR(clAmsCkptSectionCreate(ams,
                                                  &ams->ckptVersionSection,
                                                  (ClCharT*)CL_AMS_CKPT_VERSION,
                                                  strlen(CL_AMS_CKPT_VERSION)));

        /*
         * Do a first time ckpt write.
         */
        clOsalMutexLock(gAms.mutex);
        ams->mode |= CL_AMS_INSTANTIATE_MODE_CKPT_ALL;
        clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_ALL);
        clOsalMutexUnlock(gAms.mutex);
    }
    else
    {
        clOsalMutexLock(gAms.mutex);
        clOsalMutexLock(&gAms.ckptMutex);
        rc = amsCkptStandbyInitialize(&gAms);
        clOsalMutexUnlock(&gAms.ckptMutex);
        clOsalMutexUnlock(gAms.mutex);
        if(rc != CL_OK)
        {
            clLogWarning("CKP", "INIT", "AMF hot standby disabled because of standby initialize failure");
            rc = CL_OK;
        }
    }

    return rc;

    exitfn:
    return CL_AMS_RC(rc);
}

static ClRcT
clAmsCkptNotifyCallback(ClCkptHdlT              ckptHdl,
                        ClNameT                 *pName,
                        ClCkptIOVectorElementT  *pIOVector,
                        ClUint32T               numSections,
                        ClPtrT                  pCookie)
{
    ClBufferHandleT msgHandle = 0;
    ClRcT rc = CL_OK;
    ClCharT *pSectionName = NULL;
    ClUint32T dbMode = 0; 

    pSectionName = clHeapCalloc(1, pIOVector->sectionId.idLen + 1);
    if(!pSectionName) 
    {
        clLogError("CKPT", "NOTIFY", "Section malloc error");
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    }
    memcpy(pSectionName, pIOVector->sectionId.id, pIOVector->sectionId.idLen);
    pSectionName[pIOVector->sectionId.idLen] = 0;
    
    clOsalMutexLock(gAms.mutex);
    clOsalMutexLock(&gAms.ckptMutex);
    /*
     * Delete last snapshot of the DB and the invocation before loading the new one.
     */
    if(!gAms.isEnabled
       ||
       (
        gAms.serviceState != CL_AMS_SERVICE_STATE_HOT_STANDBY
        &&
        gAms.serviceState != CL_AMS_SERVICE_STATE_UNAVAILABLE))
    {
        clOsalMutexUnlock(&gAms.ckptMutex);
        clOsalMutexUnlock(gAms.mutex);
        clHeapFree(pSectionName);
        return rc;
    }
    
    clLogInfo("CKPT", "NOTIFY", 
              "Got CKPT notify callback for section [%s], of size [%lld] bytes", 
              pSectionName,
              pIOVector->dataSize);

    if(!gClAmsCkptVersionBuf[0])
    {
        rc = clAmsCkptDBReadVersion(&gAms, gClAmsCkptVersionBuf, sizeof(gClAmsCkptVersionBuf)-1);
        if(rc != CL_OK)
        {
            goto out_unlock;
        }
        clLogNotice("CKPT", "NOTIFY", "AMS DB ckpt version [%s]", gClAmsCkptVersionBuf);
    }

    if(!strncmp(pSectionName, AMS_CKPT_DB_SECTION, sizeof(AMS_CKPT_DB_SECTION)-1))
    {
        dbMode = 1;
    }
    else if(!strncmp(pSectionName, AMS_CKPT_DIRTY_SECTION, sizeof(AMS_CKPT_DIRTY_SECTION)-1))
    {
        dbMode = 2;
        if(gAms.serviceState != CL_AMS_SERVICE_STATE_HOT_STANDBY)
        {
            clLogInfo("CKP", "DIRTY", "Ignoring hot standby update till full update is received");
            goto out_unlock;
        }
    }

    if(dbMode)
    {
        rc = clBufferCreate(&msgHandle);
        if(rc != CL_OK)
        {
            clLogError("CKPT", "NOTIFY", "Buffer create returned [%#x]", rc);
            goto out_unlock;
        }

        rc = clBufferNBytesWrite(msgHandle, (ClUint8T*)pIOVector->dataBuffer, 
                                 pIOVector->dataSize);
        if(rc != CL_OK)
        {
            clLogError("CKPT", "NOTIFY", "Buffer write returned [%#x]", rc);
            goto out_unlock;
        }

        if(dbMode == 1)
        {
            clAmsDbTerminate(&gAms.db);

            if( (rc = clAmsDbInstantiate(&gAms.db) ) != CL_OK)
            {
                clLogError("CKPT", "NOTIFY", "AMS db instantiate returned [%#x]", rc);
                goto out_unlock;
            }
        }

        rc = clAmsDBUnmarshall(msgHandle);

        if(rc != CL_OK)
        {
            clLogError("CKPT", "NOTIFY", "AMS db unmarshall returned [%#x] for section [%s]",
                       rc, pSectionName);
            goto out_unlock;
        }
        else
        {
            clLogDebug("CKPT", "NOTIFY", "AMS db unmarshall success for section [%s], "
                       "bytes [%d], mode [%d]",
                       pSectionName, (ClUint32T)pIOVector->dataSize, dbMode);
        }

        /*
         * Update the state now that we have read the checkpoint.
         */
        if(gAms.serviceState != CL_AMS_SERVICE_STATE_HOT_STANDBY)
            gAms.serviceState = CL_AMS_SERVICE_STATE_HOT_STANDBY;

        /*
         * Update stale csi pending invocations.
         */
        if(dbMode == 1)
        {
            clAmsInvocationListUpdateCSIAll(CL_TRUE);
        }

        /*
         * Update the local db for persistency.
         */
        clAmsCkptDBWrite();
    }
    else if(!strncmp(pSectionName,
                     AMS_CKPT_INVOCATION_SECTION, strlen(AMS_CKPT_INVOCATION_SECTION)))
    {
        clAmsInvocationListTerminate(&gAms.invocationList);
        
        if( (rc = clAmsInvocationListInstantiate(&gAms.invocationList) ) != CL_OK)
        {
            clLogError("CKPT", "NOTIFY", "AMS invocation instantiate returned [%#x]", rc);
            goto out_unlock;
        }
        /*
         * Old versions used xmlized invocations.
         */
        if(!strncmp(gClAmsCkptVersionBuf, "B.01.01", 7))
        {
            rc = clAmsWriteXMLFile(pSectionName,
                                   (ClCharT*)pIOVector->dataBuffer,
                                   pIOVector->dataSize);
            if(rc != CL_OK)
            {
                clLogError("CKPT", "NOTIFY", "AMS invocation write xml returned [%#x]", rc);
                goto out_unlock;
            }

            rc = clAmsDeXMLizeInvocation(&gAms, pSectionName);
        }
        else
        {
            rc = clBufferCreate(&msgHandle);
            if(rc != CL_OK)
            {
                clLogError("CKPT", "NOTIFY", "Buffer create returned [%#x] for invocation", rc);
                goto out_unlock;
            }

            rc = clBufferNBytesWrite(msgHandle, (ClUint8T*)pIOVector->dataBuffer, 
                                     pIOVector->dataSize);
            if(rc != CL_OK)
            {
                clLogError("CKPT", "NOTIFY", "Buffer write returned [%#x] for invocation", rc);
                goto out_unlock;
            }
            rc = clAmsInvocationUnmarshall(&gAms, pSectionName, msgHandle);
        }
        if(rc != CL_OK)
        {
            clLogError("CKPT", "NOTIFY", "AMS invocation dexmlize returned [%#x]", rc);
            goto out_unlock;
        }
    }
    else
    {
        /*ignore*/
    }

    out_unlock:
    clOsalMutexUnlock(&gAms.ckptMutex);
    clOsalMutexUnlock(gAms.mutex);

    if(msgHandle)
        clBufferDelete(&msgHandle);

    clHeapFree(pSectionName);

    return rc;
}

ClRcT
clAmsHotStandbyRegister(ClAmsT *ams)
{
    if(ams->serviceState == CL_AMS_SERVICE_STATE_UNAVAILABLE)
    {
        clCkptImmediateConsumptionRegister(ams->ckptOpenHandle, clAmsCkptNotifyCallback, NULL);
    }
    return CL_OK;
}

ClRcT
clAmsCkptRead ( 
               CL_INOUT  ClAmsT  *ams )
{ 
    ClRcT  rc = CL_OK;
    ClCkptIOVectorElementT   ioVector;
    ClUint32T dbInvocationPair = 0;
    ClPtrT dataBuffer = NULL, dirtyDataBuffer = NULL;
    ClPtrT invocationBuffer = NULL;
    ClSizeT invocationSize = 0;
    ClSizeT dataSize = 0, dirtyDataSize = 0;
    ClBufferHandleT dataBuf = 0;
    ClBoolT xmlize = CL_FALSE;
    ClBoolT dirtySection = CL_FALSE;

    if(!gClAmsCkptVersionBuf[0])
    {
        rc = clAmsCkptDBReadVersion(ams, gClAmsCkptVersionBuf, sizeof(gClAmsCkptVersionBuf)-1);
        if(rc != CL_OK)
        {
            /*
             * Hope that the invocation is a new one instead of bailing out to see if it works.
             */
            strncpy(gClAmsCkptVersionBuf, CL_AMS_CKPT_VERSION, sizeof(gClAmsCkptVersionBuf)-1);
        }
    }

    if(!strncmp(gClAmsCkptVersionBuf, CL_AMS_CKPT_VERSION, 
                sizeof(CL_AMS_CKPT_VERSION)-1))
    {
        dirtySection = CL_TRUE;
    }

    /*
     * Read the AMS ckpt version section
     */
    memset(&ioVector,0,sizeof(ioVector));

    ioVector.dataSize=AMS_CKPT_MAX_SECTION_SIZE;
    ioVector.dataOffset=0; 

    /*
     * Read the AMS Current Active DBInvocationPair
     */
    AMS_CHECK_RC_ERROR(clAmsCkptReadCurrentDBInvocationPair(ams,&dbInvocationPair) );

    memset(&ioVector,0,sizeof(ioVector));
    ioVector.dataSize=AMS_CKPT_MAX_SECTION_SIZE;
    ioVector.dataOffset=0; 

    if(dirtySection)
    {
        AMS_CHECK_RC_ERROR(clAmsCkptCheckpointRead(
                                                   ams,
                                                   &ams->ckptDBSections[0],
                                                   &ioVector));
    }
    else
    {
        AMS_CHECK_RC_ERROR(clAmsCkptCheckpointRead(
                                                   ams,
                                                   &ams->ckptDBSections[dbInvocationPair],
                                                   &ioVector));
    }

    dataBuffer = ioVector.dataBuffer;
    dataSize = ioVector.dataSize;
    ioVector.dataBuffer = NULL;
    /*
     * Now read the invocation section
     */
    memset(&ioVector,0,sizeof(ioVector));
    ioVector.dataSize=AMS_CKPT_MAX_SECTION_SIZE;
    ioVector.dataOffset=0; 
    
    AMS_CHECK_RC_ERROR(clAmsCkptCheckpointRead(
                                               ams,
                                               &ams->ckptInvocationSections[dbInvocationPair],
                                               &ioVector));

    if(!strncmp(gClAmsCkptVersionBuf, "B.01.01", 7))
    {
        if ( ( rc = clAmsWriteXMLFile(
                                      ams->ckptInvocationSections[dbInvocationPair].value,
                                      (char *)ioVector.dataBuffer,
                                      ioVector.dataSize))
             != CL_OK )
        {
            clAmsFreeMemory(ioVector.dataBuffer);
            goto exitfn;
        }

        clAmsFreeMemory(ioVector.dataBuffer);
        xmlize = CL_TRUE;
    }
    else
    {
        invocationBuffer = ioVector.dataBuffer;
        invocationSize = ioVector.dataSize;
        ioVector.dataBuffer = NULL;
    }

    /*
     * Read the dirty section
     */
    if(dirtySection)
    {
        memset(&ioVector, 0, sizeof(ioVector));
        ioVector.dataSize = AMS_CKPT_MAX_SECTION_SIZE;
        AMS_CHECK_RC_ERROR(clAmsCkptCheckpointRead(
                                                   ams,
                                                   &ams->ckptDirtySections[dbInvocationPair],
                                                   &ioVector));
        dirtyDataBuffer = ioVector.dataBuffer;
        dirtyDataSize = ioVector.dataSize;
        memset(&ioVector, 0, sizeof(ioVector));
    }

    /*
     * Contruct the database and invocation list
     */
    rc = clBufferCreate(&dataBuf);
    if(rc != CL_OK)
    {
        goto exitfn;
    }
    rc = clBufferNBytesWrite(dataBuf, (ClUint8T*)dataBuffer, (ClUint32T)dataSize);
    if(rc != CL_OK)
    {
        goto exitfn;
    }
    rc = clAmsDBUnmarshall(dataBuf);
    if(rc != CL_OK)
    {
        clLogError("CKP", "READ", "DB unmarshall for section [%s] contents failed with [%#x]."
                   "DB section data size [%d] bytes", AMS_CKPT_DB_SECTION, rc,
                   (ClUint32T)dataSize);
        goto exitfn;
    }

    if(dirtyDataBuffer && dirtyDataSize > 0)
    {
        clBufferClear(dataBuf);
        rc = clBufferNBytesWrite(dataBuf, (ClUint8T*)dirtyDataBuffer, 
                                 (ClUint32T)dirtyDataSize);
        if(rc != CL_OK)
        {
            goto exitfn;
        }
        /*
         * Unmarshall and overwrite the contents of dirty section.
         */
        rc = clAmsDBUnmarshall(dataBuf);
        if(rc != CL_OK)
        {
            clLogError("CKP", "READ", "DB unmarshall for section [%s] contents failed with [%#x]."
                       "Dirty section data size [%d] bytes", 
                       AMS_CKPT_DIRTY_SECTION, rc, (ClUint32T)dirtyDataSize);
            goto exitfn;
        }
    }

    if(invocationSize > 0 && invocationBuffer)
    {
        clBufferClear(dataBuf);
        rc = clBufferNBytesWrite(dataBuf, (ClUint8T*)invocationBuffer, invocationSize);
        if(rc != CL_OK)
            goto exitfn;
        rc = clAmsInvocationUnmarshall(ams, ams->ckptInvocationSections[dbInvocationPair].value, dataBuf);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Invocation unmarshall returned [%#x]\n", rc));
            goto exitfn;
        }
    }

    if(xmlize)
    {
        AMS_CHECK_RC_ERROR(clAmsDeXMLizeInvocation(
                                                   ams,
                                                   ams->ckptInvocationSections[dbInvocationPair].value));
    }

    exitfn:
    if(dataBuf)
        clBufferDelete(&dataBuf);
    clAmsFreeMemory(invocationBuffer);
    clAmsFreeMemory(dataBuffer);
    clAmsFreeMemory(dirtyDataBuffer);
    if(rc != CL_OK)
        return CL_AMS_RC (rc);
    return CL_OK;
}

static ClRcT
amsCkptWrite(ClAmsT *ams, ClUint32T mode )
{ 
    ClRcT   rc;
    ClCharT *readData = NULL;
    ClUint32T dbInvocationPair;
    ClBoolT dirty = CL_TRUE;
    ClNameT *dirtySection = NULL;

    if ( ams->ckptServerReady == CL_FALSE || 
         ams->serviceState == CL_AMS_SERVICE_STATE_UNAVAILABLE || 
         !ams->isEnabled)
    { 
        AMS_LOG (CL_DEBUG_TRACE,("Checkpoint server not ready\n")); 
        return CL_OK; 
    }

    /*
     * First get the current DB Invocation Pair to write to
     */
    if(mode == CL_AMS_CKPT_WRITE_ALL)
    {
        dbInvocationPair = CL_AMS_CKPT_GET_DB_INVOCATION_PAIR(dbInvocationPair);
    }
    else
    {
        /*
         * Avoid update-Use the last so that we retain the 
         * coupling between the pair
         */
        dbInvocationPair = gClAmsCkptCurrentDbInvocationPair;
    }

    if ( (mode == CL_AMS_CKPT_WRITE_DB) || (mode == CL_AMS_CKPT_WRITE_ALL) )
    {
        ClUint32T dataLen = 0;
        ClBufferHandleT dataBuf = 0;
        AMS_CHECK_RC_ERROR(clBufferCreate(&dataBuf));
        /*
         * Check for full ckpt write modes
         */
        dirtySection = &ams->ckptDirtySections[dbInvocationPair];
        if((ams->mode & CL_AMS_INSTANTIATE_MODE_CKPT_ALL))
        {
            dbInvocationPair = 0;
            dirty = CL_FALSE;
            dirtySection = &ams->ckptDBSections[dbInvocationPair];
            if(!(ams->mode & CL_AMS_INSTANTIATE_MODE_NODE_JOIN))
            {
                ams->mode &= ~CL_AMS_INSTANTIATE_MODE_CKPT_ALL;
            }
            clAmsResetDirtyList();
            rc = clAmsDBMarshall(&ams->db, dataBuf);
        }
        else
        {
            rc = clAmsDBMarshallDirty(&ams->db, dataBuf);
        }
        if(rc != CL_OK)
        {
            clBufferDelete(&dataBuf);
            AMS_LOG(CL_DEBUG_ERROR, ("DB marshall returned [%#x]\n", rc));
            goto exitfn;
        }
        clBufferLengthGet(dataBuf, &dataLen);
        clLogDebug("PACK", "CKPT", "DB %smarshall done for [%d] bytes\n", 
                   dirty ? "dirty ":"", dataLen);
        rc = clBufferFlatten(dataBuf, (ClUint8T**)&readData);
        if(rc != CL_OK)
        {
            clBufferDelete(&dataBuf);
            AMS_LOG(CL_DEBUG_ERROR, ("Buffer flatten returned [%#x]\n", rc));
            goto exitfn;
        }
        clBufferDelete(&dataBuf);
        if ( ( rc = clAmsCkptSectionOverwrite(
                                              ams,
                                              dirtySection,
                                              &ams->ckptDifferenceVectorKeys[dbInvocationPair],
                                              (ClUint8T *)readData,
                                              dataLen,
                                              CL_AMS_CKPT_WRITE_DB))
             != CL_OK )
        { 
            clHeapFree (readData);
            goto exitfn;
        }
        
        clHeapFree (readData);
        readData = NULL;
    }

    if ( (mode == CL_AMS_CKPT_WRITE_INVOCATION) || (mode == CL_AMS_CKPT_WRITE_ALL) )
    {
        ClBufferHandleT invocationBuf = 0;
        ClUint32T invocationLen = 0;

        AMS_CHECK_RC_ERROR(clBufferCreate(&invocationBuf));
        rc = clAmsInvocationMarshall(ams, ams->ckptInvocationSections[dbInvocationPair].value, 
                                     invocationBuf);
        if(rc != CL_OK)
        {
            clBufferDelete(&invocationBuf);
            goto exitfn;
        }

        clBufferLengthGet(invocationBuf, &invocationLen);
        AMS_LOG(CL_DEBUG_INFO, ("Invocation DB marshall done for [%d] bytes\n", invocationLen));
        rc = clBufferFlatten(invocationBuf, (ClUint8T**)&readData);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Invocation buffer flatten returned [%#x]\n", rc));
            clBufferDelete(&invocationBuf);
            goto exitfn;
        }
        clBufferDelete(&invocationBuf);
        if ( ( rc = clAmsCkptSectionOverwrite(
                                              ams,
                                              &ams->ckptInvocationSections[dbInvocationPair],
                                              NULL,
                                              (ClUint8T *)readData,
                                              invocationLen,
                                              CL_AMS_CKPT_WRITE_INVOCATION))
             != CL_OK )
        { 
            clHeapFree (readData);
            goto exitfn;
        }
        
        clHeapFree (readData);
        readData = NULL;
    }

    /*
     * Now sync the active invocation pair incase there is a shift.
     */
    if(gClAmsCkptLastDbInvocationPair != dbInvocationPair)
    {
        gClAmsCkptLastDbInvocationPair = dbInvocationPair;

        AMS_LOG(CL_DEBUG_TRACE,("AMS CKPT Section overwrite for [%s],dbInvocation pair [0x%x]\n",ams->ckptCurrentSection.value,dbInvocationPair));

        AMS_CHECK_RC_ERROR(clAmsCkptSectionOverwrite(
                                                     ams,
                                                     &ams->ckptCurrentSection,
                                                     NULL,
                                                     (ClUint8T*)&dbInvocationPair,
                                                     sizeof(dbInvocationPair),
                                                     CL_AMS_CKPT_WRITE_INVOCATION));
    }

    return CL_OK;

    exitfn:
    return CL_AMS_RC (rc);
}

static ClRcT
amsCkptWriteNoLock(ClAmsT *ams, ClUint32T mode )
{ 
    ClRcT   rc = CL_OK;
    ClCharT *ckptData = NULL;
    ClUint32T dataLen = 0, invocationLen = 0;
    ClCharT *invocationData = NULL;
    ClUint32T dbInvocationPair = 0;
    ClBoolT updateInvocationPair = CL_FALSE;
    ClBoolT dirty = CL_TRUE;
    ClNameT *dirtySection = NULL;
    ClCkptHdlT ckptHandle = 0;

    clOsalMutexLock(gAms.mutex);
    if ( ams->ckptServerReady == CL_FALSE || 
         ams->serviceState == CL_AMS_SERVICE_STATE_UNAVAILABLE || 
         !ams->isEnabled)
    { 
        clOsalMutexUnlock(gAms.mutex);
        AMS_LOG (CL_DEBUG_TRACE,("Checkpoint server not ready\n")); 
        return CL_OK; 
    }

    ckptHandle = ams->ckptOpenHandle;
    /*
     * First get the current DB Invocation Pair to write to
     */
    if(mode == CL_AMS_CKPT_WRITE_ALL)
    {
        dbInvocationPair = CL_AMS_CKPT_GET_DB_INVOCATION_PAIR(dbInvocationPair);
    }
    else
    {
        /*
         * Avoid update-Use the last so that we retain the 
         * coupling between the pair
         */
        dbInvocationPair = gClAmsCkptCurrentDbInvocationPair;
    }

    if ( (mode == CL_AMS_CKPT_WRITE_DB) || (mode == CL_AMS_CKPT_WRITE_ALL) )
    {
        ClBufferHandleT dataBuf = 0;
        rc = clBufferCreate(&dataBuf);
        if(rc != CL_OK)
        {
            goto out_unlock;
        }
        dirtySection = &gClAmsCkptDirtySectionCache[dbInvocationPair];
        if(ams->mode & CL_AMS_INSTANTIATE_MODE_CKPT_ALL)
        {
            dirty = CL_FALSE;
            dbInvocationPair = 0;
            dirtySection = &gClAmsCkptDBSectionCache[dbInvocationPair];
            if(!(ams->mode & CL_AMS_INSTANTIATE_MODE_NODE_JOIN))
            {
                ams->mode &= ~CL_AMS_INSTANTIATE_MODE_CKPT_ALL;
            }
            clAmsResetDirtyList();
            rc = clAmsDBMarshall(&ams->db, dataBuf);
        }
        else
        {
            rc = clAmsDBMarshallDirty(&ams->db, dataBuf);
        }
        if(rc != CL_OK)
        {
            clBufferDelete(&dataBuf);
            AMS_LOG(CL_DEBUG_ERROR, ("DB marshall returned [%#x]\n", rc));
            goto out_unlock;
        }
        clBufferLengthGet(dataBuf, &dataLen);
        AMS_LOG(CL_DEBUG_INFO, ("DB %smarshall done for [%d] bytes\n", 
                                dirty ? "dirty ":"", dataLen));
        rc = clBufferFlatten(dataBuf, (ClUint8T**)&ckptData);
        if(rc != CL_OK)
        {
            clBufferDelete(&dataBuf);
            AMS_LOG(CL_DEBUG_ERROR, ("Buffer flatten returned [%#x]\n", rc));
            goto out_unlock;
        }
        clBufferDelete(&dataBuf);
    }

    if ( (mode == CL_AMS_CKPT_WRITE_INVOCATION) || (mode == CL_AMS_CKPT_WRITE_ALL) )
    {
        ClBufferHandleT invocationBuf = 0;

        rc = clBufferCreate(&invocationBuf);
        if(rc != CL_OK)
            goto out_unlock;

        rc = clAmsInvocationMarshall(ams, ams->ckptInvocationSections[dbInvocationPair].value, 
                                     invocationBuf);
        if(rc != CL_OK)
        {
            clBufferDelete(&invocationBuf);
            goto out_unlock;
        }

        clBufferLengthGet(invocationBuf, &invocationLen);
        AMS_LOG(CL_DEBUG_INFO, ("Invocation DB marshall done for [%d] bytes\n", invocationLen));
        rc = clBufferFlatten(invocationBuf, (ClUint8T**)&invocationData);
        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Invocation buffer flatten returned [%#x]\n", rc));
            clBufferDelete(&invocationBuf);
            goto out_unlock;
        }
        clBufferDelete(&invocationBuf);
    }

    /*
     * Now sync the active invocation pair incase there is a shift.
     */
    if(gClAmsCkptLastDbInvocationPair != dbInvocationPair)
    {
        gClAmsCkptLastDbInvocationPair = dbInvocationPair;
        updateInvocationPair = CL_TRUE;
    }

    clOsalMutexUnlock(gAms.mutex);

    if(dataLen > 0 && dirtySection)
    {
        AMS_CHECK_RC_ERROR(clAmsCkptSectionOverwriteNoLock(
                                                           ckptHandle,
                                                           dirtySection,
                                                           (ClUint8T*)ckptData,
                                                           dataLen,
                                                           CL_AMS_CKPT_WRITE_DB));
    }

    if(invocationLen > 0)
    {
        AMS_CHECK_RC_ERROR(clAmsCkptSectionOverwriteNoLock(
                                                           ckptHandle,
                                                           &gClAmsCkptInvocationSectionCache[dbInvocationPair],
                                                           (ClUint8T *)invocationData,
                                                           invocationLen,
                                                           CL_AMS_CKPT_WRITE_INVOCATION));
    }

    if(updateInvocationPair)
    {
        AMS_LOG(CL_DEBUG_TRACE,
                ("AMS CKPT Section overwrite for [%s],dbInvocation pair [0x%x]",
                 gClAmsCkptCurrentSectionCache.value, dbInvocationPair));

        AMS_CHECK_RC_ERROR(clAmsCkptSectionOverwriteNoLock(
                                                           ckptHandle,
                                                           &gClAmsCkptCurrentSectionCache,
                                                           (ClUint8T*)&dbInvocationPair,
                                                           sizeof(dbInvocationPair),
                                                           CL_AMS_CKPT_WRITE_INVOCATION));
    }

    goto out_free;

    out_unlock:
    clOsalMutexUnlock(gAms.mutex);

    exitfn:
    rc = CL_AMS_RC (rc);

    out_free:
    if(ckptData)
        clHeapFree(ckptData);
    if(invocationData)
        clHeapFree(invocationData);

    return rc;
}

static ClRcT amsCkptWriteCallback(ClPtrT unused)
{
    static ClTimeT lastTime;
    static ClTimeT lastWriteTime;
    static ClUint32T numErrors;
    static ClUint32T numWrites;
    static ClTimerTimeOutT writePause = {.tsSec = 0, .tsMilliSec = AMS_CKPT_WRITE_PAUSE_MSEC };
    ClRcT rc;
    ClTimeT currentTime;
    currentTime = clOsalStopWatchTimeGet();
    if((lastTime 
        && 
        currentTime - lastTime < AMS_CKPT_FREQUENCY_USEC)
       ||
       (numErrors && !(numErrors & 3)))
    {
        ClUint32T elapsedMsec = (currentTime - lastTime)/1000;
        ClUint32T remainMsec = 0;
        ClTimerTimeOutT delay;
        if(AMS_CKPT_FREQUENCY_MSEC > 0)
            remainMsec = AMS_CKPT_FREQUENCY_MSEC - elapsedMsec;
        else
            remainMsec = 1000;
        delay.tsSec = 0;
        delay.tsMilliSec = remainMsec;
        numWrites = 0;
        clOsalTaskDelay(delay);
    }
    /*
     * Fire the ams ckpt write now.
     */
    clLogTrace("CKP", "WRITE", "Write at [%lld] usecs", clOsalStopWatchTimeGet());
    if(AMS_CKPT_FREQUENCY < 3)
    {
        clOsalMutexLock(gAms.mutex);
        rc = amsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_ALL);
        clOsalMutexUnlock(gAms.mutex);
    }
    else
    {
        rc = amsCkptWriteNoLock(&gAms, CL_AMS_CKPT_WRITE_ALL);
    }

    lastTime = clOsalStopWatchTimeGet();
    /*
     * If its a timeout, schedule a pause before the next write. 
     * For all other errors, retry again.
     */
    if(rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {
            numErrors += 3;
            numErrors &= ~3;
        }
        else
        {
            ++numErrors;
        }
    }
    else
    { 
        numErrors = 0;
        /*
         * Check the write rate. If its too slow or very fast, pause
         */
        if(!AMS_CKPT_FREQUENCY)
        {
            ClBoolT pauseOn = CL_FALSE;
            if(!numWrites++)
                lastWriteTime = lastTime;

            if( lastTime - currentTime  >= AMS_CKPT_MIN_FREQUENCY_USEC )
            {
                pauseOn = CL_TRUE;
            }
            else if(numWrites >= AMS_CKPT_WRITE_THRESHOLD_FAST
                    &&
                    (lastTime - lastWriteTime) <= AMS_CKPT_WRITE_FREQUENCY_USEC)
            {
                pauseOn = CL_TRUE;
            }
            else if(numWrites >= AMS_CKPT_WRITE_THRESHOLD_SLOW)
            {
                pauseOn = CL_TRUE;
            }
            if(pauseOn)
            {
                numWrites = 0;
                clOsalTaskDelay(writePause);
            }
        }
    }
    return CL_OK;
}

#ifdef VXWORKS_BUILD
static ClInt32T gClCkptDisabled = -1;
#endif

ClRcT
clAmsCkptWrite(ClAmsT *ams, ClUint32T mode)
{
#ifdef VXWORKS_BUILD
    if(gClCkptDisabled < 0)
    {
        if(getenv("CL_AMF_CKPT_DISABLED"))
            gClCkptDisabled = 1;
        else 
            gClCkptDisabled = 0;
    }
    if(gClCkptDisabled)
        return CL_OK;
#endif
    /*
     * Push into the ckpt write job queue if there are no pending jobs.
     */
    return clJobQueuePushIfEmpty(&gClAmsCkptJobQueue, amsCkptWriteCallback, NULL);
}

/*
 * Forced ckpt write in case async mode is on.
 */
ClRcT clAmsCkptWriteSync(ClAmsT *ams, ClUint32T mode)
{
#ifdef VXWORKS_BUILD
    if(gClCkptDisabled < 0)
    {
        if(getenv("CL_AMF_CKPT_DISABLED"))
            gClCkptDisabled = 1;
        else 
            gClCkptDisabled = 0;
    }
    if(gClCkptDisabled)
        return CL_OK;
#endif
    return amsCkptWrite(ams, mode);
}

ClRcT   
clAmsReadXMLFile(
        CL_IN  ClCharT  *fileName,
        CL_OUT  ClCharT  **readData )
{
    struct  stat  buf = {0} ;
    FILE  *fp = NULL;
    ClCharT  *data = NULL;
    ClRcT  rc = CL_OK;

    AMS_CHECKPTR_SILENT (!fileName);

    if ( stat (fileName,&buf) != 0 )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("Error in stating File[%s]\n",fileName));
        goto exitfn;
    }


    data  = clHeapAllocate (buf.st_size + 1); 

    AMS_CHECK_NO_MEMORY ( data );

    fp = fopen (fileName,"r");
    if ( !fp )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("Error in opening File[%s]\n",fileName));
        goto exitfn;
    }

    if ( !fread (data,1,buf.st_size,fp))
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("Error in Reading data from File[%s]\n",fileName));
        fclose (fp); 
        goto exitfn;
    }
    data[buf.st_size]='\0';

    *readData = data;
    fclose (fp); 

    return CL_OK;

exitfn:

    clAmsFreeMemory (data);
    return CL_AMS_RC (rc);

}

ClRcT   
clAmsWriteXMLFile(
        CL_IN  ClCharT  *fileName,
        CL_IN  ClCharT  *writeData,
        CL_IN  ClSizeT  dataSize )
{
    ClRcT  rc = CL_OK ;
    FILE  *fp = NULL;
    ClUint32T bytes = 0;

    AMS_CHECKPTR_SILENT ( !fileName || !writeData );
    
    if(dataSize >= 0xffffffffU)
    {
        clLogWarning("DB", "WRITE-XML", "Unusually large data size [%lld]", dataSize);
        return CL_OK;
    }

    bytes = (ClUint32T) dataSize;

    fp = fopen (fileName,"w+");
    if ( !fp )
    {
        rc = CL_ERR_NULL_POINTER;
        AMS_LOG (CL_DEBUG_ERROR,("Error in opening File[%s]\n",fileName));
        goto exitfn;
    }

    clLogTrace("DB", "WRITE-XML", "WRITING [%d] bytes to file [%s]", bytes, fileName);

    fprintf (fp,"%.*s", bytes, writeData);
    fclose (fp);

    return CL_OK;

exitfn:

    return CL_AMS_RC (rc);

}


ClRcT 
clAmsCkptFree( ClAmsT  *ams )
{

    ClRcT  rc = CL_OK;
    ClInt32T i;
    static ClUint32T numPairs = 2;

    AMS_CHECKPTR (!ams);

    clJobQueueDelete(&gClAmsCkptJobQueue);

    /*
     * Free the AMS DBInvocationPairs
     */
    for(i = 0; i < numPairs; ++i)
    {
        AMS_CHECK_RC_ERROR(clAmsCkptSectionDelete(ams,
                                                  &ams->ckptDBSections[i]));
        AMS_CHECK_RC_ERROR(clAmsCkptSectionDelete(ams,
                                                  &ams->ckptDirtySections[i]));
        AMS_CHECK_RC_ERROR(clAmsCkptSectionDelete(ams,
                                                  &ams->ckptInvocationSections[i]));
        clDifferenceVectorDelete(&ams->ckptDifferenceVectorKeys[i]);
        clDifferenceVectorKeyFree(&ams->ckptDifferenceVectorKeys[i]);
    }
    /*
     * Free the AMS current active section
     */
    AMS_CHECK_RC_ERROR(clAmsCkptSectionDelete(ams,
                                              &ams->ckptCurrentSection));

    /*
     * Free the AMS version section
     */
    AMS_CHECK_RC_ERROR(clAmsCkptSectionDelete(ams,
                                              &ams->ckptVersionSection));

    AMS_CHECK_RC_ERROR( clCkptCheckpointClose(ams->ckptOpenHandle) );

    if(gClAmsCkptDBInitialized == CL_TRUE)
    {
        gClAmsCkptDBInitialized = CL_FALSE;
        clCkptLibraryFinalize(gClAmsCkptDBHdl);
    }

    exitfn:

    return rc;

}

ClRcT
clAmsCkptReadCurrentDBInvocationPair(ClAmsT *ams,
                                     ClUint32T *pDBInvocationPair)
{
    ClCkptIOVectorElementT ioVector;
    ClUint32T dbInvocationPair ;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR( !ams );
    AMS_CHECKPTR( !pDBInvocationPair);

    memset(&ioVector,0,sizeof(ioVector));
    ioVector.dataSize=AMS_CKPT_MAX_SECTION_SIZE;
    ioVector.dataOffset=0; 

    AMS_CHECK_RC_ERROR(clAmsCkptCheckpointRead(
                                        ams,
                                        &ams->ckptCurrentSection,
                                        &ioVector));

    dbInvocationPair = *(ClUint32T*)ioVector.dataBuffer;
    
    if(dbInvocationPair >= 2)
    {
        rc = CL_AMS_RC(CL_ERR_UNSPECIFIED);
        AMS_LOG(CL_DEBUG_ERROR,("Ams Ckpt Read: Invalid dbInvocationPair [0x%x] found in AMS current section [%s]\n",dbInvocationPair,ams->ckptCurrentSection.value));
        clAmsFreeMemory(ioVector.dataBuffer);
        goto exitfn;
    }

    AMS_LOG(CL_DEBUG_TRACE,("AMS Ckpt Read: Reading DBInvocation Pair [0x%x] for DB Section [%s],Invocation [%s], \n",
                            dbInvocationPair,
                            ams->ckptDBSections[dbInvocationPair].value,
                            ams->ckptInvocationSections[dbInvocationPair].value));
    clAmsFreeMemory(ioVector.dataBuffer);
    *pDBInvocationPair = dbInvocationPair;

    exitfn:
    return rc;
}
