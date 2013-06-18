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
 * ModuleName  : utils
 * File        : clHandleApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the implementation of a client side handle management service.
 *
 * Part of this code was inspired by the openais code, hence I retained
 * the copyright below.
 *****************************************************************************/

/*
 * Copyright (c) 2002-2004 MontaVista Software, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clDbg.h>
#include <clOsalApi.h>
#include <clLogApi.h>

#include <clHandleApi.h>
#include <clHandleErrors.h>
#include <ipi/clHandleIpi.h>

#define nullChkRet(ptr) clDbgIfNullReturn(ptr, CL_CID_HANDLE);

#define CL_HDL_NUM_HDLS_BUNCH           8
#define CL_HDL_INVALID_COOKIE           0xDEADBEEF




const ClCharT  *CL_HDL_VALID_DB     = "clHandleDBValid";
static ClUint32T handleDbId         = 1;


#define CL_HDL_AREA           "HDL"
#define CL_HDL_CTX_CREATE     "HLC"
#define CL_HDL_CTX_DBCREATE   "HBC"
#define CL_HDL_CTX_CHECKOUT   "HCO"
#define CL_HDL_CTX_CHECKIN    "HCI"
#define CL_HDL_CTX_DESTROY    "HLD"
#define CL_HDL_CTX_DBDESTROY  "HBD"

#define hdlValidityChk(hdl,hdbp) \
    do                                      \
    {                                       \
        if( CL_HDL_DB(hdl) != hdbp->id)                  \
        {                                   \
            clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, "Handle does not belong to this handle database"); \
            return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);    \
        }                                                  \
    }while(0)                                                                      \


#define hdlDbValidityChk(hdbp)                \
    do                                      \
    {                                       \
        if( NULL == hdbp )                  \
        {                                   \
            clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED,  \
                       "Passed NULL for database handle"); \
            return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);    \
        }                                                  \
        if( *((ClCharT **) hdbp) != CL_HDL_VALID_DB )      \
        {                                                  \
            if( *((ClCharT **) hdbp) == (ClCharT *) CL_HDL_INVALID_COOKIE ) \
            {                                                   \
                clLogError(CL_HDL_AREA, CL_LOG_CONTEXT_UNSPECIFIED, \
                           "Handle database [%p] has already been destroyed",\
                           (ClPtrT) hdbp);                                   \
                return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);                  \
            }                                                                \
            clLogCritical(CL_HDL_AREA, CL_LOG_CONTEXT_UNSPECIFIED,           \
                 "This DB handle [%p] is corrupt, MD check failed", (ClPtrT) hdbp);\
            return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);                            \
        }                                                                          \
    }while(0)                                                                      \
            
/******************************************************************************
 * LOCAL TYPES AND VARIABLES
 *****************************************************************************/


/******************************************************************************
 * API FUNCTIONS
 *****************************************************************************/

/*
 * Creates a new handle database.  User is responsible to cleanup and free
 * database.
 */
ClRcT
clHandleDatabaseCreate(
    void                    (*destructor)(void*),
    ClHandleDatabaseHandleT  *databaseHandle)
{
    ClHdlDatabaseT *hdbp = NULL;
    
    nullChkRet(databaseHandle);

    hdbp = (ClHdlDatabaseT*) clHeapCalloc(1, sizeof(ClHdlDatabaseT));
    if (NULL == hdbp)
    {
        clLogError(CL_HDL_AREA, CL_HDL_CTX_DBCREATE, 
                   "Memory allocation failed");
        return CL_HANDLE_RC(CL_ERR_NO_MEMORY);
    }
    
    (void)pthread_mutex_init(&hdbp->mutex, NULL); /* This always returns 0 */
    if (destructor != NULL)
    {
        hdbp->handle_instance_destructor = destructor;
    }
    hdbp->pValidDb = (void *) CL_HDL_VALID_DB;

    hdbp->id = handleDbId++;
    
    /*
     * Database handle is obtained from memory address here. This is OK,
     * since (1) handle type is larger or same size as address, (2) the
     * use of handle is limited to one process.
     */
    *databaseHandle = hdbp;
#if 0
    clLogTrace(CL_HDL_AREA, CL_HDL_CTX_DBCREATE, 
             "Database [%p] has been created", (ClPtrT) hdbp);
#endif
    clDbgResourceNotify(clDbgHandleGroupResource, clDbgAllocate, 0, hdbp, ("Handle database %p allocated", (ClPtrT) hdbp));
    
    return CL_OK;
}

