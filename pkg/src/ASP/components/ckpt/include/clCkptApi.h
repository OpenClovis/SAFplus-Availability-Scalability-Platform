/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : ckpt                                                          
 * File        : clCkptApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * File        : clCkptApi.h
*   This file contains Checkpoint service APIs defined by SAF
*
*
*****************************************************************************/

/**
 *  \file
 *  \brief Header file of Server based Checkpoint Service Related APIs
 *  \ingroup ckpt_apis_server
 */

/**
 ************************************
 *  \addtogroup ckpt_apis_server
 *  \{
 */

#ifndef _CL_CKPT_API_H_
#define _CL_CKPT_API_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clIocApi.h>

/********************************
      C O N S T A N T S
********************************/

/**
 * Synchronous checkpoint.
 */
#define CL_CKPT_WR_ALL_REPLICAS        0X1

/**
 * Asynchronous checkpoint.
 */
#define CL_CKPT_WR_ACTIVE_REPLICA      0X2

/**
 * Asynchronous checkpoint. Rest of the replicas updation need not be atomic.
 */
#define CL_CKPT_WR_ACTIVE_REPLICA_WEAK 0X4


/**
 *  Collocated checkpoint.
 */
#define CL_CKPT_CHECKPOINT_COLLOCATED  0X8

/**
 *  Hot stanby checkpoint. This is not defined by SAF but an extension by OpenClovis ASP.
 */
#define CL_CKPT_DISTRIBUTED           0X10
/**
 *  Safe Asynchronous checkpoint. This is not defined by SAF but an extension by OpenClovis ASP.
 */
#define CL_CKPT_WR_ALL_SAFE            0X20

/**
 *  Checkpoint open for read.
 */
#define CL_CKPT_CHECKPOINT_READ        0X1

/**
 *  Checkpoint open for write.
 */
#define CL_CKPT_CHECKPOINT_WRITE       0X2

/**
 *  Checkpoint open for create.
 */
#define CL_CKPT_CHECKPOINT_CREATE      0X4

/**
 *  NULL Section ID.
 */
#define CL_CKPT_DEFAULT_SECTION_ID   {0, NULL}
/**
 * Default ID for generated section
 */ 
#define CL_CKPT_GENERATED_SECTION_ID {0, NULL}



/**
 * The type of the handle for the the checkpoint service library.
 */
typedef ClHandleT ClCkptSvcHdlT;

/**
 *  The handle used to identify a checkpoint.
 */
typedef ClHandleT ClCkptHdlT;

/**
 * The handle used to identify a section in a checkpoint.
 */
typedef ClHandleT ClCkptSecItrHdlT;

/**
 *  Flags to indicate various attributes of a checkpoint on creation.
 */
typedef ClUint32T  ClCkptCreationFlagsT;

/**
 *  Flags to indicate open mode such as read, write or create.
 */
typedef ClUint32T  ClCkptOpenFlagsT;
/**
 *  Selection object.
 */
typedef ClUint32T  ClCkptSelectionObjT;

/**
 *  This structure represents the properties of checkpoint that can be specified during the creation process.
 */
typedef struct {

/**
 *  Create time attributes.
 */
    ClCkptCreationFlagsT   creationFlags;

/**
 *  Total size of application data in a replica.
 */
    ClSizeT                checkpointSize;

/**
 *  Checkpoint which is inactive (not opened anywhere) for this duration (in nanoseconds) is deleted.
 */
    ClTimeT                retentionDuration;

/**
 *  Maximum sections for this checkpoint.
 */
    ClUint32T              maxSections;

/**
 * Maximum size of a section.
 */
    ClSizeT                maxSectionSize;

/**
 *  Maximum length of the section identifier.
 */
    ClSizeT                maxSectionIdSize;
} ClCkptCheckpointCreationAttributesT;

/**
 *  This structure represents a section identifier.
 */
typedef struct {

/**
 *  Length of the section identifier.
 */
    ClUint16T      idLen;

/**
 *  Section identifier.
 */
    ClUint8T       *id;

} ClCkptSectionIdT;

/**
 *  This structure represents section attributes that can be specified during the creation process.
 */
typedef struct {

/**
 *  Section identifier.
 */
    ClCkptSectionIdT          *sectionId;

/**
 * Section expiration time.
 */
    ClTimeT                expirationTime;
} ClCkptSectionCreationAttributesT;

/**
 *  This enum represents the state of a section in a replica.
 */
typedef enum {

/**
 * Indicates that the section is fine.
 */
    CL_CKPT_SECTION_VALID = 1,

/**
 *  Indicates that the section has been corrupted.
 */
    CL_CKPT_SECTION_CORRUPTED = 2
} ClCkptSectionStateT;

/**
 *  This structure represents a section in a checkpoint.
 */
typedef struct {

/**
 * Section identifier.
 */
    ClCkptSectionIdT             sectionId;

/**
 *  Expiration time for the section.
 */
    ClTimeT                   expirationTime;

/**
 * Size of the section.
 */
    ClSizeT                   sectionSize;

/**
 * Indicates whether a section has a valid or an invalid state.
 */
    ClCkptSectionStateT          sectionState;

/**
 *  Last time the section is updated.
 */
    ClTimeT                   lastUpdate;
} ClCkptSectionDescriptorT;

