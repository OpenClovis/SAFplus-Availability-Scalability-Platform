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
 * ModuleName  : gms                                                           
 * File        : clGmsRmdServer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains the function calls that are passed to the 
 * clClientInstall function. These function calls are mapped onto function 
 * ids in common/clGmsCommon.h
 *
 * The RMD layer call's the function corresponding to the function id passed
 * from the remote end.
 *
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clRmdApi.h>

#include <clGms.h>
#include <clGmsMsg.h>
#include <clGmsErrors.h>
#include <clGmsApiHandler.h>
#include <clLogApi.h>
#include <clXdrApi.h>
#include "clGmsRmdServer.h"

/******************************************************************************
 * API Function Implementations
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 * (Client) Initialize API
 *---------------------------------------------------------------------------*/
static ClRcT 
unmarshal_gms_clientlib_initialize_request(
        CL_IN const ClBufferHandleT buf, 
        CL_OUT ClGmsClientInitRequestT* const req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }

    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(len == sizeof(*req)); 

    return rc;
}


static ClRcT 
marshal_gms_clientlib_initialize_response(
    CL_IN    ClGmsClientInitResponseT* const res,
    CL_OUT   const ClBufferHandleT buf )
{
    ClRcT rc = CL_OK;

    CL_ASSERT(res!=NULL);

    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    return rc;
}


ClRcT
VDECL (cl_gms_initialize_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClGmsClientInitRequestT req = {{0}};
    ClGmsClientInitResponseT res = {0};
    ClRcT rc = CL_OK;

    /* unmarshal the request from the client */ 
    rc = unmarshal_gms_clientlib_initialize_request( in_buffer, &req );
    if(rc != CL_OK)
    {
        return rc;
    }

    rc = clGmsClientLibInitHandler( &req, &res );
    if(rc!= CL_OK)
    {
        return rc;
    }

    /* marshal the response */
    rc = marshal_gms_clientlib_initialize_response(&res, out_buffer);

    if(rc != CL_OK)
    {
        return rc;
    }

    return 0x0;
}


