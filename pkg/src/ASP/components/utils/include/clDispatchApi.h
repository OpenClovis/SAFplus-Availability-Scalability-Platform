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
#include <clCommon.h>
#include <clCommonErrors.h>

#ifndef _CL_DISPATCH_API_H_
#define _CL_DISPATCH_API_H_
                                                                                                                             
# ifdef __cplusplus
extern "C"
{
# endif /* endif for __cplusplus */

/* 
 * Prototype for the service client specified generic callback
 * function. This function will be invoked from dispatch, and
 * the service client in-turn needs to handle the invocation of
 * specific callback based on the callbackType.
 */
typedef void (*ClDispatchCallbackT) (
        /* 
         * The instanceHandle of the service client for this 
         * initialization of dispatch library.
         */
        ClHandleT svcInstanceHandle,

        /* 
         * Service client specific callback type, which should be used
         * by this wrapperCallback to invoke the specific callback 
         */
        ClUint32T callbackType,

        /* 
         * The argument list for the service client specific callback
         */
        void*     callbackData);


/*
 * Prototype for the callback function which will be called 
 * during handle destroy. During finalize, the pending callbacks
 * will be flushed, and hence at that time, the callback arguments
 * to the queue node provided during enqueue need to be deallocated.
 * Dispatch will invoke this callback while flushing the queue, and 
 * inside the callback function the user should take care to deallocate
 * the callback arguments.
 * This callback should be provided during clDispatchRegister.
 */
typedef void (*ClDispatchQueueDestroyCallbackT) (
        /*
         * Service client specific callback type, which should be used
         * by this wrapperCallback to invoke the specific callback
         */
        ClUint32T callbackType,

        /*
         * The argument list for the service client specific callback
         */
        void*     callbackData);


/* Data structure which will be used as the queue node */
typedef struct ClDispatchCbQueueData {
    /* 
     * Callback type used by service client to identify
     * specific callback to be invoked
     */
    ClUint32T callbackType;
    /* 
     * Arguments for the callback to be invoked 
     */
    void      *callbackArgs;
} ClDispatchCbQueueDataT;


/* 
 * clDispatchLibInitialize function initializes the dispatch library. The
 * handle database is created here. It is called only once in the life
 * time of the application, and hence it is expected to be called by
 * EO 
 */
extern  ClRcT   clDispatchLibInitialize(void);


/* 
 * clDispatchLibFinalize function finalizes the dispatch library. The
 * handle databse will be destroyed here. The handleDatabaseHandle is
 * marked for delete, and will get destroyed when all the handles are
 * destroyed and the number of used handles count goes to 0.
 * This is expected to be called in EO finalize 
 */
extern  ClRcT   clDispatchLibFinalize(void);



/* 
 * clDispatchRegister create a handle for this particular registration
 * of service client library, and returns the handle which should be
 * used for subsequent invocation of dispatch APIs.
 */
extern  ClRcT   clDispatchRegister(
        /* 
         * A pointer to the handle designating this particular 
         * registration of the Dispatch library that is to be returned 
         * by the Dispatch Library. 
         */
        CL_OUT  ClHandleT*                      pDispatchHandle,

        /* Service client library handle for the given service */
        CL_IN   ClHandleT                       svcInstanceHandle,

        /* 
         * Function pointer to the generic callback provided by a 
         * library client, Whenever a new callback is to be delivered 
         * to the application, this callback is invoked on the library 
         * client. This function will in turn call the application 
         * callback after decoding the parameters. 
         */
        CL_IN   ClDispatchCallbackT             wrapperCallback,

        /*
         * DispatchQueueDestroyCallback which will deallocate the
         * callback arguments provided during enqueue.
         */
        CL_IN   ClDispatchQueueDestroyCallbackT queueDestroyCallback);




/* 
 * clDispatchDeregister marks the handle for delete, which was created using
 * clDispatchRegister. It also wakes up any sleeping thread on SelectionObject
 * by writing a dummy character to the pipe
 */
extern  ClRcT   clDispatchDeregister(
        /* 
         * Handle corresponding to the instance of the dispatch library 
         * to be deregistered. This must be allocated through a previous 
         * call to clDispatchRegister.
         */
        CL_IN   ClHandleT           dispatchHandle);



/* 
 * clDispatchSelectionObject returns the selection object [readFd] associated
 * with this particular registration of the dispatch library 
 */
extern  ClRcT   clDispatchSelectionObjectGet(
        /* 
         * Handle for which the selection object is being queried. This 
         * handle must be allocated through a previous call to 
         * clDispatchRegister. 
         */
        CL_IN   ClHandleT           dispatchHandle,

        /* 
         * The pointer to the memory location where the value of the 
         * readFd of the pipe will be stored and returned. 
         */
        CL_OUT  ClSelectionObjectT* pSelectionObject);


/* 
 * clDispatchCbDispatch will dequeue the pending callback and invokes the
 * service client specific generic callback. The service client should make 
 * sure that it handles the invocation of the associated callback.
 */
extern  ClRcT   clDispatchCbDispatch(
        /* 
         * Handle for which the callback(s) should be dispatched. 
         * This handle must be allocated through a previous call to 
         * clDispatchRegister.
         */
        CL_IN   ClHandleT           dispatchHandle,

        /*
         * Any of CL_DISPATCH_ONE, CL_DISPATCH_ALL or CL_DISPATCH_BLOCKING 
         */
        CL_IN   ClDispatchFlagsT     dispatchFlag);



/* 
 * clDispatchCbEnqueue enqueues the given callback data into the queue 
 * associated with this particular registration of the dispatch library.
 * This API can be used by the service client to enqueue the callbacks, 
 * and other thread can invoke clDispatchCbDispatch to dispatch the 
 * enqueued callbacks. 
 */
extern  ClRcT   clDispatchCbEnqueue(
        /* 
         * Handle for which the callback(s) should be enqueued. This handle
         * must be allocated through a previous call to clDispatchRegister.
         */
        CL_IN   ClHandleT           dispatchHandle,

        /* 
         * 32 bit unsigned integer value to denote the particular callback 
         * type which has to be invoked from the svcCallback 
         */
        CL_IN   ClUint32T           callbackType,

        /* 
         * Pointer to memory location holding the arguments to be passed to 
         * the given callback. 
         */
        CL_IN   void*               callbackArgs);

# ifdef __cplusplus
}
# endif /* endif for __cplusplus */
                                                                                                                             
#endif  /* endif for ifndef _CL_DISPATCH_API_H_ */