/**
 * This enum is used for selection of sections while iterating through all the sections.
 */
typedef enum {
    /**
     * All sections with expiration time set to CL_TIME_END
     */
    CL_CKPT_SECTIONS_FOREVER = 1,
    /**
     * All sections with expiration time less than or equal to the value 
     * of expirationTime.
     */
    CL_CKPT_SECTIONS_LEQ_EXPIRATION_TIME = 2,
    /**
     * All sections with expiration time greater than or equal to the value
     * of expiration time
     */
    CL_CKPT_SECTIONS_GEQ_EXPIRATION_TIME = 3,
    /**
     * All corrupted sections.
     */
    CL_CKPT_SECTIONS_CORRUPTED = 4,
    /**
     * All sections
     */
    CL_CKPT_SECTIONS_ANY = 5
} ClCkptSectionsChosenT;

/**
 * This structure represents an IO vector which will be used for dealing with more than zero sections.
 */
typedef struct {

/**
 *  Identifier of the section.
 */
    ClCkptSectionIdT      sectionId;

/**
 *  Pointer to the data.
 */
    ClPtrT                dataBuffer;

/**
 * Size of the data.
 */
    ClSizeT               dataSize;

/**
 *  Offset.
 */
    ClOffsetT             dataOffset;

/**
 *  Number of bytes read.
 */
    ClSizeT               readSize;
} ClCkptIOVectorElementT;

/**
 *  This structure is used to describe a checkpoint.
 */
typedef struct {

/**
 *  Creates attribute.
 */
    ClCkptCheckpointCreationAttributesT   checkpointCreationAttributes;

/**
 *  Total number of sections.
 */
    ClUint32T                        numberOfSections;

/**
 *  Memory used by the checkpoint.
 */
    ClUint32T                        memoryUsed;
} ClCkptCheckpointDescriptorT;

/**
 ************************************
 *  \brief This function gets called whenever a checkpoint is getting updated  
 *   on the local server. Provided the application has been registered for the 
 *   checkpoint update.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHdl (in) Handle of the updated checkpoint.  
 *
 *  \param pName (in) Name of the checkpoint has been updated locally. 
 *
 *  \param pIoVector (in) List of updated sections and data of the checkpoint. 
 *
 *  \param numSections (in) Number of sections are being updated on this checkpoint. 
 *
 *  \param pCookie (in) Cookie which got from clCkptImmediateConsumptionRegister will be given back to 
 *                      application via this variable.
 *
 *  \retval  CL_OK The Log stream is opened successfully. 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Ckpt Service library or some other module of 
 *  Ckpt Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \par Description:
 *  This notification callback is getting called, whenever the particular 
 *  checkpoint has been updated on the local server.This callback will be called
 *  for whoever registers for the checkpoint update. Thie notication callback will be 
 *  registered through clCkptImmediateConsumptionRegister() call. This function carries 
 *  information about all updated sections and number of sections. This is mainly used
 *  for HOT stanby applications on which the stanby application will be notified 
 *  immediately.  
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptImmediateConsumptionRegister
 *
 */

typedef ClRcT (*ClCkptNotificationCallbackT)( 
                        ClCkptHdlT              ckptHdl,
                        ClNameT                 *pName,
                        ClCkptIOVectorElementT  *pIOVector,
                        ClUint32T               numSections,
                        ClPtrT                  pCookie  );
/**
 ************************************
 *  \brief This function gets called When clCkptCheckpointOpenAsync() call 
 *   returns on the server. 
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param invocation (in) invocation value of the call. This is basically an
 *  identification about the call.
 *
 *  \param checkpointHandle (in) contains handle of the opened checkpoint, if the
 *  clCkptCheckpointOpenAsync() was succesfull. 
 *
 *  \param error (in) return value of the clCkptCheckpointOpenAsync() call 
 *
 *  \retval  CL_OK The checkpoint is opened successfully. 
 *
 *  \retval  CL_ERR_NOT_EXIST \c CL_CKPT_CHECKPOINT_CREATE flag is not set in creation flags and
 *  the checkpoint does not exist.
 *
 *  \retval CL_ERR_ALREADY_EXIST \c CL_CKPT_CHECKPOINT_CREATE flag is set in creation flags but
 *  the Checkpoint already exists and was originally created with different
 *  attributes than specified by CheckpointCreation attributes. 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Checkpoint Service library or some other module of 
 *  Ckpt Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Checkpoint Service library or some other module 
 *  of Checkpoint Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This CheckpointOpe callback function is getting called, when
 *  clCkptCheckpointOpenAsync() call returns from the server. It carries the  
 *  invocation to identify the call and return code for indicating the status
 *  of the call. If the retcode is CL_OK, then checkpoint handlE will be carrying
 *  the proper handle of the checkpoint which was opened on the particular
 *  invocation.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointOpenAsync()
 *
 */

typedef void (*ClCkptCheckpointOpenCallbackT)(
                        ClInvocationT     invocation,
                        ClCkptHdlT        checkpointHandle,
                        ClRcT             error);

