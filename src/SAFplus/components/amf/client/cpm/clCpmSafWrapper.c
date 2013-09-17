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

/**
 * This file implements wrapper calls on OpenClovis APIs to provide
 * SAF compliant APIs for AMF.
 */

#include <clCpmApi.h>

#include <clSafUtils.h>
#include <saAmf.h>
#include <clEoIpi.h>
#include <clAmfTestHooks.h>

SaAisErrorT saAmfInitialize(SaAmfHandleT *amfHandle,
                            const SaAmfCallbacksT *amfCallbacks,
                            SaVersionT *version)
{
    ClRcT rc;
    
    CL_AMF_TEST_HOOK_INITIALIZE(amfHandle, amfCallbacks, version);

    rc = clASPInitialize();
    if(CL_OK != rc)
    {
        return clClovisToSafError(rc);
    }
    
    rc = clCpmClientInitialize((ClCpmHandleT *)amfHandle,
                               (ClCpmCallbacksT *)amfCallbacks,
                               (ClVersionT *)version);
    
    if(rc == CL_OK)
    {
        ClSelectionObjectT dispatchFd = 0;
        rc = clCpmSelectionObjectGet(*amfHandle, &dispatchFd);
    }

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfFinalize(SaAmfHandleT amfHandle)
{
    ClRcT rc;

    rc = clCpmClientFinalize(amfHandle);

    rc = clASPFinalize();
    
    return clClovisToSafError(rc);
}

SaAisErrorT saAmfComponentRegister(SaAmfHandleT amfHandle,
                                   const SaNameT *compName,
                                   const SaNameT *proxyCompName)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};
    SaNameT proxyCompNameInternal = {0};
    SaNameT *proxy = (SaNameT*)proxyCompName;

    if(!compName) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal, (const SaNameT*)compName);
    
    if(proxyCompName)
    {
        saNameCopy((SaNameT*)&proxyCompNameInternal, 
                   (const SaNameT*)proxyCompName);
        proxy = &proxyCompNameInternal;
    }
    rc = clCpmComponentRegister(amfHandle,
                                (const SaNameT*)&compNameInternal,
                                (const SaNameT*)proxy);
    return clClovisToSafError(rc);
}

