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
#include <clDispatch.h>
#include <clHandleApi.h>
#include <clDebugApi.h>
#include <errno.h>
#include <string.h>

/* DataStructures */
static ClHandleDatabaseHandleT  databaseHandle = CL_HANDLE_INVALID_VALUE;

static const ClCharT    pipeNotifyChar = 'c';
/* Function Definitions */

/* 
 * clDispatchLibInitialize function initiates the dispatch library. The
 * handle databse is created here. It is called only once in the life
 * time of the application, and hence it is expected to be called by
 * EO 
 */
ClRcT   clDispatchLibInitialize(void)
{
    ClRcT   rc = CL_OK;

    if (databaseHandle != CL_HANDLE_INVALID_VALUE)
    {
        return CL_ERR_INITIALIZED;
    }

    /* Create the handle database */
    rc = clHandleDatabaseCreate(clDispatchHandleDestructor,
                                &databaseHandle);
    return rc;
}

/*
 * clDispatchLibFinalize function finalizes the dispatch library. The
 * handle databse will be destroyed here. The handleDatabaseHandle is
 * marked for delete, and will get destroyed when all the handles are
 * destroyed and the number of used handles count goes to 0.
 * This is expected to be called in EO finalize
 */

ClRcT   clDispatchLibFinalize(void)
{
    ClRcT   rc = CL_OK;

    CHECK_LIB_INIT;

    rc = clHandleDatabaseDestroy(databaseHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleDatabaseDestroy failed with rc 0x%x\n",rc));
    }
    databaseHandle = CL_HANDLE_INVALID_VALUE;
    return rc;
}


/*
 * clDispatchRegister create a handle for this particular registration
 * of service client library, and returns the handle which should be
 * used for subsequent invocation of dispatch APIs.
 */