/**
 ************************************
 *  \brief This function gets called When clCkptCheckpointSynchronizeAsync() call 
 *   returns on the server. 
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param invocation (in) invocation value of the call. This is basically an
 *  identification about the call.
 *
 *  \param error (in) return value of the clCkptCheckpointSynchronizeAsync() call 
 *
 *  \retval  CL_OK The checkpoint is opened successfully. 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Checkpoint Service library or some other module of 
 *  Ckpt Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Checkpoint Service library or some other module 
 *  of Checkpoint Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  Applications which use asynchronous checkpoint option will be notified about the asynchronous
 *  write or open status with a callback. If any problem occurs with the asynchronous write,
 *  the application asks checkpointing to synchronize all the replicas. This is an
 *  asynchronous call. So the applications can register another optional callback which will enable
 *  them to know the status of 'synchronize-all-replicas' call.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointSynchronizeAsync() 
 *
 */

typedef void (*ClCkptCheckpointSynchronizeCallbackT)(
                        ClInvocationT invocation,
                        ClRcT  error);

/**
 *  This structure is the only location where all the callbacks converge.
 */
typedef struct {

/**
 *  Asynchronous open callback.
 */
    ClCkptCheckpointOpenCallbackT          checkpointOpenCallback;

/**
 * Synchronize callback.
 */
    ClCkptCheckpointSynchronizeCallbackT   checkpointSynchronizeCallback;
} ClCkptCallbacksT;


/**
 ************************************
 *  \brief  Initializes the checkpoint service client and registers the various callbacks.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptSvcHandle (out) Checkpoint service handle created by the checkpoint client
 *  and returned to the application. This handle designates this particular initialization of the
 *  the Checkpoint Service. The application must not modify or interpret this.
 *
 *  \param callbacks Optional callbacks for applications which use asynchronous checkpoints.
 *
 *  \param version (in/out)  As an input parameter, version is a pointer to the required
 *  Checkpoint Service version.  As an output parameter, the version actually supported by the
 *  Checkpoint Service is delivered.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *  \retval CL_ERR_NULL_POINTER
 *  \retval CL_ERR_VERSION_MISMATCH
 *
 *  \par Description:
 *  This function initializes the Checkpoint library for the invoking process and registers
 *  the various callback functions. This function must be invoked prior to the invocation of
 *  any other Checkpoint Service functionality. The handle \e ckptSvcHandle is returned as the
 *  reference to this association between the process and the Checkpoint Service. The
 *  process uses this handle in subsequent communication with the Checkpoint Service.
 *
 *x
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptFinalize()
 *
 */
extern ClRcT clCkptInitialize( CL_OUT ClCkptSvcHdlT            *ckptSvcHandle,  /* Checkpoint service handle */
                        CL_IN  const ClCkptCallbacksT   *callbacks,      /* Optional callbacks */
                        CL_INOUT ClVersionT             *version);       /* Version */




/**
 ************************************
 *  \brief  Closes the checkpoint service client and cancels all pending callbacks related to the handle.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize() function,
 *   designating this particular initialization of the Checkpoint Service.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  The clCkptFinalize() function closes the association, represented by the \e ckptHandle
 *  parameter, between the invoking process and the Checkpoint Service. The process
 *  must have invoked clCkptInitialize() before it invokes this function. A process must
 *  invoke this function once for each handle acquired by invoking  clCkptInitialize(). 
 *
 *  \par 
 *  If the clCkptFinalize() function returns successfully, the clCkptFinalize() function
 *  releases all resources acquired when clCkptInitialize() was called. Moreover, it
 *  closes all checkpoints that are open for the particular handle. Furthermore, it cancels
 *  all pending callbacks related to the particular handle. 
 *
 *  \par 
 *  After clCkptFinalize() is called, the selection object is no longer valid. Note that
 *  because the callback invocation is asynchronous, it is still possible that some callback
 *  calls are processed after this call returns successfully.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptInitialize()
 *
 */

extern ClRcT clCkptFinalize( CL_IN ClCkptSvcHdlT ckptHandle);

/**
 ************************************
 *  \brief Opens an existing checkpoint. If there is no existing checkpoint, 
 *   then this function creates a new checkpoint and opens it.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize() function,
 *   designating this particular initialization of the Checkpoint Service.
 *
 *  \param ckeckpointName Name of the checkpoint to be opened.
 *
 *  \param  checkpointCreationAttributes A pointer to the create attributes of a the checkpoint.
 *  Refer to ClCkptCheckpointCreationAttributesT structure. This parameter must be filled
 *  only when the \c CREATE flag is set.
 *
 *  \param checkpointOpenFlags  Flags to indicate the desired mode to open. It can have the following values:
 *  \arg \c CL_CKPT_CHECKPOINT_READ
 *  \arg \c CL_CKPT_CHECKPOINT_WRITE
 *  \arg \c CL_CKPT_CHECKPOINT_CREATE
 *
 *  \param timeout clCkptCheckpointOpen is considered to be failed if not completed within the timeout.
 *  A checkpoint replica may still be created. [It is not supported currently]
 *
 *  \param checkpointHandle (out) A pointer to the checkpoint handle, allocated in the address
 *  space of the invoking process. If the checkpoint is opened successfully, the Checkpoint
 *  Service stores in \e checkpointHandle the handle that the process uses to access
 *  the checkpoint in subsequent invocations of the functions of the Checkpoint Service.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameters passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_ALREADY_EXIST If the checkpoint is already existing.
 *  \retval CL_ERR_NO_MEMORY If there is not enough memory.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  The clCkptCheckpointOpen() function opens a checkpoint.
 *  If the checkpoint does not exist, the checkpoint is created first.
 *  An invocation of clCkptCheckpointOpen() is blocking. A new checkpoint handle is
 *  returned upon completion. A checkpoint can be opened multiple times for reading
 *  and/or writing in the same process or in different processes. 
 *
 *  \par 
 *  When a checkpoint is opened using the clCkptCheckpointOpen() or
 *  clCkptCheckpointOpenAsync() function, some combination of the creation flags,
 *  defined in ClCkptCheckpointCreationFlagsT, are bitwise OR-ed together to provide
 *  the value of the \e creationFlags field of the \e checkpointCreationAttributes parameter.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointOpenAsync(), clCkptCheckpointClose()
 *
 */

