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
#include "clJobQueue.h"
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>

#define CL_JOBQUEUE_RC(rc) CL_RC(CL_CID_JOBQUEUE, rc)

#define JQ_PFX(hdl) do { clOsalMutexLock(&hdl->mutex); } while(0)

#define JQ_SFX(hdl) do { clOsalMutexUnlock(&hdl->mutex); } while(0)

typedef struct
{
    ClCallbackT job;
    ClPtrT data;
} ClJobT;

typedef struct ClJobQueueWalkArg
{
    ClJobQueueWalkCallbackT cb;
    ClPtrT cbArg;
}ClJobQueueWalkArgT;

enum
{
    CreatedJobQueue = 1,
    CreatedQueue    = 2,
    CreatedPool     = 4,
    Running         = 8,
};


/* A helper function so that we can someday queue unused ClJobTs instead of constantly alloc/dealloc */
static ClJobT* getJob(ClJobQueueT* hdl, ClCallbackT job,ClPtrT data)
{
    ClJobT* ret = (ClJobT*) clHeapCalloc(1, sizeof(ClJobT));
    if (ret)
    {
        ret->job  = job;
        ret->data = data;
    }
    return ret;
}

static void releaseJob(ClJobQueueT* hdl, ClJobT* job)
{
    clHeapFree(job);
}

/* We never use the delete/destroy functionality.  We can't because we need a cookie (what Job Queue) to be passed as well */
static void deleteCallback(ClQueueDataT userData)
{
    return;
}


ClRcT clJobQueuePreIdle(ClPtrT cookie)
{
    ClJobQueueT* hdl =  (ClJobQueueT*) cookie;
    if(! (hdl->flags & Running) ) return CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE);
    JQ_PFX(hdl);
    ClJobT* jb = NULL;
    ClQueueDataT temp = NULL;
    ClRcT rc = clQueueNodeDelete(hdl->queue, &temp);
    if (rc == CL_OK)
    {
        jb = (ClJobT*) temp;
        CL_ASSERT(jb);
        clTaskPoolRun(hdl->pool,jb->job, jb->data);
        releaseJob(hdl,jb);
    }

    JQ_SFX(hdl);
    return CL_OK;  
}


ClRcT clJobQueueInit(ClJobQueueT* hdl, ClUint32T maxJobs, ClUint32T maxTasks)
{
    ClRcT rc;
    hdl->flags = 0;
    hdl->queue = 0;
    hdl->pool  = 0;

    rc = clOsalMutexInit(&hdl->mutex);
    if (rc != CL_OK) goto error;

    rc = clQueueCreate(maxJobs, deleteCallback, deleteCallback, &hdl->queue);
    if (rc != CL_OK) goto error;

    rc = clTaskPoolCreate(&hdl->pool, maxTasks, clJobQueuePreIdle, hdl);
    if (rc != CL_OK) goto error;

    hdl->flags |= CreatedQueue | CreatedPool | Running;
    return CL_OK;  

    error:
    clOsalMutexDestroy(&hdl->mutex);
    if (hdl->pool) clTaskPoolDelete(hdl->pool);
    if (hdl->queue) clQueueDelete(&hdl->queue);

    hdl->queue = 0;
    hdl->pool  = 0;
    return rc;
}

ClRcT clJobQueueMonitorStart(ClJobQueueT *hdl, ClTimerTimeOutT monitorThreshold, ClTaskPoolMonitorCallbackT monitorCallback)
{
    ClRcT rc = CL_OK;

    if(!hdl) return CL_JOBQUEUE_RC(CL_ERR_INVALID_PARAMETER);

    JQ_PFX(hdl);
    rc = clTaskPoolMonitorStart(hdl->pool, monitorThreshold, monitorCallback);
    JQ_SFX(hdl);

    return rc;
}

ClRcT clJobQueueMonitorStop(ClJobQueueT *hdl)
{
    ClRcT rc = CL_OK;
    if(!hdl) return CL_JOBQUEUE_RC(CL_ERR_INVALID_PARAMETER);
    
    JQ_PFX(hdl);
    rc = clTaskPoolMonitorStop(hdl->pool);
    JQ_SFX(hdl);

    return rc;
}

ClRcT clJobQueueMonitorDelete(ClJobQueueT *hdl)
{
    ClRcT rc = CL_OK;
    if(!hdl) return CL_JOBQUEUE_RC(CL_ERR_INVALID_PARAMETER);

    JQ_PFX(hdl);
    rc = clTaskPoolMonitorDelete(hdl->pool);
    JQ_SFX(hdl);

    return rc;
}

ClRcT clJobQueueCreate(ClJobQueueT** hdl, ClUint32T maxJobs, ClUint32T maxTasks)
{
    ClRcT rc;
    *hdl = (ClJobQueueT*) clHeapCalloc(1, sizeof(ClJobQueueT));
    if (*hdl == NULL) return CL_JOBQUEUE_RC(CL_ERR_NO_MEMORY);

    rc = clJobQueueInit(*hdl, maxJobs, maxTasks);
    if (rc != CL_OK)
    {
        clHeapFree(*hdl);
        *hdl = NULL;
    }
    else (*hdl)->flags |= CreatedJobQueue;

    return rc;  
}