/*
 * Frees up the handle database specified by databaseHandle.  If the handle
 * database is empty, it frees up the database immediately.  Otherwise,
 * it will mark the database for deletion and free it when the last entry
 * is deleted.
 */
ClRcT
clHandleDatabaseDestroy(ClHandleDatabaseHandleT databaseHandle)
{
    ClHdlDatabaseT *hdbp   = (ClHdlDatabaseT*)databaseHandle;
    ClRcT          ec      = CL_OK;
    ClHandleT      handle  = 0;
    
    hdlDbValidityChk(hdbp);
    
    ec = pthread_mutex_lock(&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }
    clDbgResourceNotify(clDbgHandleGroupResource, clDbgRelease, 0, hdbp, ("Handle database %p released", (ClPtrT) hdbp));
    /*
     * Go thru the list of handles and delete everything, if they have not
     * been cleaned up properly, through warning message and destroy the
     * database
     */
    if (hdbp->n_handles_used > 0) /* database is not empty */
    {
       for( handle = 0; handle < hdbp->n_handles; handle++)
       {
           if( hdbp->handles[handle].state != HANDLE_STATE_EMPTY )
           {
               /* explicitly making '0' for smooth removal of handles */
               hdbp->handles[handle].ref_count = 1;
               hdbp->handles[handle].state     = HANDLE_STATE_PENDINGREMOVAL;
               ec = pthread_mutex_unlock(&hdbp->mutex);
               if( ec != 0 )
               {
                   /* Who cares about the error code, during shut down */
                   goto free_exit;          
               }
               clLogWarning(CL_HDL_AREA, CL_HDL_CTX_DBDESTROY, 
                            "Handle [%p:%#llX] has not been cleaned, destroying...",
                            (ClPtrT) hdbp, (handle + 1));
               clHandleCheckin(databaseHandle, handle + 1);
               ec = pthread_mutex_lock(&hdbp->mutex);
               if( ec != 0 )
               {
                   goto free_exit;
               }
           }
       }
    }
    /* Explicitly not checking the error code */    
    pthread_mutex_unlock(&hdbp->mutex);
free_exit:    
    if( NULL != hdbp->handles )
    {
        free(hdbp->handles); 
    }
    hdbp->pValidDb = (void *) CL_HDL_INVALID_COOKIE; 
    clHeapFree(hdbp);
    return CL_OK;
}