extern ClRcT clCkptCheckpointOpen( CL_IN ClCkptSvcHdlT          ckptHandle,   /* Service handle */
                            CL_IN const ClNameT        *ckeckpointName, /* Name of the checkpoint */
                            CL_IN const ClCkptCheckpointCreationAttributesT *checkpointCreationAttributes,
                            CL_IN ClCkptOpenFlagsT       checkpointOpenFlags, /* Open flags */
                            CL_IN ClTimeT               timeout,             /* Time out */
                            CL_IN ClCkptHdlT             *checkpointHandle);  /* Return handle */



/**
 ************************************
 *  \brief Creates and opens a checkpoint asynchronously.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize() function,
 *   designating this particular initialization of the Checkpoint Service.
 *
 *  \param invocation This parameter is used by the application to identify the callback.
 *
 *  \param checkpointName Name of the checkpoint to be opened.
 *
 *  \param checkpointCreationAttributes Pointer to the create attributes of a the checkpoint.
 *  Refer to ClCkptCheckpointCreationAttributesT structure.
 *
 *  \param checkpointOpenFlags  Flags to indicate the desired mode to open. It can have the following values:
 *  \arg \c CL_CKPT_CHECKPOINT_READ
 *  \arg \c CL_CKPT_CHECKPOINT_WRITE
 *  \arg \c CL_CKPT_CHECKPOINT_CREATE
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to create and open a checkpoint asynchronously.
 *
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointOpen(), clCkptCheckpointClose()
 */

extern ClRcT clCkptCheckpointOpenAsync( CL_IN ClCkptSvcHdlT            ckptHandle,
                                 CL_IN ClInvocationT          invocation,
                                 CL_IN const ClNameT          *checkpointName,
                                 CL_IN const ClCkptCheckpointCreationAttributesT *checkpoiNtCreationAttributes,
                                 CL_IN ClCkptOpenFlagsT          checkpointOpenFlags);

/**
 ************************************
 *  \brief Closes the checkpoint designated by the \e checkpointHandle.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle \e checkpointHandle must have been obtained
 *   previously by the invocation of the clCkptCheckpointOpen() function.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This function closes the checkpoint, designated by \e checkpointHandle, which was
 *  opened by an earlier invocation of the clCkptCheckpointOpen() or
 *  clCkptCheckpointOpenAsync() function. 
 *
 *  \par 
 *  After this invocation, the handle \e checkpointHandle is no longer valid.
 *  The deletion of a checkpoint frees all resources allocated by the Checkpoint Service
 *  for it. When a process is terminated, all of its opened checkpoints are closed.
 *  This call cancels all pending callbacks that refer directly or indirectly to the handle
 *  \e checkpointHandle.
 *
 *  \par 
 *  In case  clCkptCheckpointDelete() has already been called, then by calling the clCkptCheckpointClose()
 *  function, the reference count to this checkpoint becomes zero and the checkpoint is deleted.
 *  After this call, if the reference count becomes 0, and clCkptCheckpointDelete() has not been called,
 *  then the retention timer associated with the checkpoint [ and provided during invocation of
 *  clCkptCheckpointOpenAsync() or clCkptCheckpointOpen() functions ] is started. On expiration of the timer
 *  the checkpoint is deleted.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointOpen(), clCkptCheckpointOpenAsync(), clCkptCheckpointDelete()
 *
 */

extern ClRcT clCkptCheckpointClose(CL_IN ClCkptHdlT checkpointHandle);

/**
 ************************************
 *  \brief Removes the checkpoint from the system and frees all resources allocated to it.
 *
 *  \par Header File:
 *  clCkptApi.ht
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize() function,
 *   designating this particular initialization of the Checkpoint Service.
 *  \param checkpointName Pointer to the name of the checkpoint to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  If the parameters passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_INUSE If the checkpoint is already in use.
 *
 *  \par Description:
 *  This function deletes an existing checkpoint, identified by \e checkpointName from the
 *  cluster. 
 *
 *  \par
 *  After completion of the invocation,
 *  \arg The name \e checkpointName is no longer valid, that is, any invocation of a function
 *  of the Checkpoint Service that uses the checkpoint name returns an error,
 *  unless a checkpoint is re-created with this name. The checkpoint is re-created
 *  by specifying the same name of the checkpoint to be unlinked in an open call
 *  with the \c CL_CKPT_CHECKPOINT_CREATE flag set. This way, a new instance
 *  of the checkpoint is created, while the old instance of the checkpoint is
 *  not yet finally deleted. Note that this is similar to the way POSIX treats files.
 *  \arg If no process has the checkpoint open when clCkptCheckpointDelete() is
 *  invoked, the checkpoint is immediately deleted.
 *  \arg Any process that has the checkpoint open can still continue to access it. Deletion
 *  of the checkpoint will occur when the last clCkptCheckpointClose() operation is
 *  performed. 
 *
 *  \par
 *  The deletion of a checkpoint frees all resources allocated by the Checkpoint Service
 *  for it. This function can be invoked by any process, and the invoking process need not be the
 *  creator or opener of the checkpoint.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointOpen(), clCkptCheckpointOpenAsync(), clCkptCheckpointClose()
 *
 */