/*
  ClRcT clJobQueueCreate2(ClJobQueueT** hdl, ClTaskPool* pool);
*/
  
ClRcT clJobQueueDelete(ClJobQueueT* hdl)
{
    if(hdl->flags & CreatedQueue) 
    {
        /*
         * Prevent parallel job queue pushes during a queue delete.
         */
        JQ_PFX(hdl);
        hdl->flags &= ~Running;
        JQ_SFX(hdl);
        clJobQueueStop(hdl);
        if (hdl->flags & CreatedPool) clTaskPoolDelete(hdl->pool);
        clQueueDelete(&hdl->queue);
        clOsalMutexDestroy(&hdl->mutex);
    }
    if (hdl->flags & CreatedJobQueue) clHeapFree(hdl);
    return CL_OK;
}

static ClRcT clJobQueuePushConditional(ClJobQueueT* hdl, ClCallbackT job, ClPtrT data, ClBoolT isEmpty)
{
    ClRcT rc = CL_OK;
    ClJobT* jb;

    if(!(hdl->flags & CreatedQueue)) return CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE);

    JQ_PFX(hdl);

    if (!(hdl->flags & Running)) { rc = CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE); goto out; }

    if(isEmpty)
    {
        ClUint32T queueSize = 0;
        (void)clQueueSizeGet(hdl->queue, &queueSize);
        if(queueSize) 
            goto out;
    }

    jb = getJob(hdl, job,data);
  
    if (!jb) { rc = CL_JOBQUEUE_RC(CL_ERR_NO_MEMORY); goto out; }

    rc = clQueueNodeInsert(hdl->queue,jb);

    if (rc != CL_OK) releaseJob(hdl,jb);
    else clTaskPoolWake(hdl->pool); /* Wake any idle pool tasks because there's work on the q (the idle callback handles the dequeuing) */
  
    out:
    JQ_SFX(hdl);
    return rc;
}

ClRcT clJobQueuePush(ClJobQueueT* hdl, ClCallbackT job, ClPtrT data)
{
    return clJobQueuePushConditional(hdl, job, data, CL_FALSE);
}

ClRcT clJobQueuePushIfEmpty(ClJobQueueT *hdl, ClCallbackT job, ClPtrT data)
{
    return clJobQueuePushConditional(hdl, job, data, CL_TRUE);
}

ClRcT clJobQueueStop(ClJobQueueT* hdl)
{
    ClJobT* jb=0;
    ClRcT rc;

    if( !(hdl->flags & CreatedQueue) ) return CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE);
    JQ_PFX(hdl);

    /* Clear out all pending jobs */
    do{
        ClQueueDataT temp;
        rc = clQueueNodeDelete(hdl->queue, &temp);
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            jb = (ClJobT*) temp;
            CL_ASSERT(jb);
            releaseJob(hdl,jb);
        }
    } while (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST);

    /* GAS TODO: Let all the tasks finish up */

    hdl->flags &= ~Running;

    rc = CL_OK;

    JQ_SFX(hdl);
    return rc;
}


ClRcT clJobQueueQuiesce(ClJobQueueT* hdl)
{  
    ClRcT rc;

    if(! (hdl->flags & CreatedQueue) ) return CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE);
    JQ_PFX(hdl);
    hdl->flags &= ~Running;

    clTaskPoolQuiesce(hdl->pool);

    rc = CL_OK;

    JQ_SFX(hdl);
    return rc;

}

static void jobQueueWalkCallback(ClQueueDataT data, ClPtrT arg)
{
    ClJobT *job = (ClJobT*)data;
    ClJobQueueWalkArgT *walkArg = arg;
    if(walkArg->cb)
    {
        walkArg->cb(job->job, job->data, walkArg->cbArg);
    }
}

ClRcT clJobQueueStatsGet(ClJobQueueT *hdl, ClJobQueueWalkCallbackT cb, ClPtrT arg, ClJobQueueUsageT *pJobQueueUsage)
{
    ClRcT rc = CL_OK;
    ClJobQueueWalkArgT walkArg = {0};
    if(!hdl || !pJobQueueUsage) return CL_JOBQUEUE_RC(CL_ERR_INVALID_PARAMETER);
    if(!(hdl->flags & CreatedQueue)) return CL_JOBQUEUE_RC(CL_ERR_INVALID_STATE);
    JQ_PFX(hdl);
    rc = clQueueSizeGet(hdl->queue, &pJobQueueUsage->numMsgs);
    if(rc != CL_OK)
    {
        pJobQueueUsage->numMsgs = 0;
        goto out;
    }
    rc = clTaskPoolStatsGet(hdl->pool, &pJobQueueUsage->taskPoolUsage);
    if(rc != CL_OK)
        goto out;

    if(cb)
    {
        walkArg.cb = cb;
        walkArg.cbArg = arg;
        rc = clQueueWalk(hdl->queue, jobQueueWalkCallback, &walkArg);
    }

    out:
    JQ_SFX(hdl);
    return rc;
}
