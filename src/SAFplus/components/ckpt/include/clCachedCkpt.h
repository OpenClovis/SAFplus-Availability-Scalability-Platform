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
 * File        : clCachedCkpt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the Cached Checkpoint
 * which provides a fast lookup for a P2P generated database.
 *
 *
 *******************************************************************************/

/**
 *  \file
 *  \brief Header file of the Cached Checkpoint which provides a fast lookup for
 *  a P2P generated database.
 *  \ingroup cached_ckpt
 */

/**
 *  \addtogroup cached_ckpt
 *  \{
 */

#ifndef _CL_CACHED_CKPT_H_
#define _CL_CACHED_CKPT_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHash.h>
#include <clIocApi.h>
#include <clOsalApi.h>
#include <saCkpt.h>
#include <clCkptApi.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************/
#define CL_CACHED_CKPT_MAX_SECTION      1024

#define CL_CACHED_CKPT_SHM_MODE                (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CL_CACHED_CKPT_SHM_OPEN_FLAGS          (O_RDWR)
#define CL_CACHED_CKPT_SHM_CREATE_FLAGS        (O_RDWR | O_CREAT)
#define CL_CACHED_CKPT_SHM_EXCL_CREATE_FLAGS   (O_RDWR | O_CREAT | O_EXCL)
#define CL_CACHED_CKPT_MMAP_FLAGS              (MAP_SHARED)
#define CL_CACHED_CKPT_MMAP_PROT_FLAGS         (PROT_READ | PROT_WRITE)

/**
 * The type of the CachedCkpt section data.
 */ 
typedef struct {
    /**
     * Name of the checkpoint section.
     */
    ClNameT sectionName;
    /**
     * IOC address of the component "responsible" for this section.
     */
    ClIocAddressT sectionAddress;
    /**
     * Size of the section data.
     */
    ClUint32T dataSize;
    /**
     * Data of the section.
     */
    ClUint8T *data;
    
}ClCachedCkptDataT;



/**
 * Cached Checkpoint global data structure.
 */
typedef struct {
    /**
     * File descriptor that refers to an open shared memory.
     */
    ClInt32T         fd;
    /**
     * Semaphore to protect Cached Checkpoint shared data.
     */
    ClOsalSemIdT cacheSem;
    /**
     * Service handle used for checkpoint service.
     */
    SaCkptHandleT ckptSvcHandle;
    /**
     * Checkpoint handle.
     */
    SaCkptCheckpointHandleT ckptHandle;
    /**
     * Shared buffer size.
     */
    ClUint32T cachSize;
    /**
     * Shared buffer.
     */
    ClUint8T *cache;

    /**
     * Cache shared memory name
     */
    ClCharT cacheName[CL_MAX_NAME_LENGTH];

}ClCachedCkptSvcInfoT;

/*******************************************************************************/

/************************* CACHED CHECKPOINT APIs ******************************/

/**
 ************************************
 *  \brief Initializes the Cached Checkpoint service.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by 
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 * 
 *  \param ckptName (in) Name of the checkpoint to be opened.
 * 
 *  \param ckptAttributes (in) A pointer to the create attributes of the checkpoint.
 * 
 *  \param openFlags (in) Flags to indicate the desired mode to open. It can 
 *   have the following values:
 *  \arg \c SA_CKPT_CHECKPOINT_READ
 *  \arg \c SA_CKPT_CHECKPOINT_WRITE
 *  \arg \c SA_CKPT_CHECKPOINT_CREATE
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameters passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_ALREADY_EXIST If the checkpoint is already existing.
 *  \retval CL_ERR_NO_MEMORY If there is not enough memory.
 *
 *  \par Description:
 *  This API is used to initialize the Cached Checkpoint service. It
 *  must be called before calling any other Cached Checkpoint APIs.
 *
 *  \par Library Files:
 *  clCachedCkpt
 *
 *  \sa clCachedCkptFinalize()
 *
 */