extern ClRcT clCkptCheckpointDelete( CL_IN ClCkptSvcHdlT       ckptHandle,      /* checkpoint svc handle */
                              CL_IN const ClNameT     *checkpointName);

/**
 ************************************
 *  \brief Sets the retention duration of a checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The checkpoint whose retention time is being set. This handle
 *  is obtained by invoking clCkptCheckpointOpen() function.
 *  \param retentionDuration Duration for which the checkpoint can be retained in nanoseconds.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to set the retention duration of a checkpoint.
 *  If by invocation of clCkptCheckpointClose(), the reference count becomes 0, and clCkptCheckpointDelete() has not been
 *  called, then the retention timer associated with the checkpoint ( and provided during invocation of
 *  clCkptCheckpointOpenAsync() or clCkptCheckpointOpen() functions ) is started. On expiration of the timer, the checkpoint
 *  is deleted.
 *
 *  \par Library File:
 *  ClCkpt
 *
 */
extern ClRcT clCkptCheckpointRetentionDurationSet( CL_IN ClCkptHdlT checkpointHandle,
                                            CL_IN ClTimeT retentionDuration);

/**
 ************************************
 *  \brief Sets the local replica to be active replica.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle \e checkpointHandle obtained by invoking
 *   clCkptCheckpointOpen() function.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to set the local replica to be active replica
 *  if no active replica has been set for the checkpoint.
 *
 *  \par Library File:
 *  ClCkpt
 *
 */
extern ClRcT clCkptActiveReplicaSet(CL_IN ClCkptHdlT checkpointHandle);

/**
 ************************************
 *  \brief  Returns the status and the various attributes of the checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.ht
 *
 *  \param checkpointHandle  The handle of the checkpoint obtained by invoking
 *   clCkptCheckpointOpen() function.
 *  \param checkpointStatus (out) Pointer to checkpoint descriptor in the address space of
 *   the invoking process, that contains the checkpoint status information to be returned.
 *   Refer to ClCkptCheckpointDescriptorT structure.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  If the \e checkpointStatus parameter passed is a NULL pointer.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to return the status and the various attributes of the checkpoint.
 *  The list of attributes are defined by the ClCkptCheckpointDescriptorT structure.
 *  This function retrieves the \e checkpointStatus of the checkpoint designated by
 *  checkpointHandle. 
 *
 *  \par
 *  If the checkpoint was created using either \c CL_CKPT_WR_ACTIVE_REPLICA or
 *  \c CL_CKPT_WR_ACTIVE_REPLICA_WEAK option, the checkpoint status is obtained
 *  from the active replica. If the checkpoint was created using the
 *  \c CL_CKPT_WR_ALL_REPLICAS option, the Checkpoint Service determines the replica
 *  from which to obtain the checkpoint status.
 *
 *  \par Library File:
 *  ClCkpt
 *
 */
extern ClRcT clCkptCheckpointStatusGet( CL_IN ClCkptHdlT                 checkpointHandle,
                                 CL_OUT ClCkptCheckpointDescriptorT *checkpointStatus);
/**
 ************************************
 *  \brief Creates a section in the checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle \e checkpointHandle obtained by invoking
 *   clCkptCheckpointOpen() function.
 *
 *  \param sectionCreationAttributes A pointer to a ClCkptSectionCreationAttributesT
 *  structure, that contains the (in/out) field \e sectionId and the (in) field \e expirationTime.
 *
 *  \param initialData Pointer to the location in the address space of the invoking
 *  process that contains the initial data of the section to be created.
 *
 *  \param initialDataSize The size in bytes of the initial data of the section to be created.
 *  Initial size can be at most \e maxSectionSize, as specified by the checkpoint creation
 *  attributes in clCkptCheckpointOpen().
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When either of the parameters \e sectionCreationAttributes
 *  or initialData passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_ALREADY_EXIST If the checkpoint is already existing.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function creates a new section in the checkpoint referred to by checkpointHandle
 *  as long as the total number of existing sections is less than the maximum number of
 *  sections specified by the clCkptCheckpointOpen() or
 *  clCkptCheckpointOpenAsync() function call. Unlike a checkpoint, a section
 *  need not be opened for access. The section will be deleted by the Checkpoint Service
 *  when its expiration time is reached. 
 *  \par
 *  If a checkpoint is created to have only one section,
 *  it is not necessary to create that section. The default section is identified by the
 *  special identifier \c CL_CKPT_DEFAULT_SECTION_ID. If the checkpoint was created
 *  with the \c CL_CKPT_WR_ALL_REPLICAS property, the section is created in all of the
 *  checkpoint replicas when the invocation returns; otherwise, the section has been created
 *  at least in the active checkpoint replica, when the invocation returns and will be
 *  created asynchronously in the other checkpoint replicas.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationInitialize(),
 *  clCkptSectionIterationNext(),
 *  clCkptSectionIterationFinalize(),
 *  clCkptCheckpointWrite(),
 *  clCkptSectionOverwrite()
 *
 */