ClRcT   clDispatchRegister(
        CL_OUT  ClHandleT*                      pDispatchHandle,
        CL_IN   ClHandleT                       svcInstanceHandle,
        CL_IN   ClDispatchCallbackT             wrapperCallback,
        CL_IN   ClDispatchQueueDestroyCallbackT queueDestroyCallback)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;
    ClFdT   fds[2] = {0};

    if (svcInstanceHandle == CL_HANDLE_INVALID_VALUE)
    {
        return CL_ERR_INVALID_PARAMETER;
    }

    if ((wrapperCallback == NULL) || (pDispatchHandle == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CHECK_LIB_INIT;

    /* Create the handle for this initialization of the library */
    rc = clHandleCreate (databaseHandle,
                         sizeof(ClDispatchDbEntryT),
                         pDispatchHandle);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT (*pDispatchHandle != CL_HANDLE_INVALID_VALUE);

    /* Checkout the handle */
    rc = clHandleCheckout(databaseHandle, *pDispatchHandle, (void *)&thisDbEntry);

    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT (thisDbEntry != NULL);

    /* Store SVC instance handle */
    thisDbEntry->svcInstanceHandle = svcInstanceHandle;
    thisDbEntry->svcCallback = wrapperCallback;
    thisDbEntry->queueDestroyCallback = queueDestroyCallback;

    /* Create the queue */
    rc = clQueueCreate(0x0,
                       clDispatchQueueDequeueCallback,
                       clDispatchQueueDestroyCallback,
                       &thisDbEntry->cbQueue);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    /* Create Mutex to protect the queue */
    rc = clOsalMutexCreate(&thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    errno = 0;
    /* Create the pipe */
    ClInt32T ec = pipe(fds); // since return code for system call can be -ve
    if (ec < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Unable to create pipe: %s",strerror(errno)));
        rc = CL_ERR_LIBRARY;
        goto error_return;
    }
    /* 
     * Reinitialize rc to CL_OK as it would have changed with
     * above assignment
     */
    rc = CL_OK;

    thisDbEntry->readFd = fds[0];
    thisDbEntry->writeFd = fds[1];

    thisDbEntry->shouldDelete = CL_FALSE;

error_return:
    if ((clHandleCheckin(databaseHandle, *pDispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleCheckin failed"));
    }
    return rc;
}


/*
 * clDispatchDeregister marks the handle for delete, which was created using
 * clDispatchRegister. It also wakes up any sleeping thread on SelectionObject
 * by writing a dummy character to the pipe
 */
ClRcT   clDispatchDeregister(
        CL_IN   ClHandleT           dispatchHandle)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;
    ClCharT     ch = '\0';

    CHECK_LIB_INIT;

    rc = clHandleCheckout(databaseHandle, dispatchHandle, (void *)&thisDbEntry);
    if (rc != CL_OK)
    {
        return CL_ERR_INVALID_HANDLE;
    }
    CL_ASSERT(thisDbEntry != NULL);

    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        /* Handle checkin and return */
        if ((clHandleCheckin(databaseHandle, dispatchHandle)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clHandleCheckin failed"));
        }
        return rc;
    }

    thisDbEntry->shouldDelete = CL_TRUE;

    /* 
     * Write a dummy character on the pipe. This is to unblock any
     * waiting thread on the pipe
     */
    errno = 0;
    if (write(thisDbEntry->writeFd ,(void*)&ch, 1) < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("write into pipe failed: %s",strerror(errno)));
        rc = CL_ERR_LIBRARY;
    }

    if ((clOsalMutexUnlock(thisDbEntry->dispatchMutex)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Mutex unlock failed"));
    }

    if ((clHandleCheckin(databaseHandle, dispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleCheckin failed"));
    }

    if ((clHandleDestroy(databaseHandle, dispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleDestroy failed"));
    }

    return rc;
}

/* 
 * clDispatchSelectionObject returns the selection object [readFd] associated
 * with this particular initialization of the dispatch library 
 */
ClRcT   clDispatchSelectionObjectGet(
        CL_IN   ClHandleT           dispatchHandle,
        CL_OUT  ClSelectionObjectT* pSelectionObject)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;

    if (pSelectionObject == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    CHECK_LIB_INIT;

    rc = clHandleCheckout(databaseHandle, dispatchHandle, (void *)&thisDbEntry);
    if (rc != CL_OK)
    {
        return CL_ERR_INVALID_HANDLE;
    }
    CL_ASSERT(thisDbEntry != NULL);

    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    if (thisDbEntry->shouldDelete == CL_TRUE)
    {
        rc = CL_ERR_INVALID_HANDLE;
        goto error_unlock_return;
    }
    
    *pSelectionObject = (ClSelectionObjectT)thisDbEntry->readFd;

error_unlock_return:
    rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Mutex Unlock failed with rc = 0x%x\n",rc));
    }

error_return:
    if ((clHandleCheckin(databaseHandle, dispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleCheckin failed"));
    }

    return rc;
}


/*
 * clDispatchCbDispatch will dequeue the pending callback and invokes the
 * service client specific generic callback. The service client should make
 * sure that it handles the invocation of the associated callback.
 */
ClRcT   clDispatchCbDispatch(
        CL_IN   ClHandleT           dispatchHandle,
        CL_IN   ClDispatchFlagsT     dispatchFlag)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;
    ClUint32T   queueSize = 0;
    ClDispatchCbQueueDataT*   queueData = NULL;
    ClCharT ch = pipeNotifyChar;

    if ((dispatchFlag != CL_DISPATCH_ONE) &&
            (dispatchFlag != CL_DISPATCH_ALL) &&
            (dispatchFlag != CL_DISPATCH_BLOCKING))
    {
        return CL_ERR_INVALID_PARAMETER;
    }

    CHECK_LIB_INIT;

    rc = clHandleCheckout(databaseHandle, dispatchHandle, (void *)&thisDbEntry);
    if (rc != CL_OK)
    {
        return CL_ERR_INVALID_HANDLE;
    }
    CL_ASSERT(thisDbEntry != NULL);

    /* Lock the mutex */
    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    if (thisDbEntry->shouldDelete == CL_TRUE)
    {
        rc = CL_ERR_INVALID_HANDLE;
        goto error_unlock_return;
    }

    switch (dispatchFlag)
    {
        case CL_DISPATCH_ONE:
            {
                rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
                if (rc != CL_OK)
                {
                    goto error_unlock_return;
                }

                if (queueSize < 1)
                {
                    /* Dont return any error. So rc = CL_OK */
                    goto error_unlock_return;
                }

                /* Dequeue the node */
                rc = clQueueNodeDelete(thisDbEntry->cbQueue, (ClQueueDataT*)&queueData);
                if (rc != CL_OK)
                {
                    goto error_unlock_return;
                }
                CL_ASSERT(queueData != NULL);

                /* Read a character from the pipe */
                errno = 0;
                if ((read(thisDbEntry->readFd, (void*)&ch, 1)) < 1)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("read error on pipe: %s",strerror(errno)));
                }
                            
                rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
                if (rc != CL_OK)
                {
                    goto error_return;
                }

                /* Invoke the generic callback */
                thisDbEntry->svcCallback(thisDbEntry->svcInstanceHandle,
                                         queueData->callbackType,
                                         queueData->callbackArgs);
                clHeapFree(queueData);
            }
            /* Dont do break, as we have already unlocked the mutex */
            goto error_return;
        case CL_DISPATCH_ALL:
            {
                rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
                if (rc != CL_OK)
                {
                    goto error_unlock_return;
                }

                while (queueSize != 0)
                {
                    rc = clQueueNodeDelete(thisDbEntry->cbQueue, (ClQueueDataT*)&queueData);
                    if (rc != CL_OK)
                    {
                        goto error_unlock_return;
                    }
                    CL_ASSERT(queueData != NULL);

                    /* Read the byte */
                    errno = 0;
                    if ((read(thisDbEntry->readFd,(void*)&ch,1)) < 1)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Read error on the pipe: %s",strerror(errno)));
                    }

                    rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    /* Invoke the generic callback */
                    thisDbEntry->svcCallback(thisDbEntry->svcInstanceHandle,
                                             queueData->callbackType,
                                             queueData->callbackArgs);

                    /* Free the queueData */
                    clHeapFree(queueData);

                    /* Get the lock again */
                    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    if (thisDbEntry->shouldDelete == CL_TRUE)
                    {
                        rc = CL_ERR_INVALID_HANDLE;
                        goto error_unlock_return;
                    }

                    rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
                    if (rc != CL_OK)
                    {
                        goto error_unlock_return;
                    }
                } /* End of while */

            }
            /* Just break, as we need to unlock and return */
            break;
        case CL_DISPATCH_BLOCKING:
            {
                while (1)
                {
                    rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    /* Block on the readFd */
                    errno = 0;
                    if ((read(thisDbEntry->readFd,(void*)&ch,1)) < 1)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Read error on the pipe: %s",strerror(errno)));
                        /* 
                         * This might be because finalize has been done and write fd
                         * is closed. So return
                         */
                        goto error_return;
                    }

                    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    if (thisDbEntry->shouldDelete == CL_TRUE)
                    {
                        rc = CL_OK;
                        goto error_unlock_return;
                    }

                    rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
                    if (rc != CL_OK)
                    {
                        goto error_unlock_return;
                    }

                    if (queueSize < 1)
                    {
                        /* 
                         * This would happen when read has come out due
                         * to signal
                         */
                        continue;
                    }

                    /* Dequeue the node */
                    rc = clQueueNodeDelete(thisDbEntry->cbQueue, (ClQueueDataT*)&queueData);
                    if (rc != CL_OK)
                    {
                        goto error_unlock_return;
                    }
                    CL_ASSERT(queueData != NULL);

                    rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    /* Invoke the generic callback */
                    thisDbEntry->svcCallback(thisDbEntry->svcInstanceHandle,
                            queueData->callbackType,
                            queueData->callbackArgs);

                    /* Free the queueData */
                    clHeapFree(queueData);

                    /* Get the lock again */
                    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
                    if (rc != CL_OK)
                    {
                        goto error_return;
                    }

                    if (thisDbEntry->shouldDelete == CL_TRUE)
                    {
                        rc = CL_ERR_INVALID_HANDLE;
                        goto error_unlock_return;
                    }
                }
                break;
            }
    } /* End of switch */

error_unlock_return:
    if ((clOsalMutexUnlock(thisDbEntry->dispatchMutex)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Mutex Unlock failed\n"));
    }

error_return:
    if ((clHandleCheckin(databaseHandle, dispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleCheckin failed"));
    }

    return rc;
}


/*
 * clDispatchCbEnqueue enqueues the given callback data into the queue
 * associated with this particular registration of the dispatch library.
 * This API can be used by the service client to enqueue the callbacks,
 * and other thread can invoke clDispatchCbDispatch to dispatch the
 * enqueued callbacks.
 */
ClRcT   clDispatchCbEnqueue(
        CL_IN   ClHandleT           dispatchHandle,
        CL_IN   ClUint32T           callbackType,
        CL_IN   void*               callbackArgs)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;
    ClDispatchCbQueueDataT* queueData = NULL;
    ClCharT ch = pipeNotifyChar;

    CHECK_LIB_INIT;

    /* Checkout the handle */
    rc = clHandleCheckout(databaseHandle, dispatchHandle, (void *)&thisDbEntry);
    if (rc != CL_OK)
    {
        return CL_ERR_INVALID_HANDLE;
    }
    CL_ASSERT(thisDbEntry != NULL);

    /* Lock the mutex on the handle */
    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    /* Check if the handle is already finalized */
    if (thisDbEntry->shouldDelete == CL_TRUE)
    {
        clOsalMutexUnlock(thisDbEntry->dispatchMutex);
        rc = CL_ERR_INVALID_HANDLE;
        goto error_return;
    }

    queueData = clHeapAllocate(sizeof(ClDispatchCbQueueDataT));
    queueData->callbackType = callbackType;
    queueData->callbackArgs = callbackArgs;

    /* Insert the queue node */
    rc = clQueueNodeInsert(thisDbEntry->cbQueue,(ClQueueDataT)queueData);
    if (rc != CL_OK)
    {
        if (clOsalMutexUnlock(thisDbEntry->dispatchMutex) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clOsalMutexUnlock failed"));
        }
        goto error_return;
    }

    /* 
     * Write a character into the pipe so that the reader thread is
     * awoken 
     */
    errno = 0;
    if (write(thisDbEntry->writeFd ,(void*)&ch, 1) < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("write into pipe failed: %s",strerror(errno)));
        rc = CL_ERR_UNSPECIFIED;
        /* FIXME :Dequeue the last node inserted */

    }

    /* Unlock the mutex */
    if (clOsalMutexUnlock(thisDbEntry->dispatchMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clOsalMutexUnlock failed"));
    }

error_return:
    /* Checkin the handle */
    if ((clHandleCheckin(databaseHandle, dispatchHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHandleCheckin failed"));
    }

    return rc;
}


/* Handle destructor function. */
static void clDispatchHandleDestructor(void* cbArgs)
{
    ClRcT   rc = CL_OK;
    ClDispatchDbEntryT* thisDbEntry = NULL;
    ClUint32T   queueSize = 0;
    ClDispatchCbQueueDataT*   queueData = NULL;

    if (cbArgs == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Handle destructor is called with NULL pointer"));
        return;
    }

    thisDbEntry = (ClDispatchDbEntryT*)cbArgs;

    /* Lock the queue mutex */
    rc = clOsalMutexLock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Unable to lock dispatch Mutex in Handle destructor callback"));
        return;
    }

    /*
     * Before deleting the queue you need to flush the queue.
     * Even though clQueueDelete will flush the node, we need
     * handle it explicitly so that we can invoke
     * ClDispatchQueueDestroyCallbackT registered during Register
     * which will deallocate the memory for the callback arguments.
     */
    rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
    if (rc != CL_OK)
    {
        goto proceed_other_functions;
    }

    while (queueSize != 0)
    {
        rc = clQueueNodeDelete(thisDbEntry->cbQueue, (ClQueueDataT*)&queueData);
        if (rc != CL_OK)
        {
            goto proceed_other_functions;
        }
        CL_ASSERT(queueData != NULL);

        /* Invoke the queue destroy callback function */
        thisDbEntry->queueDestroyCallback(queueData->callbackType,
                                          queueData->callbackArgs);

        rc = clQueueSizeGet(thisDbEntry->cbQueue, &queueSize);
        if (rc != CL_OK)
        {
            goto proceed_other_functions;
        }
    }

proceed_other_functions:
    /* 
     * Delete the queue. This will also flush the queue. So all the
     * pending callbacks will be flushed.
     */
    rc = clQueueDelete(&thisDbEntry->cbQueue);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Unable to delete the queue. Rc = 0x%x",rc));
    }
        
    /* Delete the pipe */
    errno = 0;
    if ((close(thisDbEntry->readFd)) < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Unable to close write fd of the pipe:%s",strerror(errno)));
    }

    errno = 0;
    if ((close(thisDbEntry->writeFd)) < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Unable to close read fd of the pipe:%s",strerror(errno)));
    }


    /* Delete the mutex */
    rc = clOsalMutexUnlock(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Failed to unlock the dispatch mutex"));
    }
    rc = clOsalMutexDelete(thisDbEntry->dispatchMutex);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Failed to delete the dispatch mutex"));
    }


    return;
}

/* 
 * Queue Destroy Callback:-
 * This callback is called when QueueDelete is called. This is
 * called for every node in the queu when QueueDelete is called
 * So it needs to delete the user data
 */
static void clDispatchQueueDestroyCallback(
        const ClQueueDataT queueData)
{
    ClDispatchCbQueueDataT *pQueueData = (ClDispatchCbQueueDataT *)queueData;
    if (pQueueData == NULL)
    {
        return;
    }

    clHeapFree(pQueueData);
    return;
}

static void clDispatchQueueDequeueCallback(
        const ClQueueDataT queueData)
{
    /* 
     * The queueData will be explicitly freed in the
     * caller function. So not doing anything here.
     */
    return;
}