ClRcT clCachedCkptInitialize(ClCachedCkptSvcInfoT *serviceInfo, 
                                       const SaNameT *ckptName,
                                       const SaCkptCheckpointCreationAttributesT *ckptAttributes,
                                       SaCkptCheckpointOpenFlagsT openFlags,
                                       ClUint32T cachSize);
/*******************************************************************************/


/**
 ************************************
 *  \brief Finalizes the Cached Checkpoint service.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by 
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *
 *  \par Description:
 *  This API should be used when an application decides of not using the
 *  Cached Checkpoint service any more. The application must have invoked 
 *  clCachedCkptInitialize() before it invokes this function.
 *
 *  \par Library Files:
 *  clCachedCkpt
 *
 *  \sa clCachedCkptInitialize()
 *
 */
ClRcT clCachedCkptFinalize(ClCachedCkptSvcInfoT *serviceInfo);
/*******************************************************************************/


/**
 ************************************
 *  \brief Creates a section in the checkpoint.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by 
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 *
 *  \param sectionData (in) A pointer to the section data to be stored.
 *  Refer to ClCachedCkptDataT structure.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_ALREADY_EXIST If the checkpoint section is already existing.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API creates a new section in the checkpoint referred to by 
 *  \e serviceInfo->ckptHandle and stores its data in the cache (local copy).
 * 
 *  \par Library Files:
 *  clCachedCkpt
 *
 *  \sa
 *  clCachedCkptSectionDelete(),
 *  clCachedCkptSectionUpdate()
 *
 */
ClRcT clCachedCkptSectionCreate(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClCachedCkptDataT *sectionData);
/*******************************************************************************/


/**
 ************************************
 *  \brief Updates a checkpoint section.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 *
 *  \param sectionData (in) A pointer to the section data to be updated.
 *  Refer to ClCachedCkptDataT structure.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This API is used to update a single section in the checkpoint. As a result 
 *  of this invocation, the previous data of the section will change.
 * 
 *  \par Library Files:
 *  clCachedCkpt
 *
 *  \sa
 *  clCachedCkptSectionCreate(),
 *  clCachedCkptSectionDelete()
 * 
 */
ClRcT clCachedCkptSectionUpdate(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClCachedCkptDataT *sectionData);
/*******************************************************************************/


/**
 ************************************
 *  \brief Deletes a section in the checkpoint.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 * 
 *  \param sectionName (in) Name of the section to be deleted.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 * 
 *  \par Description:
 *  This API deletes a section in the checkpoint referred to by \e serviceInfo->ckptHandle
 *  and remove its data out of the cache (local copy).
 * 
 *  \par Library Files:
 *  clCachedCkpt
 *
 *  \sa
 *  clCachedCkptSectionCreate(),
 *  clCachedCkptSectionUpdate()
 * 
 */
ClRcT clCachedCkptSectionDelete(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClNameT *sectionName);
/*******************************************************************************/

/**
 ************************************
 *  \brief Reads a single section.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 * 
 *  \param sectionName (in) Name of the section to be read.
 *
 *  \param sectionData (out) Pointer to the section data to be read.
 *  Refer to ClCachedCkptDataT structure.
 * 
 *  \par Description:
 *  This API reads a single section and return section data to the caller.
 * 
 *  \par Library Files:
 *  clCachedCkpt
 * 
 */
void clCachedCkptSectionRead(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClNameT *sectionName,
                                       ClCachedCkptDataT **sectionData);

/**
 ************************************
 *  \brief Gets the first section of the checkpoint.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 *
 *  \param sectionData (out) Pointer to the section data to be read.
 *  Refer to ClCachedCkptDataT structure.
 *
 *  \param sectionOffset (out) Pointer to the offset of the next section
 *
 *
 *  \par Description:
 *  This API gets the first section of the checkpoint and return section data to the caller.
 *
 *  \par Library Files:
 *  clCachedCkpt
 *
 */