extern ClRcT clCkptSectionCreate( CL_IN ClCkptHdlT         checkpointHandle,
                           CL_IN ClCkptSectionCreationAttributesT *sectionCreationAttributes,
                           CL_IN const ClUint8T   *initialData,
                           CL_IN ClSizeT         initialDataSize);


/**
 ************************************
 *  \brief  Deletes a section in the given checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.ht
 *
 *  \param checkpointHandle  The handle to the checkpoint holding the section
 *   to be deleted. The handle to the checkpoint holding the section to
 *   be deleted. The handle \e checkpointHandle obtained by invoking
 *   clCkptCheckpointOpen() function.
 *  \param sectionId A pointer to the identifier of the section to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameter \e sectionId passed is a NULL pointer.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function deletes a section in the checkpoint referred to by \e checkpointHandle. If
 *  the checkpoint was created with the \c CL_CKPT_WR_ALL_REPLICAS property, the
 *  section has been deleted in all of the checkpoint replicas when the invocation returns;
 *  otherwise, the section has been deleted at least in the active checkpoint replica when
 *  the invocation returns. The default section, identified by
 *  \c CL_CKPT_DEFAULT_SECTION_ID, cannot be deleted by invoking the
 *  clCkptSectionDelete() function.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *   clCkptSectionCreate(),
 *   clCkptSectionExpirationTimeSet(),
 *   clCkptSectionIterationInitialize(),
 *   clCkptSectionIterationNext(),
 *   clCkptSectionIterationFinalize(),
 *   clCkptCheckpointWrite(),
 *   clCkptSectionOverwrite()
 */
extern ClRcT clCkptSectionDelete( CL_IN ClCkptHdlT  checkpointHandle,
                           CL_IN const ClCkptSectionIdT     *sectionId);

/**
 ************************************
 *  \brief Sets the expiration time of a section.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle \e checkpointHandle obtained by invoking
 *   clCkptCheckpointOpen() function.
 *  \param sectionId Section identifier.
 *  \param expirationTime Expiration time of the section.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to set the expiration time of a section.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *   clCkptSectionCreate(),
 *   clCkptSectionDelete(),
 *   clCkptSectionIterationInitialize(),
 *   clCkptSectionIterationNext(),
 *   clCkptSectionIterationFinalize(),
 *   clCkptCheckpointWrite(),
 *   clCkptSectionOverwrite()
 *
 */
extern ClRcT clCkptSectionExpirationTimeSet( CL_IN ClCkptHdlT                checkpointHandle,
                                      CL_IN const ClCkptSectionIdT*   sectionId,
                                      CL_IN ClTimeT                expirationTime);
/**
 ************************************
 *  \brief Enables the application to iterate through sections in a checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle  The handle \e checkpointHandle obtained by invoking
 *   clCkptCheckpointOpen() function.
 *  \param sectionsChosen Rule for the iteration.
 *   Refer the ClCkptSectionChosenT enum.
 *  \param expirationTime Expiration time used along with the rule.
 *  \param sectionIterationHandle (out) Handle used to identify a present section.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *   This function is used to enable the application to iterate through the sections in a checkpoint.
 *
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionCreate(),
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationNext(),
 *  clCkptSectionIterationFinalize(),
 *  clCkptCheckpointWrite(),
 *  clCkptSectionOverwrite()
 *
 */
extern ClRcT clCkptSectionIterationInitialize( CL_IN  ClCkptHdlT              checkpointHandle,
                                        CL_IN  ClCkptSectionsChosenT   sectionsChosen,
                                        CL_IN  ClTimeT              expirationTime,
                                        CL_OUT ClHandleT         *sectionIterationHandle);

/**
 ************************************
 *  \brief  Returns the next section in the list of sections.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param sectionIterationHandle Checkpoint handle returned as part of clCkptCheckpointOpen().
 *  \param sectionDescriptor (out) Pointer to the descriptor of a section.
 *   Refer the ClCkptSectionDescriptorT structure.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to retrieve the next section in the list of sections matching the
 *  clCkptSectionIterationInitialize() call.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionCreate(),
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationInitialize(),
 *  clCkptSectionIterationFinalize(),
 *  clCkptCheckpointWrite(),
 *  clCkptSectionOverwrite
 *
 */
extern ClRcT clCkptSectionIterationNext( CL_IN  ClHandleT   sectionIterationHandle,
                                  CL_OUT ClCkptSectionDescriptorT *sectionDescriptor);

/**
 ************************************
 *  \brief Frees resources associated with the iteration.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param sectionIterationHandle Handle of the iteration.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to free resources associated with the iteration 
 *  identified by the sectionIterationHandle.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionCreate(),
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationInitialize(),
 *  clCkptSectionIterationNext(),
 *  clCkptCheckpointWrite(),
 *  clCkptSectionOverwrite()
 *
 */
extern ClRcT clCkptSectionIterationFinalize(CL_IN ClHandleT sectionIterationHandle);

