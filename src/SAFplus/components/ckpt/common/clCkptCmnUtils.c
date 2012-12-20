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
 * File        : clCkptCmnUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains common utility routines for server and client
*
*
 ***************************************************************************/
#include <string.h>
#include <clCommon.h>
#include  <clIocApi.h>
#include  <clIocIpi.h>
#include  <clLogApi.h>
#include  <clIdlApi.h>

#include  <clCkptUtils.h>
#include  <clCkptErrors.h>
#include  <clDebugApi.h>
#include  <clCkptCommon.h>
#include  <clLeakyBucket.h>
#include  <clIocLogicalAddresses.h>

/**=========================================================**/
/**         Utility Routines                                **/
/**=========================================================**/


/* 
 * Routine to Log a checkpoint error
 */
 
void clCkptLogError(ClUint32T   logLvl, 
                    ClRcT       retCode, 
                    ClUint32T   libCode)
{
    ClLogSeverityT severity = CL_LOG_ERROR;
    ClCharT        *libName = "clAspCkptLib";

    if (logLvl  == CL_DEBUG_CRITICAL) severity = CL_LOG_CRITICAL; 
    if (libCode == CL_CKPT_SVR) libName = NULL;
    switch(CL_GET_ERROR_CODE(retCode))
    {
    case CL_ERR_NO_MEMORY :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            break;       
    case CL_ERR_NULL_POINTER:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_NULL_ARGUMENT);
            break;       
    case  CL_ERR_NOT_EXIST:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_RESOURCE_NON_EXISTENT);
            break;       
    case  CL_ERR_INVALID_HANDLE:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_INVALID_HANDLE);
            break;       
    case  CL_ERR_INVALID_BUFFER:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_INVALID_BUFFER);
            break;       
    case  CL_ERR_DUPLICATE:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_DUPLICATE_ENTRY);
            break;       
    case  CL_ERR_OUT_OF_RANGE:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_PARAM_OUT_OF_RANGE);
            break;       
    case  CL_ERR_NO_RESOURCE:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_RESOURCE_UNAVAILABLE);
            break;       
    case  CL_ERR_INITIALIZED:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_COMPONENT_INITIALIZED);
            break;       
    case  CL_ERR_BUFFER_OVERRUN:
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_BUFFER_OVERRUN);
            break;       
    case CL_ERR_NOT_INITIALIZED :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);
            break;       
    case CL_ERR_VERSION_MISMATCH :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_VERSION_MISMATCH);
            break;       
    case CL_ERR_ALREADY_EXIST :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_ENTRY_ALREADY_EXISTING);
            break;       
    case CL_ERR_INVALID_STATE :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_INVALID_STATE);
            break;       
    case CL_ERR_INUSE :
            clLogWrite(CL_LOG_HANDLE_APP, severity, 
                    libName,CL_LOG_MESSAGE_0_RESOURCE_BUSY);
            break;       
    default:
            break;
    }
    return;
}



/*
 * Handle delete related callback function.
 */
 
void    ckptHdlDeleteCallback(ClCntKeyHandleT  userKey, 
                              ClCntDataHandleT userData)
{
    clHeapFree(userData);
    return;
}



/*
 * Function to derive logical address.
 */
 
void ckptOwnLogicalAddressGet(ClIocLogicalAddressT * logicalAddress)
{
    *logicalAddress = CL_IOC_CKPT_LOGICAL_ADDRESS;
}



/*
 * Function for getting the master address.
 */
 
ClRcT clCkptMasterAddressGet(ClIocNodeAddressT *pIocAddress)
{
    ClRcT                rc          = CL_OK;
    ClIocLogicalAddressT logicalAddr = 0 ;
    ClInt32T maxRetries = 5;
    /*
     * Validate the input parameters.
     */
    if (pIocAddress == NULL)
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                       ("Ckpt: clCkptMasterAddressGet failed rc[0x %x]\n",rc), rc);
    }
    
    /*
     *  Generate the Logical address 
     */
    ckptOwnLogicalAddressGet(&logicalAddr);
    
    rc = clIocMasterAddressGetExtended(logicalAddr,CL_IOC_CKPT_PORT,pIocAddress, maxRetries, NULL);
    if(rc != CL_OK)
    {
        rc = CL_CKPT_ERR_NOT_EXIST;
    }

    exitOnError:
    return rc;
}