ClRcT
clHandleAdd (ClHandleDatabaseHandleT databaseHandle,  void* instance, ClHandleT* handle_out)
{
  ClHandleT      handle       = 0;
  ClHdlEntryT    *new_handles = NULL;
  ClBoolT        found        = CL_FALSE;
  ClRcT          rc           = CL_OK;
  ClHdlDatabaseT *hdbp        = (ClHdlDatabaseT*) databaseHandle;

  nullChkRet(handle_out);
  hdlDbValidityChk(hdbp);

  rc = pthread_mutex_lock (&hdbp->mutex);
  if (rc != 0)
    {
      return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }

  for (handle = 0; handle < hdbp->n_handles; handle++) 
    {
      if (hdbp->handles[handle].state == HANDLE_STATE_EMPTY) 
        {
          found = 1;
          break;
        }
    }

  if (found == 0) 
    {
      new_handles = (ClHdlEntryT *) realloc ( hdbp->handles, 
                                    sizeof (ClHdlEntryT) *
                              (hdbp->n_handles + CL_HDL_NUM_HDLS_BUNCH));
      if (new_handles == NULL)
        {
          rc = pthread_mutex_unlock (&hdbp->mutex);
          if (rc != 0)
            {
              return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be very bad */
            }
          return CL_HANDLE_RC(CL_ERR_NO_MEMORY);
        }
      memset(&new_handles[hdbp->n_handles], '\0', 
             sizeof(ClHdlEntryT) * CL_HDL_NUM_HDLS_BUNCH);
      hdbp->n_handles += CL_HDL_NUM_HDLS_BUNCH;
      hdbp->handles    = new_handles;
    }

  hdbp->handles[handle].state     = HANDLE_STATE_USED;
  hdbp->handles[handle].instance  = instance;
  hdbp->handles[handle].ref_count = 1;
  hdbp->handles[handle].flags     = 0;
    
  hdbp->n_handles_used++;

  /*
   * Adding 1 to handle to ensure the non-zero handle interface.
   */
  *handle_out = CL_HDL_MAKE(ASP_NODEADDR,hdbp->id, handle + 1);

  clDbgResourceNotify(clDbgHandleResource, clDbgAllocate, hdbp, handle+1, ("Handle [%p:%#llX] allocated", (ClPtrT)hdbp, handle+1));
  
  rc = pthread_mutex_unlock (&hdbp->mutex);
  if (rc != 0)
    {
      return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
#if 0
  clLogTrace(CL_HDL_AREA, CL_HDL_CTX_CREATE, 
             "Handle [%p:%#llX] has been created",
             (ClPtrT) hdbp, (handle + 1));
#endif
  return rc;
}



ClRcT
clHandleCreate (
                ClHandleDatabaseHandleT databaseHandle,
                ClInt32T instance_size,
                ClHandleT *handle_out)
{
  ClWordT        idx       = 0;
  void           *instance    = NULL;
  ClRcT          rc           = CL_OK;
  ClHdlDatabaseT *hdbp        = (ClHdlDatabaseT*) databaseHandle;

  nullChkRet(handle_out);
  hdlDbValidityChk(hdbp);
  
  instance = clHeapCalloc (1, instance_size);
  if (instance == NULL) 
    {
      rc = CL_HANDLE_RC(CL_ERR_NO_MEMORY);
      return rc;
    }

  rc =  clHandleAdd (databaseHandle, instance, handle_out);
  idx = CL_HDL_IDX(*handle_out)-1;
  
  hdbp->handles[idx].flags = HANDLE_ALLOC_FLAG;
  
  if (rc != CL_OK)
  {
      clHeapFree(instance);
  }  
  return rc;
}

ClRcT
clHandleCreateSpecifiedHandle (
    ClHandleDatabaseHandleT databaseHandle,
	ClInt32T instance_size,
    ClHandleT handle)
{
	void           *instance = NULL;
    ClRcT          rc        = CL_OK;
    ClHdlDatabaseT *hdbp     = (ClHdlDatabaseT*) databaseHandle;
    ClRcT          ec        = CL_OK;

    hdlDbValidityChk(hdbp);
    /* We allow handles to be created that are pointing to some other handle DB and node hdlValidityChk(handle,hdbp); */
    handle = CL_HDL_IDX(handle); /* once we've verified it, we only care about the index */

    if (CL_HANDLE_INVALID_VALUE == handle)
    {
        clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Passed handle [%p:%#llX] is an invalid handle",
                   (ClPtrT) hdbp, handle);
        clDbgPause();
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    handle--;

    ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }
    if(handle >= hdbp->n_handles)
    {
        /*
         * Allocating space in Excess of to accomodate the value specified by the user.
         * NOTE: User should ensure that a sane value is supplied for handle.
         */
        ClHandleT excess_handles = handle - hdbp->n_handles + 1;

		ClHdlEntryT *new_handles = (ClHdlEntryT *)realloc (hdbp->handles,
			sizeof (ClHdlEntryT) * (hdbp->n_handles+excess_handles));
		if (new_handles == 0) 
        {
			ec = pthread_mutex_unlock (&hdbp->mutex);
            if (ec != 0)
            {
                return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be very bad */
            }
			return CL_HANDLE_RC(CL_ERR_NO_MEMORY);
		}

        /* 
         * Initialize the excess space. HANDLE_STATE_EMPTY will remain 0 
         * is assumed else need to set the excess entries explicitly in a 
         * loop. 
         */
        memset(&new_handles[hdbp->n_handles], 0, sizeof (ClHdlEntryT) * excess_handles);

        /* Update the values if success */
		hdbp->n_handles += excess_handles;
		hdbp->handles = new_handles;
	}

    if (hdbp->handles[handle].state != HANDLE_STATE_EMPTY) {
        /*
         * The specified handle already in use so return the specific error.
         */
        rc = CL_HANDLE_RC(CL_ERR_ALREADY_EXIST);
        goto error_exit;
    }

	instance = clHeapCalloc (1, instance_size);
	if (instance == 0) {
		rc = CL_HANDLE_RC(CL_ERR_NO_MEMORY);
        goto error_exit;
	}
	memset (instance, 0, instance_size);
	hdbp->handles[handle].state = HANDLE_STATE_USED;
	hdbp->handles[handle].instance = instance;
	hdbp->handles[handle].ref_count = 1;

    clDbgResourceNotify(clDbgHandleResource, clDbgAllocate, hdbp, handle+1, ("Specific handle [%p:%#llX] allocated", (ClPtrT) hdbp, handle+1));
    hdbp->n_handles_used++;
error_exit:

	ec = pthread_mutex_unlock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
	return rc;
}

ClRcT
clHandleMove(
    ClHandleDatabaseHandleT databaseHandle,
    ClHandleT oldHandle,
    ClHandleT newHandle)
{
	void           *instance = NULL;
    ClRcT          rc        = CL_OK;
    ClHdlDatabaseT *hdbp     = (ClHdlDatabaseT*) databaseHandle;
    ClRcT          ec        = CL_OK;

    hdlDbValidityChk(hdbp);
    /* We allow handles to be created that are pointing to some other handle DB and node hdlValidityChk(handle,hdbp); */
    oldHandle = CL_HDL_IDX(oldHandle); /* once we've verified it, we only care about the index */
    newHandle = CL_HDL_IDX(newHandle);
    
    if (CL_HANDLE_INVALID_VALUE == oldHandle 
        ||
        CL_HANDLE_INVALID_VALUE == newHandle)
    {
        clLogError("HDL", "MOVE",
                   "Passed [%s] handle is invalid",
                   oldHandle ? "new" : "old");
        clDbgPause();
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    --oldHandle;
    --newHandle;
    ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }

    if(oldHandle >= hdbp->n_handles)
    {
        pthread_mutex_unlock(&hdbp->mutex);
        clLogError("HDL", "MOVE", "Old handle [%#llx] passed is invalid", oldHandle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }

    if(hdbp->handles[oldHandle].ref_count != 1)
    {
        pthread_mutex_unlock(&hdbp->mutex);
        clLogError("HDL", "MOVE", "Old handle [%#llx] is %s", 
                   oldHandle, hdbp->handles[oldHandle].ref_count < 1 ? "invalid" : "in use");
        return CL_HANDLE_RC(CL_ERR_INVALID_STATE);
    }
    
    if(newHandle < hdbp->n_handles && hdbp->handles[newHandle].ref_count > 0)
    {
        pthread_mutex_unlock(&hdbp->mutex);
        clLogError("HDL", "MOVE", "New handle [%#llx] is in use", newHandle);
        return CL_HANDLE_RC(CL_ERR_ALREADY_EXIST);
    }

    instance = hdbp->handles[oldHandle].instance;
    if(!instance)
    {
        pthread_mutex_unlock(&hdbp->mutex);
        clLogError("HDL", "MOVE", "Old handle [%#llx] instance is NULL", oldHandle);
        return CL_HANDLE_RC(CL_ERR_INVALID_STATE);
    }

    if(newHandle >= hdbp->n_handles)
    {
        /*
         * Allocating space in Excess of to accomodate the value specified by the user.
         * NOTE: User should ensure that a sane value is supplied for handle.
         */
        ClHandleT excess_handles = newHandle - hdbp->n_handles + 1;

		ClHdlEntryT *new_handles = (ClHdlEntryT *)realloc (hdbp->handles,
                                                           sizeof (ClHdlEntryT) * (hdbp->n_handles+excess_handles));
		if (new_handles == NULL) 
        {
			ec = pthread_mutex_unlock (&hdbp->mutex);
            if (ec != 0)
            {
                return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be very bad */
            }
			return CL_HANDLE_RC(CL_ERR_NO_MEMORY);
		}

        /* 
         * Initialize the excess space. HANDLE_STATE_EMPTY will remain 0 
         * is assumed else need to set the excess entries explicitly in a 
         * loop. 
         */
        memset(&new_handles[hdbp->n_handles], 0, sizeof (ClHdlEntryT) * excess_handles);

        /* Update the values if success */
		hdbp->n_handles += excess_handles;
		hdbp->handles = new_handles;
	}
    /*
     * Reset the old handle
     */
    memset(&hdbp->handles[oldHandle], 0, sizeof(hdbp->handles[oldHandle]));
	hdbp->handles[newHandle].state = HANDLE_STATE_USED;
	hdbp->handles[newHandle].instance = instance;
	hdbp->handles[newHandle].ref_count = 1;

    clDbgResourceNotify(clDbgHandleResource, clDbgAllocate, hdbp, newHandle+1, 
                        ("Specific handle [%p:%#llX] allocated", (ClPtrT) hdbp, newHandle+1));

	ec = pthread_mutex_unlock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
	return rc;
}


ClRcT
clHandleDestroy (
    ClHandleDatabaseHandleT databaseHandle,
    ClHandleT handle)
{
    ClHdlDatabaseT *hdbp = (ClHdlDatabaseT*) databaseHandle;
    ClRcT          ec    = CL_OK;

    hdlDbValidityChk(hdbp);
    handle = CL_HDL_IDX(handle); /* once we've verified it, we only care about the index */
    
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    if (CL_HANDLE_INVALID_VALUE == handle--)
    {
        clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, 
                "Passed handle [%p:%#llX] is invalid", (ClPtrT) hdbp, handle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }
    /* Verify this particular handle has been already created */
    if( (NULL == hdbp->handles) || (0 == hdbp->n_handles_used) )
    {
        clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, 
                "Invalid attempt to delete the non exiting handle [%p:%#llX]", 
                (ClPtrT) hdbp, handle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }

    ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }
    if (handle >= (ClHandleT)hdbp->n_handles)
    {
        ec = pthread_mutex_unlock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
        }
        clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, 
                "Passed handle [%p:%#llX] has not been created", 
                (ClPtrT) hdbp, handle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }
    clDbgResourceNotify(clDbgHandleResource, clDbgRelease, hdbp, handle+1, ("Handle [%p:%#llX] (state: %d, ref: %d) released", (ClPtrT)hdbp, handle+1,hdbp->handles[handle].state,hdbp->handles[handle].ref_count));

    if (HANDLE_STATE_USED == hdbp->handles[handle].state)
    {
        hdbp->handles[handle].state = HANDLE_STATE_PENDINGREMOVAL;

        ec = pthread_mutex_unlock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
        }
        /*
         * Adding 1 to handle to ensure the non-zero handle interface.
         */
        ec = clHandleCheckin (databaseHandle, handle+1);
        return ec;
    }
    else if (HANDLE_STATE_EMPTY == hdbp->handles[handle].state)
    {
        ec = CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }
    else if (HANDLE_STATE_PENDINGREMOVAL == hdbp->handles[handle].state)
    {
        ec = pthread_mutex_unlock( &hdbp->mutex);
        if( ec != 0 )
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
        }
        clLogWarning(CL_HDL_AREA, CL_HDL_CTX_DESTROY,  
                     "Destroy has been called for this handle [%p:%#llX]" 
                     "returning CL_OK", (ClPtrT) hdbp,
                     (handle + 1));
        return CL_OK;
    }
    else
    {
        clDbgCodeError(CL_ERR_INVALID_HANDLE, 
                       ("Passed handle [%p:%#llX] doesn't have any proper state,"
                       "corrupted code", (ClPtrT) hdbp, (handle + 1)));
        /*
         * Invalid state - this musn't happen!
         */
    }
    if(pthread_mutex_unlock (&hdbp->mutex) != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }

#if 0
    clLogTrace(CL_HDL_AREA, CL_HDL_CTX_DESTROY, 
               "Handle [%p:%#llX] has been deleted successfully", 
               (ClPtrT) hdbp, (handle + 1));
#endif
    return ec;
}


ClRcT
clHandleCheckout(
                 ClHandleDatabaseHandleT databaseHandle,
                 ClHandleT               handle,
                 void                    **instance)
{ 
	ClRcT           rc    = CL_OK;
    ClHdlDatabaseT  *hdbp = (ClHdlDatabaseT*)databaseHandle;
    ClHdlStateT     state = 0;
    ClRcT           ec    = CL_OK;
    
    hdlDbValidityChk(hdbp);
    /* sometimes people want to create the same handle across multiple nodes hdlValidityChk(handle,hdbp); */
    handle = CL_HDL_IDX(handle); /* once we've verified it, we only care about the index */
    nullChkRet(instance);
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    if (CL_HANDLE_INVALID_VALUE == handle--)
    {
        clDbgCodeError(CL_HANDLE_RC(CL_ERR_INVALID_HANDLE), ("Passed Invalid Handle [0x0]"));
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }

    ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }

    if (handle >= (ClHandleT)hdbp->n_handles)
    {
        rc = CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
        pthread_mutex_unlock(&hdbp->mutex);
        clDbgCodeError(rc, ("Passed Invalid Handle [%p:%#llX]", (ClPtrT) hdbp, (handle+1)));
        return rc;
    }
    
    if ( ( state = hdbp->handles[handle].state ) != HANDLE_STATE_USED)
    {
        pthread_mutex_unlock(&hdbp->mutex);
        if (state == HANDLE_STATE_EMPTY)
        {
            /* In some of our ASP components the assumption made,
             * like checkout handle returns CL_ERR_INVALID_HANDLE 
             * to verify the handle does exist or not.
             * so removing the debug pause
             */
#if 0
            clDbgCodeError(rc, ("Handle [%p:%#llX] is not allocated", (ClPtrT)
                                hdbp, (handle+1)));
#endif
        }
        else if (state == HANDLE_STATE_PENDINGREMOVAL)
        {
            clDbgCodeError(rc, ("Handle [%p:%#llX] is being removed", (ClPtrT)
                                hdbp, (handle+1)));
        }
        else
        {
            clDbgCodeError(rc, ("Handle [%p:%#llX] invalid state %d", (ClPtrT)
                                hdbp, (handle+1), state));
        }

        rc = CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
        clDbgCodeError(rc, ("Handle [%p:%#llX] is invalid", (ClPtrT) hdbp, (handle+1)));
        return rc;
    }

    *instance = hdbp->handles[handle].instance;

    hdbp->handles[handle].ref_count += 1;

    ec = pthread_mutex_unlock (&hdbp->mutex);
    if (ec != 0)
    {
        clDbgCodeError(CL_HANDLE_RC(CL_ERR_MUTEX_ERROR), ("Mutex unlock failed errno %d", errno));
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
#if 0
    clLogTrace(CL_HDL_AREA, CL_HDL_CTX_CHECKOUT, 
               "Checked out handle [%p:%#llX]", (ClPtrT) hdbp, (handle + 1));
#endif
    return rc;
}