SaAisErrorT saAmfComponentUnregister(SaAmfHandleT amfHandle,
                                     const SaNameT *compName,
                                     const SaNameT *proxyCompName)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};
    SaNameT proxyCompNameInternal = {0};
    SaNameT *proxy = (SaNameT*)proxyCompName;
    
    if(!compName) return SA_AIS_ERR_INVALID_PARAM;
    
    saNameCopy((SaNameT*)&compNameInternal, (const SaNameT*)compName);

    if(proxyCompName)
    {
        saNameCopy((SaNameT*)&proxyCompNameInternal, 
                   (const SaNameT*)proxyCompName);
        proxy = &proxyCompNameInternal;
    }

    rc = clCpmComponentUnregister(amfHandle,
                                  (const SaNameT*)&compNameInternal,
                                  (const SaNameT*)proxy);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfComponentNameGet(SaAmfHandleT amfHandle,
                                  SaNameT *compName)
{
    ClRcT rc;
    
    rc = clCpmComponentNameGet(amfHandle, (SaNameT *)compName);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfSelectionObjectGet(SaAmfHandleT amfHandle, 
                                    SaSelectionObjectT *selectionObject)
{
    ClRcT rc;
    
    rc = clCpmSelectionObjectGet(amfHandle, selectionObject);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfDispatch(SaAmfHandleT amfHandle, 
                          SaDispatchFlagsT dispatchFlags)
{
    ClRcT rc;
    
    rc = clCpmDispatch(amfHandle, dispatchFlags);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfCSIQuiescingComplete(SaAmfHandleT amfHandle,
                                      SaInvocationT invocation,
                                      SaAisErrorT error)
{
    ClRcT rc;

    rc = clCpmCSIQuiescingComplete(amfHandle,
                                   invocation,
                                   clSafToClovisError(error));

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfHAStateGet(SaAmfHandleT amfHandle,
                            const SaNameT *compName,
                            const SaNameT *csiName,
                            SaAmfHAStateT *haState)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};
    SaNameT csiNameInternal = {0};

    if(!compName || !csiName || !haState) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal, 
               (const SaNameT*)compName);

    saNameCopy((SaNameT*)&csiNameInternal,
               (const SaNameT*)csiName);

    rc = clCpmHAStateGet(amfHandle,
                         (SaNameT *)&compNameInternal,
                         (SaNameT *)&csiNameInternal,
                         (ClAmsHAStateT *)haState);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfProtectionGroupTrack(SaAmfHandleT amfHandle,
                                      const SaNameT *csiName,
                                      SaUint8T trackFlags,
                                      SaAmfProtectionGroupNotificationBufferT
                                      *notificationBuffer)
{
    ClRcT rc;
    SaNameT csiNameInternal = {0};
    
    if(!csiName) return SA_AIS_ERR_INVALID_PARAM;
    
    saNameCopy((SaNameT*)&csiNameInternal, 
               (const SaNameT*)csiName);

    rc = clCpmProtectionGroupTrack(amfHandle,
                                   (SaNameT *)&csiNameInternal,
                                   trackFlags, 
                                   (ClAmsPGNotificationBufferT *)
                                   notificationBuffer);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfProtectionGroupTrackStop(SaAmfHandleT amfHandle,
                                          const SaNameT *csiName)
{
    ClRcT rc;
    SaNameT csiNameInternal = {0};
    
    if(!csiName) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&csiNameInternal,
               (const SaNameT*)csiName);

    rc = clCpmProtectionGroupTrackStop(amfHandle, (SaNameT *)&csiNameInternal);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfComponentErrorReport(SaAmfHandleT amfHandle,
                                      const SaNameT *erroneousComponent,
                                      SaTimeT errorDetectionTime,
                                      SaAmfRecommendedRecoveryT
                                      recommendedRecovery,
                                      SaNtfIdentifierT ntfIdentifier)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};

    if(!erroneousComponent) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal,
               (const SaNameT*)erroneousComponent);

    rc = clCpmComponentFailureReport(amfHandle,
                                     (const SaNameT *)&compNameInternal,
                                     errorDetectionTime,
                                     recommendedRecovery,
                                     (ClUint32T)ntfIdentifier);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfComponentErrorClear(SaAmfHandleT amfHandle,
                                     const SaNameT *compName,
                                     SaNtfIdentifierT ntfIdentifier)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};

    if(!compName) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal, 
               (const SaNameT*)compName);

    rc = clCpmComponentFailureClear(amfHandle, (SaNameT *)&compNameInternal);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfResponse(SaAmfHandleT amfHandle,
                          SaInvocationT invocation,
                          SaAisErrorT error)
{
    ClRcT rc;

    rc = clCpmResponse(amfHandle, invocation, clSafToClovisError(error));

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfHealthcheckStart(SaAmfHandleT amfHandle, 
                                  const SaNameT *compName,
                                  const SaAmfHealthcheckKeyT *healthcheckKey,
                                  SaAmfHealthcheckInvocationT invocationType,
                                  SaAmfRecommendedRecoveryT recommendedRecovery)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};

    if(!compName) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal,
               (const SaNameT*)compName);

    rc = clCpmHealthcheckStart(amfHandle,
                               (SaNameT *)&compNameInternal,
                               (ClAmsCompHealthcheckKeyT *)healthcheckKey,
                               invocationType,
                               recommendedRecovery);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfHealthcheckStop(SaAmfHandleT amfHandle,
                                 const SaNameT *compName,
                                 const SaAmfHealthcheckKeyT *healthcheckKey)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};

    if(!compName) return SA_AIS_ERR_INVALID_PARAM;
    
    saNameCopy((SaNameT*)&compNameInternal,
               (const SaNameT*)compName);

    rc = clCpmHealthcheckStop(amfHandle,
                              (SaNameT *)&compNameInternal,
                              (ClAmsCompHealthcheckKeyT *)healthcheckKey);

    return clClovisToSafError(rc);
}

SaAisErrorT saAmfHealthcheckConfirm(SaAmfHandleT amfHandle,
                                    const SaNameT *compName,
                                    const SaAmfHealthcheckKeyT *healthcheckKey,
                                    SaAisErrorT healthcheckResult)
{
    ClRcT rc;
    SaNameT compNameInternal = {0};

    if(!compName) return SA_AIS_ERR_INVALID_PARAM;

    saNameCopy((SaNameT*)&compNameInternal,
               (const SaNameT*)compName);

    rc = clCpmHealthcheckConfirm(amfHandle,
                                 (SaNameT *)&compNameInternal,
                                 (ClAmsCompHealthcheckKeyT *)healthcheckKey,
                                 clSafToClovisError(healthcheckResult));

    return clClovisToSafError(rc);
}