void clCachedCkptSectionGetFirst(ClCachedCkptSvcInfoT *serviceInfo,
                                       ClCachedCkptDataT **sectionData,
                                       ClUint32T        *sectionOffset);

/**
 ************************************
 *  \brief Gets next section of the checkpoint.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 *
 *  \param sectionData (out) Pointer to the section data to be read.
 *  Refer to ClCachedCkptDataT structure.
 *
 *  \param sectionOffset (out) Pointer to the offset of the next section
 *
 *  \par Description:
 *  This API gets next section of the checkpoint and return section data to the caller.
 *
 *  \par Library Files:
 *  clCachedCkpt
 *
 */
void clCachedCkptSectionGetNext(ClCachedCkptSvcInfoT *serviceInfo,
                                       ClCachedCkptDataT **sectionData,
                                       ClUint32T        *sectionOffset);

/*******************************************************************************/

/**
 ************************************
 *  \brief Synchronizes the cache (local copy) with the checkpoint.
 *
 *  \par Header File:
 *  clCachedCkpt.h
 *
 *  \par Parameters:
 *  \param serviceInfo (in) A pointer to the global data structure used by
 *  CachedCkpt service. Refer to ClCachedCkptSvcInfoT structure.
 * 
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API synchronizes the cache (local copy) with the checkpoint. It is 
 *  called typically when the checkpoint is updated remotely.
 *
 *  \par Library Files:
 *  clCachedCkpt
 *
 */
ClRcT clCachedCkptSynch(ClCachedCkptSvcInfoT *serviceInfo, ClBoolT isEmpty);
/*******************************************************************************/

/* Functions to update cache */
ClRcT clCacheEntryAdd (ClCachedCkptSvcInfoT *serviceInfo, 
                                       const ClCachedCkptDataT *sectionData);
ClRcT clCacheEntryUpdate (ClCachedCkptSvcInfoT *serviceInfo, 
                                       const ClCachedCkptDataT *sectionData);
ClRcT clCacheEntryDelete (ClCachedCkptSvcInfoT *serviceInfo, 
                                       const ClNameT *sectionName);

/*
 * Append the data chunk to the last section
 */
ClRcT clCacheEntryDataAppend(ClCachedCkptSvcInfoT *serviceInfo,
                             ClPtrT data,
                             ClUint32T dataSize);

/*
 * Delete a chunk from the last section matching a data chunk of dataSize
 */
ClRcT clCacheEntryDataDelete(ClCachedCkptSvcInfoT *serviceInfo,
                             ClPtrT data,
                             ClUint32T dataSize);

/* Functions to update checkpoint */
ClRcT clCkptEntryUpdate(ClCachedCkptSvcInfoT *serviceInfo,
                                       const ClCachedCkptDataT *sectionData);
ClRcT clCkptEntryDelete(ClCachedCkptSvcInfoT *serviceInfo, const ClNameT *sectionName);

/****************************Client Side APIs**************************/
/**
 * Client side data structure.
 */
typedef struct {
    /**
     * File descriptor that refers to an open shared memory.
     */
    ClInt32T         fd;
    /**
     * Semaphore to protect the shared memory.
     */
    ClOsalSemIdT cacheSem;
    /**
     * Shared buffer size.
     */
    ClUint32T cachSize;
    /**
     * Shared buffer.
     */
    ClUint8T *cache;
}ClCachedCkptClientSvcInfoT;

void clCachedCkptClientLookup(ClCachedCkptClientSvcInfoT *serviceInfo,
                                       const ClNameT *sectionName,
                                       ClCachedCkptDataT **sectionData);
ClRcT clCachedCkptClientInitialize(ClCachedCkptClientSvcInfoT *serviceInfo,
                                       const ClNameT *ckptName,
                                       ClUint32T cachSize);
ClRcT clCachedCkptClientFinalize(ClCachedCkptClientSvcInfoT *serviceInfo);



#ifdef __cplusplus
}
#endif

#endif                          /* _CL_CACHED_CKPT_H_ */

/** \} */