/*
 * Function for updating idl handle with passed parameters.
 */
ClRcT ckptIdlHandleUpdate(ClIocNodeAddressT nodeId,
                          ClIdlHandleT      handle,
                          ClUint32T         numRetries)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK;
    ClUint32T timeout  = CKPT_RMD_DFLT_TIMEOUT;
    /*
     * If running with traffic shaper, bump up the timeout since the payload is
     * expected to be large and CKPT is one of the key reasons. So respect!
     */
    if(gClIocTrafficShaper)
    {
        timeout = CL_MIN(60000, timeout*5);
    }
    /*
     * Set the idlObj.
     */
    memset(&address, 0, sizeof(ClIdlAddressT));
    address.addressType     = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = nodeId;
    address.address.iocAddress.iocPhyAddress.portId       = CL_IOC_CKPT_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = timeout;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = numRetries;
    
    rc = clIdlHandleUpdate(handle,&idlObj);
    
    return rc;
}

ClRcT clCkptClientIdlHandleInit(ClIdlHandleT  *pHdl)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK;
    ClUint32T timeout = CKPT_RMD_DFLT_TIMEOUT;
    if(gClIocTrafficShaper)
    {
        timeout = CL_MIN(timeout*5, 60000);
    }
    /*
     * Set the idlObj.
     */
    memset(&address, 0, sizeof(ClIdlAddressT));
    address.addressType     = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = 0;
    address.address.iocAddress.iocPhyAddress.portId       = CL_IOC_CKPT_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = timeout;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = 0;
    
    rc = clIdlHandleInitialize(&idlObj, pHdl);
    
    return rc;
}

ClRcT clCkptClientIdlHandleUpdate(ClIdlHandleT       idlHdl,
                                  ClIocNodeAddressT  nodeAddress,
                                  ClIocPortT         portId,
                                  ClUint32T          numRetries)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK;
    ClUint32T timeout = CKPT_RMD_DFLT_TIMEOUT;
    if(gClIocTrafficShaper)
    {
        timeout = CL_MIN(60000, timeout*5);
    }
    /*
     * Set the idlObj.
     */
    memset(&address, 0, sizeof(ClIdlAddressT));
    address.addressType     = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = nodeAddress;
    address.address.iocAddress.iocPhyAddress.portId       = portId;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = timeout;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = numRetries;
    
    rc = clIdlHandleUpdate(idlHdl, &idlObj);
    
    return rc;
}


/*
 * Leaky bucket initialize for CKPT.
 */

#ifdef CL_ENABLE_ASP_TRAFFIC_SHAPING

#define CL_LEAKY_BUCKET_DEFAULT_VOL (200*1024)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE (100*1024)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL (400)

ClRcT clCkptLeakyBucketInitialize(void)
{
    ClInt64T leakyBucketVol = getenv("CL_LEAKY_BUCKET_VOL") ? 
        (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_VOL")) : CL_LEAKY_BUCKET_DEFAULT_VOL;
    ClInt64T leakyBucketLeakSize = getenv("CL_LEAKY_BUCKET_LEAK_SIZE") ?
        (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_LEAK_SIZE")) : CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE;
    ClTimerTimeOutT leakyBucketInterval = {.tsSec = 0, .tsMilliSec = CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL };
    ClLeakyBucketWaterMarkT leakyBucketWaterMark = {0};

    leakyBucketWaterMark.lowWM = leakyBucketVol/3;
    leakyBucketWaterMark.highWM = leakyBucketVol/2;
    
    leakyBucketWaterMark.lowWMDelay.tsSec = 0;
    leakyBucketWaterMark.lowWMDelay.tsMilliSec = 50;

    leakyBucketWaterMark.highWMDelay.tsSec = 0;
    leakyBucketWaterMark.highWMDelay.tsMilliSec = 100;

    leakyBucketInterval.tsMilliSec = getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL") ? atoi(getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL")) :
        CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL;

    clLogInfo("LEAKY", "BUCKET-INI", "Creating a leaky bucket with vol [%lld], leak size [%lld], interval [%d ms]",
                 leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval.tsMilliSec);

    return clLeakyBucketCreateSoft(leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval, 
                                   &leakyBucketWaterMark, &gClLeakyBucket);

}

#else

ClRcT clCkptLeakyBucketInitialize(void)
{
    return CL_OK;
}

#endif