/**
 ************************************
 *  \brief  Writes multiple sections on to a given checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle to the checkpoint that is to be written to.
 *  The handle \e checkpointHandle obtained by invoking either
 *  clCkptCheckpointOpen() or clCkptCheckpointOpenAsync() function.
 *
 *  \param ioVector Pointer to the IO Vector containing the section IDs and the data to be written.
 *  \param numberOfElements Total number of elements in \e ioVector.
 *  \param erroneousVectorIndex (out) A pointer to an index, stored in the caller's address
 *  space, of the first iovector element that makes the invocation fail. If the index is set to
 *  NULL or if the invocation succeeds, the field remains unchanged.
 *  Updated if the clCkptCheckpointWrite() call fails.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameters passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \par Description:
 *  This function writes data from the memory regions specified by \e ioVector into a checkpoint:
 *  \arg If this checkpoint is created with the \c CL_CKPT_WR_ALL_REPLICAS
 *  property, all of the checkpoint replicas gets updated when the invocation
 *  returns. If the invocation does not complete or returns with an error, nothing
 *  is written at all.
 *  \arg If the checkpoint is created with the
 *  \c CL_CKPT_WR_ACTIVE_REPLICA property, the active checkpoint replica
 *  gets updated when the invocation returns. Other checkpoint replicas are
 *  updated asynchronously. If the invocation does not complete or returns with
 *  an error, nothing is written.
 *  \arg If the checkpoint is created with the
 *  \c CL_CKPT_WR_ACTIVE_REPLICA_WEAK property, the active checkpoint
 *  replica gets updated when the invocation returns. Other checkpoint replicas
 *  are updated asynchronously. If the invocation returns with an error, nothing
 *  is written at all. However, if the invocation does not complete, the
 *  operation may be partially completed and some sections may be corrupted in
 *  the active checkpoint replica.
 *  \par
 *  In a single invocation, several sections and several portions of sections can be
 *  updated simultaneously. The elements of the \e ioVectors are written in order from
 *  \e ioVector[0] to \e ioVector [numberOfElements - 1]. As a result of this invocation, some
 *  sections may grow.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionCreate(),
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationInitialize(),
 *  clCkptSectionIterationNext(),
 *  clCkptSectionIterationFinalize(),
 *  clCkptSectionOverwrite()
 *
 */
extern ClRcT clCkptCheckpointWrite( CL_IN  ClCkptHdlT     checkpointHandle,
                             CL_IN  const ClCkptIOVectorElementT   *ioVector,
                             CL_IN  ClUint32T       numberOfElements,
                             CL_OUT ClUint32T      *erroneousVectorIndex);

/**
 ************************************
 *  \brief  Writes a single section in a given checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle that designates the checkpoint that is written to.
 *  The handle \e checkpointHandle is obtained by invoking either
 *  clCkptCheckpointOpen() or clCkptCheckpointOpenAsync() functions.
 *  \param sectionId A pointer to an identifier for the section that is to be overwritten.
 *   If this pointer points to \c CL_CKPT_DEFAULT_SECTION_ID, the default section is updated.
 *  \param dataBuffer Pointer to the buffer from where the data is being copied.
 *  \param dataSize The size in bytes of the data to be written, which becomes
 *   the new size for this section.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameters \e sectionId and \e dataBuffer
 *  passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to write a single section in a given checkpoint.
 *  This function is similar to clCkptCheckpointWrite() except that it overwrites only a
 *  single section. As a result of this invocation, the previous data and size of the section
 *  will change. This function may be invoked, even if there was no prior invocation of
 *  clCkptCheckpointWrite().
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa
 *  clCkptSectionCreate(),
 *  clCkptSectionDelete(),
 *  clCkptSectionExpirationTimeSet(),
 *  clCkptSectionIterationInitialize(),
 *  clCkptSectionIterationNext(),
 *  clCkptSectionIterationFinalize(),
 *  clCkptCheckpointWrite()
 *
 */
extern ClRcT clCkptSectionOverwrite(CL_IN  ClCkptHdlT        checkpointHandle,
                              CL_IN const ClCkptSectionIdT    *sectionId,
                              CL_IN const void             *dataBuffer,
                              CL_IN ClSizeT                 dataSize);

/**
 ************************************
 *  \brief Reads multiple sections at a time.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle The handle to the checkpoint that is to be read. The handle
 *  \e checkpointHandle obtained by invoking either
 *  clCkptCheckpointOpen() or clCkptCheckpointOpenCallback() function.
 *
 *  \param ioVector Pointer to an IO Vector containing the section IDs and the data to be written.
 *
 *  \param numberOfElements  Total number of elements in \e ioVector.
 *  Each element is of the type
 *  ClCkptIOVectorElementT, and containing the following fields:
 *  \arg sectionId: The identifier of the section to be read from.
 *  \arg dataBuffer: (in/out)  A pointer to a buffer containing the data to be read to. If
 *  \e dataBuffer is NULL, the value of \e datasize provided by the invoker is ignored
 *  and the buffer is provided by the Checkpoint Service library. The buffer must
 *  be deallocated by the invoker.
 *  \arg dataSize: Size of the data to be read to the buffer designated by
 *  \e dataBuffer. The size is at most \e maxSectionSize, as specified in the creation
 *  attributes of the checkpoint.
 *  \arg dataOffset: Offset in the section that marks the start of the data that is to
 *  be read.
 *  \arg readSize: (out) Used by clCkptCheckpointRead() to record the number of
 *  bytes of data that have been read; otherwise, this field is not used.
 *
 *  \param erroneousVectorIndex (out) Pointer to the index for errors in \e ioVector.
 *  This is an index in the caller's address space, of
 *  the first vector element that causes the invocation to fail. If the invocation succeeds,
 *  then \e erroneousVectorIndex is \c NULL and should be ignored.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  When the parameters passed are NULL pointers.
 *  \retval CL_ERR_NOT_INITIALIZED If the checkpoint server is not completely initialized.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to read multiple sections at a time. It can be used to 
 *  read a single section.
 *
 *  \par Library File:
 *  ClCkpt
 *
 */