ClRcT
clHandleCheckin(
                ClHandleDatabaseHandleT databaseHandle,
                ClHandleT handle)
{
    ClRcT          rc        = CL_OK;
    void           *instance = NULL;
    ClHdlDatabaseT *hdbp     = (ClHdlDatabaseT*) databaseHandle;
    ClRcT          ec        = CL_OK;
    ClInt32T       refcount  = 0;

    hdlDbValidityChk(hdbp);
    /* sometimes people want to create the same handle across multiple nodes hdlValidityChk(handle,hdbp); */
    handle = CL_HDL_IDX(handle); /* once we've verified it, we only care about the index */
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    if (CL_HANDLE_INVALID_VALUE == handle--)
    {
        clLogError(CL_HDL_AREA, CL_HDL_CTX_CHECKIN, 
                "Passed handle [%p:%#llX] is invalid", (ClPtrT) hdbp, handle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }

    ec = pthread_mutex_lock(&hdbp->mutex);
    if (ec != 0)
    {
        int err = errno;
        clDbgCodeError(clErr, ("Handle database mutex lock failed error: %s (%d)", strerror(err), err) );
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }

    if (handle >= (ClHandleT)hdbp->n_handles)
    {
        pthread_mutex_unlock( &hdbp->mutex);
        clLogError(CL_HDL_AREA, CL_HDL_CTX_CHECKIN, 
                "Passed handle [%p:%#llX] is invalid handle", (ClPtrT) hdbp, handle);
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }
    refcount = hdbp->handles[handle].ref_count;
    if( (--refcount <= 0) && (hdbp->handles[handle].state != HANDLE_STATE_PENDINGREMOVAL) )
    {
        pthread_mutex_unlock( &hdbp->mutex);
        clLogError(CL_HDL_AREA, CL_HDL_CTX_CHECKIN,  
                "There is no balance between checkout, checkin for handle [%p:%#llX]", 
                (ClPtrT) hdbp, (handle + 1));
        return CL_HANDLE_RC(CL_ERR_INVALID_STATE);
    }

    CL_ASSERT(hdbp->handles[handle].ref_count > 0); // unsigned compare (CID 196 on #1780)
    hdbp->handles[handle].ref_count -= 1;

    if (hdbp->handles[handle].ref_count == 0)
    {
        instance = (hdbp->handles[handle].instance);
        if (hdbp->handle_instance_destructor != NULL)
        {
            hdbp->handle_instance_destructor(instance);
        }
        if (hdbp->handles[handle].flags & HANDLE_ALLOC_FLAG)  /* Clean up the handle if we allocated it */
          clHeapFree(instance);
        
        memset(&hdbp->handles[handle], 0,    /* This also makes entry EMPTY */
                sizeof(ClHdlEntryT));
        CL_ASSERT(hdbp->n_handles_used > 0); //  unsigned compare (CID 196 on #1780)
        hdbp->n_handles_used--;
    }

    ec = pthread_mutex_unlock(&hdbp->mutex);
    if (ec != 0)
    {
        int err = errno;
        clDbgCodeError(clErr, ("Handle database mutex unlock failed error: %s (%d)", strerror(err), err) );
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
    /* This check to avoid recursive call from LogClient */
    if( refcount > 0 )
    {
#if 0
        clLogTrace(CL_HDL_AREA, CL_HDL_CTX_CHECKIN, 
                "Checkin for handle [%p:%#llX]", 
                (ClPtrT) hdbp, (handle + 1));
#endif
    }

    return rc;
}

/*
 * The following API was added to walk through the existing handle database
 * and execute the user specified callback for each handle in use.
 */
ClRcT
clHandleWalk (ClHandleDatabaseHandleT databaseHandle,
	ClHandleDbWalkCallbackT fpUserWalkCallback,
    void *pCookie)
{
	ClHandleT      handle  = CL_HANDLE_INVALID_VALUE;
    ClHdlDatabaseT *hdbp   = (ClHdlDatabaseT*)databaseHandle;
    ClRcT          ec      = CL_OK;
    ClRcT          rc      = CL_OK;

    hdlDbValidityChk(hdbp);
    if (NULL == fpUserWalkCallback)
    {
        clLogError("HDL", CL_LOG_CONTEXT_UNSPECIFIED, "fpUserWalkCallback is NULL");
        return CL_HANDLE_RC(CL_ERR_NULL_POINTER);
    }
    ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }

    for (handle = 0; handle < hdbp->n_handles; handle++) {
        
        ClHandleT tempHandle = 0;
        /*
         * Ignore the handle if not used.
         */
        if (hdbp->handles[handle].state != HANDLE_STATE_USED) {
            continue;
        }

        /*
         * Unlock the database since the user needs to checkout the
         * handle in the callback.
         */
        ec = pthread_mutex_unlock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* Disastrous */
        }

        /*
         * Adding 1 to handle to ensure the non-zero handle interface.
         */
        tempHandle = CL_HDL_MAKE(ASP_NODEADDR, hdbp->id, handle + 1);
        rc = fpUserWalkCallback(databaseHandle, tempHandle, pCookie);
        if(rc != CL_OK) {
            goto exit;  /* Already unlocked */
        }

        /*
         * Lock the database again to test check remaining handles.
         */
        ec = pthread_mutex_lock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* Disastrous */
        }
    }

    ec = pthread_mutex_unlock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }
exit:
    return rc;
}

/*
 * The API returns CL_ERR_INVALID_HANDLE if the handle is not valid.
 */ 
ClRcT
clHandleValidate (
    ClHandleDatabaseHandleT databaseHandle,
    ClHandleT handle)
{
    ClHdlDatabaseT *hdbp = (ClHdlDatabaseT*)databaseHandle;
    int            ec    = 0;
    
    hdlDbValidityChk(hdbp);
    /* sometimes people want to create the same handle across multiple nodes hdlValidityChk(handle,hdbp); */
    handle = CL_HDL_IDX(handle); /* once we've verified it, we only care about the index */
    /*
     * Decrementing handle to ensure the non-zero handle interface.
     */
    if (CL_HANDLE_INVALID_VALUE == handle--)
    {
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE); /* 0 no longer allowed */
    }

	ec = pthread_mutex_lock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR);
    }
    
    if (handle >= (ClHandleT)hdbp->n_handles)
    {
        ec = pthread_mutex_unlock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
        }
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }

    if (HANDLE_STATE_USED != hdbp->handles[handle].state )
    {
        ec = pthread_mutex_unlock (&hdbp->mutex);
        if (ec != 0)
        {
            return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
        }
        return CL_HANDLE_RC(CL_ERR_INVALID_HANDLE);
    }

	ec = pthread_mutex_unlock (&hdbp->mutex);
    if (ec != 0)
    {
        return CL_HANDLE_RC(CL_ERR_MUTEX_ERROR); /* This can be devastating */
    }

	return CL_OK;
}