/*-----------------------------------------------------------------------------
 * (Client) Finalize API
 *---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_finalize_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_GMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*-----------------------------------------------------------------------------
 * Cluster Track API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterTrackRequest(
    CL_IN    const ClBufferHandleT         buf,
    CL_INOUT ClGmsClusterTrackRequestT*  const    req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterTrackResponse(
    CL_IN    ClGmsClusterTrackResponseT*  const res,
    CL_INOUT const ClBufferHandleT       buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    if ((rc != CL_OK) || (res == NULL))
    {
        goto error_exit;
    }
    
    if (res->buffer.notification != NULL)
    {
        CL_ASSERT(res->buffer.numberOfItems > 0);
        
        rc = clBufferNBytesWrite(buf, (void*)res->buffer.notification,
                 res->buffer.numberOfItems * sizeof(ClGmsClusterNotificationT));
    }

error_exit:
    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_track_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterTrackRequestT    req = {0};
    ClGmsClusterTrackResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterTrackRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterTrackHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterTrackResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    if (res.buffer.notification != NULL)
    {
        clHeapFree((void*)res.buffer.notification);
    }
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Track Stop API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterTrackStopRequest(
    CL_IN    const ClBufferHandleT             buf,
    CL_INOUT ClGmsClusterTrackStopRequestT*  const    req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterTrackStopResponse(
    CL_IN    ClGmsClusterTrackStopResponseT* const res,
    CL_INOUT const ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_track_stop_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterTrackStopRequestT    req = {0};
    ClGmsClusterTrackStopResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterTrackStopRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterTrackStopHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterTrackStopResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Member Get API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterMemberGetRequest(
    CL_IN    const ClBufferHandleT                 buf,
    CL_INOUT ClGmsClusterMemberGetRequestT*  const        req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterMemberGetResponse(
    CL_IN    ClGmsClusterMemberGetResponseT* const  res,
    CL_INOUT const ClBufferHandleT           buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_member_get_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterMemberGetRequestT    req = {0};
    ClGmsClusterMemberGetResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterMemberGetRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterMemberGetHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterMemberGetResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Member Get Async API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterMemberGetAsyncRequest(
    CL_IN    const ClBufferHandleT                 buf,
    CL_INOUT ClGmsClusterMemberGetAsyncRequestT* const    req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterMemberGetAsyncResponse(
    CL_IN    ClGmsClusterMemberGetAsyncResponseT* const  res,
    CL_INOUT const  ClBufferHandleT               buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_member_get_async_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterMemberGetAsyncRequestT    req = {0};
    ClGmsClusterMemberGetAsyncResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterMemberGetAsyncRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterMemberGetAsyncHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterMemberGetAsyncResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group List Get API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupInfoListGetRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupsInfoListGetRequestT         *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:
    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupsInfoListGetResponse(
    CL_IN    ClGmsGroupsInfoListGetResponseT     *res,
    CL_INOUT ClBufferHandleT              buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
    if (res->groupsList.groupInfoList != NULL)
    {
        CL_ASSERT(res->groupsList.noOfGroups > 0);
        
        rc = clBufferNBytesWrite(buf, (void*)res->groupsList.groupInfoList,
                 res->groupsList.noOfGroups * sizeof(ClGmsGroupInfoT));
    }

error_exit:
    return rc;
}
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_list_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    rc = CL_GMS_RC( CL_ERR_NOT_IMPLEMENTED );
    ClGmsGroupsInfoListGetRequestT    req = {0};
    ClGmsGroupsInfoListGetResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupInfoListGetRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupInfoListGetHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupsInfoListGetResponse(&res, out_buffer);
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Info Get API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupInfoGetRequest(
    CL_IN    ClBufferHandleT             buf,
    CL_INOUT ClGmsGroupInfoGetRequestT         *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:
    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupInfoGetResponse(
    CL_IN    ClGmsGroupInfoGetResponseT     *res,
    CL_INOUT ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
error_exit:
    return rc;
}
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_info_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    rc = CL_GMS_RC( CL_ERR_NOT_IMPLEMENTED );
    ClGmsGroupInfoGetRequestT    req = {0};
    ClGmsGroupInfoGetResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupInfoGetRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupInfoGetHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupInfoGetResponse(&res, out_buffer);
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Comp Up Notify API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsCompUpNotifyRequest(
    CL_IN    ClBufferHandleT             buf,
    CL_INOUT ClGmsCompUpNotifyRequestT         *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:
    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsCompUpNotifyResponse(
    CL_IN    ClGmsCompUpNotifyResponseT     *res,
    CL_INOUT ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
error_exit:
    return rc;
}
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_comp_up_notify_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    rc = CL_GMS_RC( CL_ERR_NOT_IMPLEMENTED );
    ClGmsCompUpNotifyRequestT    req = {0};
    ClGmsCompUpNotifyResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsCompUpNotifyRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsCompUpNotifyHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
error_return_res:
    /* Marshal result */
    rc = marshalClGmsCompUpNotifyResponse(&res, out_buffer);
    
    return rc;
}
    
/*-----------------------------------------------------------------------------
 * Group Mcast Send API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupMcastRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupMcastRequestT         *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(ClGmsGroupMcastRequestT);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
    req->data = clHeapAllocate(req->dataSize);
    if (req->data == NULL)
        return CL_ERR_NO_MEMORY;

    len = req->dataSize;
    /* Now read the data pointer */
    rc = clBufferNBytesRead(buf, (void*)req->data, &len);