extern ClRcT clCkptCheckpointRead( CL_IN ClCkptHdlT               checkpointHandle,
                            CL_INOUT ClCkptIOVectorElementT   *ioVector,
                            CL_IN ClUint32T              numberOfElements,
                            CL_OUT ClUint32T              *erroneousVectorIndex);

/**
 ************************************
 *  \brief Synchronizes the replicas of a checkpoint.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle Checkpoint handle returned as part of clCkptCheckpointOpen.
 *  \param  timeout Time-out to execute the operation.[It is not supported currently]
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_TRY_AGAIN System is in a transitory unstable state, please try later.
 *
 *  \par Description:
 *  This function is used to synchronize the replicas of a checkpoint.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointSynchronizeAsync()
 *
 */
extern ClRcT clCkptCheckpointSynchronize( CL_IN ClCkptHdlT      ckeckpointHandle,
                                   CL_IN ClTimeT      timeout);
/**
 ************************************
 *  \brief Synchronizes the replicas of a Checkpoint asynchronously.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle Checkpoint handle returned as part of clCkptCheckpointOpen().
 *  \param invocation Identifies the call when the callback is called.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to synchronize the replicas of a Checkpoint asynchronously.
 *
 *  \par Library File:
 *  ClCkpt
 *
 *  \sa clCkptCheckpointSynchronize()
 *
 */
extern ClRcT clCkptCheckpointSynchronizeAsync( CL_IN ClCkptHdlT checkpointHandle,
                                        CL_IN ClInvocationT invocation);
/**
 ************************************
 *  \brief Registers a callback function to be called to notify change in the checkpoint data.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param checkpointHandle Checkpoint handle returned as part of clCkptCheckpointOpen().
 *  \param 
 *  \param invocation Identifies the call when the callback is called.
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This function is used to register a callback function that is called.
 *  whenever the specified checkpoint data is updated.
 *
 *  \par Library File:
 *  ClCkpt
 *
 */
extern ClRcT clCkptImmediateConsumptionRegister( CL_IN ClCkptHdlT checkpointHandle,
                                        CL_IN ClCkptNotificationCallbackT callback,
                                        CL_IN ClPtrT pCookie);

/**
 ************************************
 *  \brief  Helps detect pending callbacks.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize()
 *   function, designating this particular initialization of the Ckpt Service.
 *
 *  \param pSelectionObject (out) A pointer to the operating system handle that
 *  the invoking EO can use to detect pending callbacks.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED On initialization failure.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *
 *  \par Description:
 *  This function returns the operating system handle, \e pSelectionObject,
 *  associated with the handle \e ckptHandle. The invoking EO can use
 *  this handle to detect pending callbacks, instead of repeatedly
 *  invoking clCkptDispatch() for this purpose.
 *  \par
 *  The \e pSelectionObject returned by this function, is a file descriptor
 *  that can be used with poll() or select() systems call detect incoming
 *  callbacks. 
 *  \par
 *  The \e selectionObject returned by this function is valid until
 *  clCkptFinalize() is invoked on the same handle \e ckptHandle.
 *
 *  \par Library File:
 *   ClCkpt
 *
 *  \sa clCkptDispatch()
 *
 */

extern ClRcT clCkptSelectionObjectGet(CL_IN ClCkptSvcHdlT ckptHandle,
                                      CL_OUT ClSelectionObjectT *selectionObject);


/**
 ************************************
 *  \brief Invokes the pending callback in context of the EO.
 *
 *  \par Header File:
 *  clCkptApi.h
 *
 *  \param ckptHandle The handle, obtained through the clCkptInitialize()
 *   function, designating this particular initialization of the Ckpt Service.
 *
 *  \param dispatchFlags Flags that specify the callback execution behavior
 *  clCkptDispatch() function, which have the values \c CL_DISPATCH_ONE,
 *  \c CL_DISPATCH_ALL or \c CL_DISPATCH_BLOCKING, as defined in clCommon.h.
 *
 *  \retval CL_OK The function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED On initialization failure.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *
 *  \par Description:
 *  This function invokes, in the context of the calling EO, pending callbacks for
 *  handle \e ckptHandle in a way that is specified by the \e dispatchFlags
 *  parameter.
 *
 *  \par Library File:
 *   ClCkpt
 *
 *  \sa clCkptSelectionObjectGet()
 *
 */
extern ClRcT clCkptDispatch(CL_IN ClCkptSvcHdlT ckptHandle,
                            CL_IN ClDispatchFlagsT dispatchFlags);

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_CKPT_API_H_*/

/**
 *  \}
 */