error_exit:
    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupMcastResponse(
    CL_IN    ClGmsGroupMcastResponseT     *res,
    CL_INOUT ClBufferHandleT              buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    
    return rc;
}
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_mcast_send_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    rc = CL_GMS_RC( CL_ERR_NOT_IMPLEMENTED );
    ClGmsGroupMcastRequestT    req = {0};
    ClGmsGroupMcastResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupMcastRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupMcastHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupMcastResponse(&res, out_buffer);
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Track API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupTrackRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupTrackRequestT               *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupTrackResponse(
    CL_IN    ClGmsGroupTrackResponseT   *res,
    CL_INOUT ClBufferHandleT      buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
    if (res->buffer.notification != NULL)
    {
        CL_ASSERT(res->buffer.numberOfItems > 0);
        
        rc = clBufferNBytesWrite(buf, (void*)res->buffer.notification,
                 res->buffer.numberOfItems * sizeof(ClGmsGroupNotificationT));
    }

error_exit:
    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_track_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupTrackRequestT    req = {0};
    ClGmsGroupTrackResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupTrackRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupTrackHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupTrackResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    if (res.buffer.notification != NULL)
    {
        clHeapFree((void*)res.buffer.notification);
    }
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Track Stop API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupTrackStopRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupTrackStopRequestT           *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupTrackStopResponse(
    CL_IN    ClGmsGroupTrackStopResponseT   *res,
    CL_INOUT ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_track_stop_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupTrackStopRequestT    req = {0};
    ClGmsGroupTrackStopResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupTrackStopRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupTrackStopHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupTrackStopResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Member Get API
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_member_get_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Member Get Async API
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_member_get_async_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Cluster Join API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterJoinRequest(
    CL_IN    const ClBufferHandleT                 buf,
    CL_INOUT ClGmsClusterJoinRequestT* const              req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterJoinResponse(
    CL_IN    ClGmsClusterJoinResponseT* const res,
    CL_INOUT const ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_join_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterJoinRequestT    req = {0};
    ClGmsClusterJoinResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterJoinRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterJoinHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterJoinResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Leave API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterLeaveRequest(
    CL_IN    const ClBufferHandleT            buf,
    CL_INOUT ClGmsClusterLeaveRequestT*  const       req)
{
    ClRcT rc = CL_OK;

    rc |= clXdrUnmarshallClVersionT(buf, &req->clientVersion);
    rc |= clXdrUnmarshallClUint32T(buf, &req->groupId);
    rc |= clXdrUnmarshallClUint32T(buf, &req->nodeId);
    rc |= clXdrUnmarshallClUint16T(buf, &req->sync);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterLeaveResponse(
    CL_IN    ClGmsClusterLeaveResponseT* const res,
    CL_INOUT const ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc |= clXdrMarshallClVersionT(&res->serverVersion, buf, 0);
    rc |= clXdrMarshallClUint32T(&res->rc, buf, 0);

    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_leave_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterLeaveRequestT    req = {0};
    ClGmsClusterLeaveResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterLeaveRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterLeaveHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterLeaveResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Leader Elect API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterLeaderElectRequest(
    CL_IN    const ClBufferHandleT              buf,
    CL_INOUT ClGmsClusterLeaderElectRequestT* const    req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterLeaderElectResponse(
    CL_IN    ClGmsClusterLeaderElectResponseT* const res,
    CL_INOUT const ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_leader_elect_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterLeaderElectRequestT    req = {0};
    ClGmsClusterLeaderElectResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterLeaderElectRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterLeaderElectHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterLeaderElectResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Member Eject API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsClusterMemberEjectRequest(
    CL_IN    const ClBufferHandleT             buf,
    CL_INOUT ClGmsClusterMemberEjectRequestT*  const  req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if ((rc != CL_OK) || (len < sizeof(*req)))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsClusterMemberEjectResponse(
    CL_IN    ClGmsClusterMemberEjectResponseT* const res,
    CL_INOUT const ClBufferHandleT            buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_cluster_member_eject_rmd) (
    CL_IN   ClEoDataT              c_data,     /* Unused */
    CL_IN   const ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  const ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsClusterMemberEjectRequestT    req = {0};
    ClGmsClusterMemberEjectResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsClusterMemberEjectRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsClusterMemberEjectHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsClusterMemberEjectResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Create API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupCreateRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupCreateRequestT              *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupCreateResponse(
    CL_IN    ClGmsGroupCreateResponseT *res,
    CL_INOUT ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_create_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupCreateRequestT    req = {0};
    ClGmsGroupCreateResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupCreateRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupCreateHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupCreateResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Destroy API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupDestroyRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupDestroyRequestT              *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupDestroyResponse(
    CL_IN    ClGmsGroupDestroyResponseT *res,
    CL_INOUT ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_destroy_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupDestroyRequestT    req = {0};
    ClGmsGroupDestroyResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupDestroyRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupDestroyHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupDestroyResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Join API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupJoinRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupJoinRequestT              *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupJoinResponse(
    CL_IN    ClGmsGroupJoinResponseT *res,
    CL_INOUT ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_join_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupJoinRequestT    req = {0};
    ClGmsGroupJoinResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupJoinRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupJoinHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupJoinResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Leave API
 *---------------------------------------------------------------------------*/
static ClRcT
unmarshalClGmsGroupLeaveRequest(
    CL_IN    ClBufferHandleT                 buf,
    CL_INOUT ClGmsGroupLeaveRequestT              *req)
{
    ClRcT rc = CL_OK;
    ClUint32T len = 0x0;

    rc = clBufferLengthGet(buf, &len);
    if (rc != CL_OK || len < sizeof(*req))
    {
        return CL_GMS_RC(CL_GMS_ERR_UNMARSHALING_FAILED);
    }
    
    len = sizeof(*req);
    rc = clBufferNBytesRead(buf, (void*)req, &len);
    if (rc != CL_OK)
    {
        goto error_exit;
    }
    CL_ASSERT(len == sizeof(*req)); /* to never happen */
    
error_exit:

    return rc;
}

/*---------------------------------------------------------------------------*/
static ClRcT
marshalClGmsGroupLeaveResponse(
    CL_IN    ClGmsGroupLeaveResponseT *res,
    CL_INOUT ClBufferHandleT     buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(res!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)res, sizeof(*res));

    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_leave_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    ClRcT rc = CL_OK;
    ClGmsGroupLeaveRequestT    req = {0};
    ClGmsGroupLeaveResponseT   res = {0};
    
    /* Unmarshal request: may allocate auxiliary data if req has such thing */
    rc = unmarshalClGmsGroupLeaveRequest(in_buffer, &req);
    if (rc != CL_OK) /* If error, no additional memory allocation happened */
    {
        res.rc = rc; /* failure is not an RMD error but a GMS error */
        goto error_return_res;
    }
    
    /* Call the higher level call: it should fill out res */
    rc = clGmsGroupLeaveHandler(&req, &res);
    if (rc != CL_OK) /* No additional data was allocated in res by API call */
    {
        res.rc = rc;
        goto error_return_res;
    }
    
    /* Free aux req data if any */
    /* none */
    
error_return_res:
    /* Marshal result */
    rc = marshalClGmsGroupLeaveResponse(&res, out_buffer);
    
    /* Free aux res data if any */
    /* none */
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Leader Elect API
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_leader_elect_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Member Eject API
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_group_member_eject_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/******************************************************************************
 * RMD Async Callback Function for Cluster Track Callback
 *****************************************************************************/
static ClRcT
marshalClGmsClusterTrackCallbackData(
    CL_IN    ClGmsClusterTrackCallbackDataT* const data,
    CL_INOUT const ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(data!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)data, sizeof(*data));
    if ((rc != CL_OK) || (data == NULL))
    {
        goto error_return;
    }
    
    if (data->buffer.notification != NULL)
    {
        CL_ASSERT(data->buffer.numberOfItems > 0);
        rc = clBufferNBytesWrite(buf, (void*)data->buffer.notification,
               data->buffer.numberOfItems * sizeof(ClGmsClusterNotificationT));
    }
    
error_return:
    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_cluster_track_callback(
    CL_IN   const ClIocAddressT                   addr,
    CL_IN   ClGmsClusterTrackCallbackDataT* const data)
{
    ClRcT                  rc = CL_OK;
    ClBufferHandleT buffer = NULL;
    ClRmdOptionsT          rmd_options = CL_RMD_DEFAULT_OPTIONS;
    
    CL_ASSERT(data != NULL);
    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = marshalClGmsClusterTrackCallbackData(data, buffer);
    if (rc != CL_OK)
    {
        goto error_free_buffer;
    }

    rmd_options.priority = CL_IOC_HIGH_PRIORITY;
    rmd_options.timeout = CL_GMS_RMD_DEFAULT_TIMEOUT;
    rmd_options.retries = CL_GMS_RMD_DEFAULT_RETRIES;

    rc = clRmdWithMsg(addr,
                      GMS_FUNC_ID(CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK),
                      buffer,
                      (ClBufferHandleT)NULL,
                      CL_RMD_CALL_ASYNC,
                      &rmd_options,
                      NULL);

error_free_buffer:
    clBufferDelete(&buffer);
    
    return rc;
}


/******************************************************************************
 * RMD Async Callback Function for Cluster Member Get Callback
 *****************************************************************************/
static ClRcT
marshalClGmsClusterMemberGetCallbackData(
    CL_IN    ClGmsClusterMemberGetCallbackDataT* const data,
    CL_INOUT const ClBufferHandleT              buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(data!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)data, sizeof(*data));

    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_cluster_member_get_callback(
    CL_IN   const ClIocAddressT                       addr,
    CL_IN   ClGmsClusterMemberGetCallbackDataT* const data)
{
    ClRcT                  rc = CL_OK;
    ClBufferHandleT buffer = NULL;
    ClRmdOptionsT          rmd_options = CL_RMD_DEFAULT_OPTIONS;
    
    CL_ASSERT(data != NULL);
    
    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = marshalClGmsClusterMemberGetCallbackData(data, buffer);
    if (rc != CL_OK)
    {
        goto error_free_buffer;
    }

    rmd_options.priority = 0;
    rmd_options.timeout = CL_GMS_RMD_DEFAULT_TIMEOUT;
    rmd_options.retries = CL_GMS_RMD_DEFAULT_RETRIES;
    
    rc = clRmdWithMsg(addr,
                      GMS_FUNC_ID(CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK),
                      buffer,
                      (ClBufferHandleT)NULL,
                      0,
                      &rmd_options,
                      NULL);

   
error_free_buffer:
    clBufferDelete(&buffer);
    
    return rc;
}


/******************************************************************************
 * RMD Async Callback Function Handler for Cluster Member Eject Callback
 *****************************************************************************/
static ClRcT
marshalClGmsClusterMemberEjectCallbackData(
    CL_IN    ClGmsClusterMemberEjectCallbackDataT* const data,
    CL_INOUT const  ClBufferHandleT               buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(data!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)data, sizeof(*data));

    return rc;
}


/*---------------------------------------------------------------------------*/

ClRcT
cl_gms_cluster_member_eject_callback(
    CL_IN   const    ClIocAddressT                      addr,
    CL_IN   ClGmsClusterMemberEjectCallbackDataT* const data)
{
    ClRcT                  rc = CL_OK;
    ClBufferHandleT buffer = NULL;
    ClRmdOptionsT          rmd_options = CL_RMD_DEFAULT_OPTIONS;
    
    CL_ASSERT(data != NULL);
    
    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = marshalClGmsClusterMemberEjectCallbackData(data, buffer);
    if (rc != CL_OK)
    {
        goto error_free_buffer;
    }

    rmd_options.priority = 0;
    rmd_options.timeout = CL_GMS_RMD_DEFAULT_TIMEOUT;
    rmd_options.retries = CL_GMS_RMD_DEFAULT_RETRIES;
    
    rc = clRmdWithMsg(addr,
                      GMS_FUNC_ID(CL_GMS_CLIENT_CLUSTER_MEMBER_EJECT_CALLBACK),
                      buffer,
                      (ClBufferHandleT)NULL,
                      0,
                      &rmd_options,
                      NULL);

error_free_buffer:
    clBufferDelete(&buffer);
    
    return rc;
}

/******************************************************************************
 * RMD Async Callback Function for Group Track Callback
 *****************************************************************************/
static ClRcT
marshalClGmsGroupTrackCallbackData(
    CL_IN    ClGmsGroupTrackCallbackDataT   *data,
    CL_INOUT ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(data!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)data, sizeof(*data));
    if (rc != CL_OK)
    {
        goto error_return;
    }
    
    if (data->buffer.notification != NULL)
    {
        CL_ASSERT(data->buffer.numberOfItems > 0);
        rc = clBufferNBytesWrite(buf, (void*)data->buffer.notification,
               data->buffer.numberOfItems * sizeof(ClGmsGroupNotificationT));
    }
    
error_return:
    return rc;
}

/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_group_track_callback(
    CL_IN   ClIocAddressT addr,
    CL_IN   ClGmsGroupTrackCallbackDataT *data)
{
    ClRcT                  rc = CL_OK;
    ClBufferHandleT buffer = NULL;
    ClRmdOptionsT          rmd_options = CL_RMD_DEFAULT_OPTIONS;
    
    CL_ASSERT(data != NULL);
    
    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = marshalClGmsGroupTrackCallbackData(data, buffer);
    if (rc != CL_OK)
    {
        goto error_free_buffer;
    }

    rmd_options.priority = 0;
    rmd_options.timeout = CL_GMS_RMD_DEFAULT_TIMEOUT;
    rmd_options.retries = CL_GMS_RMD_DEFAULT_RETRIES;
    
    rc = clRmdWithMsg(addr,
                      GMS_FUNC_ID(CL_GMS_CLIENT_GROUP_TRACK_CALLBACK),
                      buffer,
                      (ClBufferHandleT)NULL,
                      0,
                      &rmd_options,
                      NULL);

error_free_buffer:
    clBufferDelete(&buffer);
    return rc;
}

/******************************************************************************
 * RMD Async Callback Function for Group Member Get Callback
 *****************************************************************************/
/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_group_member_get_callback(
    CL_IN   ClIocAddressT addr,
    CL_IN   ClGmsGroupMemberGetCallbackDataT *data)
{
    return CL_ERR_NOT_IMPLEMENTED;
}


/******************************************************************************
 * RMD Async Callback Function Handler for Group Member Eject Callback
 *****************************************************************************/
/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_group_member_eject_callback(
    CL_IN   ClIocAddressT addr,
    CL_IN   ClGmsGroupMemberEjectCallbackDataT *data)
{
    return CL_ERR_NOT_IMPLEMENTED;
}

/******************************************************************************
 * RMD Async Callback Function for Group Mcast Send
 *****************************************************************************/
static ClRcT
marshalClGmsGroupMcastCallbackData(
    CL_IN    ClGmsGroupMcastCallbackDataT* const data,
    CL_INOUT const ClBufferHandleT          buf)
{
    ClRcT rc = CL_OK;
    
    CL_ASSERT(data!=NULL);
    
    rc = clBufferNBytesWrite(buf, (void*)data, sizeof(ClGmsGroupMcastCallbackDataT));
    if ((rc != CL_OK) || (data == NULL))
    {
        goto error_return;
    }

    rc = clBufferNBytesWrite(buf, (void*)data->data, data->dataSize);
    if ((rc != CL_OK) || (data == NULL))
    {
        goto error_return;
    }
    
error_return:
    return rc;
}


/*---------------------------------------------------------------------------*/
ClRcT
cl_gms_group_mcast_callback(
    CL_IN   const ClIocAddressT                 addr,
    CL_IN   ClGmsGroupMcastCallbackDataT* const data)
{
    ClRcT                  rc = CL_OK;
    ClBufferHandleT buffer = NULL;
    ClRmdOptionsT          rmd_options = CL_RMD_DEFAULT_OPTIONS;
    
    CL_ASSERT(data != NULL);
    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    rc = marshalClGmsGroupMcastCallbackData(data, buffer);
    if (rc != CL_OK)
    {
        goto error_free_buffer;
    }

    rmd_options.priority = CL_IOC_HIGH_PRIORITY;
    rmd_options.timeout = CL_GMS_RMD_DEFAULT_TIMEOUT;
    rmd_options.retries = CL_GMS_RMD_DEFAULT_RETRIES;

    rc = clRmdWithMsg(addr,
                      GMS_FUNC_ID(CL_GMS_CLIENT_GROUP_MCAST_CALLBACK),
                      buffer,
                      (ClBufferHandleT)NULL,
                      CL_RMD_CALL_ASYNC,
                      &rmd_options,
                      NULL);

error_free_buffer:
    clBufferDelete(&buffer);
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Inter-server communication API
 *---------------------------------------------------------------------------*/
ClRcT
VDECL (cl_gms_server_receieve_rmd) (
    CL_IN   ClEoDataT        c_data,     /* Unused */
    CL_IN   ClBufferHandleT  in_buffer,  /* Input data from server */
    CL_OUT  ClBufferHandleT  out_buffer) /* Never used in callbacks */
{
    return CL_GMS_RC(CL_ERR_NOT_IMPLEMENTED);
}
