#!/usr/bin/env python3
import os, ctypes
from clPythonBindings import clLib

# load libClCompCfg_so and libmw_so first
filename = os.path.basename(__file__)
compName = os.path.splitext(filename)[0]
soClCompConfigCompName = "libClCompConfig" + compName + ".so"
binDirPath = os.path.dirname(os.path.abspath(__file__))
libDirPath = binDirPath + os.sep + "../lib/"
solibClCompConfigPath = libDirPath + soClCompConfigCompName
solibmwPath = libDirPath + "libmw.so"

clLib.libClCompCfg_so = ctypes.CDLL(solibClCompConfigPath, mode=ctypes.RTLD_GLOBAL)
clLib.libmw_so = ctypes.CDLL(solibmwPath, mode=ctypes.RTLD_GLOBAL)

from clPythonBindings.saAmf import eSaAmfHAStateT, SaAmfHandleT, SaAmfCallbacksT, SaAmfHealthcheckCallbackT,\
    SaAmfComponentTerminateCallbackT, SaAmfCSISetCallbackT, SaAmfCSIRemoveCallbackT, SaAmfProtectionGroupTrackCallbackT,\
    saAmfInitialize, saAmfSelectionObjectGet, saAmfComponentNameGet, saAmfComponentRegister, saAmfDispatch, saAmfFinalize,\
    saAmfComponentUnregister, saAmfResponse, saAmfCSIQuiescingComplete
from clPythonBindings.clAmsUtils import CL_AMS_STRING_CSI_FLAGS, CL_AMS_STRING_H_STATE
from clPythonBindings.saAis import SaNameT, SaVersionT, eSaAisErrorT, SaSelectionObjectT, eSaDispatchFlagsT
from clPythonBindings.clCommon import ClHandleT, CL_TRUE, CL_FALSE
from clPythonBindings.clLogApi import clLogMsgWrite, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, eClLogSeverityT
from clPythonBindings.clUtils import getCallerInfo
from clPythonBindings.libc import fd_set, getpid, FD_ZERO, FD_SET, select, errno
from clPythonBindings.clIocApi import ClIocPortT, clIocLocalAddressGet
from clPythonBindings.clEoApi import clEoMyEoIocPortGet

from clPythonBindings import saAmf, saAis, clCommon, clLogApi, clUtils, clIocApi, clEoApi, libc, clCompCfg

CL_LOG_HANDLE_APP = ClHandleT.in_dll(clLib.libmw_so, "CL_LOG_HANDLE_APP")

def clprintf(severity, fmtString, *va_args):
    _, fileName, loc = getCallerInfo()

    clLogMsgWrite(
        CL_LOG_HANDLE_APP,
        severity,
        10,
        CL_LOG_AREA_UNSPECIFIED,
        CL_LOG_CONTEXT_UNSPECIFIED,
        fileName,
        loc,
        fmtString,
        *va_args
    )

pid_t = ctypes.c_int

mypid = pid_t(0)
amfHandle = SaAmfHandleT(0)
appName = SaNameT("")

unblockNow = CL_FALSE

#
#   Declare other global variables here.
#

from clPythonBindings import clAmsClientNotification, clCpmApi, clCommonErrors, clAmsUtils

############################################################################
#   Application Life Cycle Management Functions.
############################################################################

#
#   main
#   -------------------
#   This function is invoked when the application is to be initialized.
#

def main():
    callbacks = SaAmfCallbacksT()
    version = SaVersionT('B', 1, 1)
    iocPort = ClIocPortT(0)
    rc = eSaAisErrorT.SA_AIS_OK

    dispatch_fd = SaSelectionObjectT(0)
    read_fds = fd_set()

    #
    #   Declare other local variables here.
    #

    global mypid
    global amfHandle
    global appName
    mypid = getpid()

    callbacks.saAmfHealthcheckCallback          = SaAmfHealthcheckCallbackT() # NULL
    callbacks.saAmfComponentTerminateCallback   = SaAmfComponentTerminateCallbackT(clCompAppTerminate)
    callbacks.saAmfCSISetCallback               = SaAmfCSISetCallbackT(clCompAppAMFCSISet)
    callbacks.saAmfCSIRemoveCallback            = SaAmfCSIRemoveCallbackT(clCompAppAMFCSIRemove)
    callbacks.saAmfProtectionGroupTrackCallback = SaAmfProtectionGroupTrackCallbackT() # NULL

    rc = saAmfInitialize(amfHandle, callbacks, version)

    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    FD_ZERO(read_fds)

    rc = saAmfSelectionObjectGet(amfHandle, dispatch_fd)

    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    FD_SET(dispatch_fd, read_fds)

    #
    # Do the application specific initialization here.
    #

    pycallback = clAmsClientNotification.ClAmsClientNotificationCallbackT(amsNotficationCallback)
    clAmsNotificationInitialize(pycallback)

    rc = saAmfComponentNameGet(amfHandle, appName)
    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    rc = saAmfComponentRegister(amfHandle, appName, None)
    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    rc = clEoMyEoIocPortGet(iocPort)

    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Component [%.*s] : PID [%d]. Initializing\n", appName.length, appName.__str__(), mypid)
    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "   IOC Address             : 0x%x\n", clIocLocalAddressGet())
    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "   IOC Port                : 0x%x\n", iocPort)

    EINTR = 4   # Interrupted system call
    while(not unblockNow):
        if select(dispatch_fd + 1, read_fds, None, None, None) < 0:
            if errno() == EINTR:
                continue
            clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Error in select()")
            break
        saAmfDispatch(amfHandle, eSaDispatchFlagsT.SA_DISPATCH_ALL)

    #
    # Do the application specific finalization here.
    #

    clAmsNotificationFinalize()

    rc = saAmfFinalize(amfHandle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "AMF finalization error[0x%X]", rc)
    clprintf (eClLogSeverityT.CL_LOG_SEV_INFO, "AMF Finalized")

def errorexit(rc):
    clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Component [%.*s] : PID [%d]. Initialization error [0x%x]\n", appName.length, appName.__str__(), mypid, rc)
    exit()

#
# clCompAppTerminate
# ------------------
# This function is invoked when the application is to be terminated.
#

def clCompAppTerminate(invocation, compName):
    invocation = ctypes.c_ulonglong(invocation)
    rc = eSaAisErrorT.SA_AIS_OK

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. Terminating\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    rc = saAmfComponentUnregister(amfHandle, compName.contents, None)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_ERROR,
            "Component [%.*s] : PID [%d]. Termination error [0x%x]\n",
            compName.contents.length, compName.contents.__str__(), mypid, rc
        )
        return
    
    saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. Terminated\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )
    
    global unblockNow
    unblockNow = CL_TRUE

#
# clCompAppAMFCSISet
# ------------------
# This function is invoked when a CSI assignment is made or the state
# of a CSI is changed.
#

def clCompAppAMFCSISet(invocation, compName, haState, csiDescriptor):
    invocation = ctypes.c_ulonglong(invocation)

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Set Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    clCompAppAMFPrintCSI(csiDescriptor, haState)

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCED:
        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCING:
        saAmfCSIQuiescingComplete(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    else:
        exit(0)

#
# clCompAppAMFCSIRemove
# ---------------------
# This function is invoked when a CSI assignment is to be removed.
#

def clCompAppAMFCSIRemove(invocation, compName, csiName, csiFlags):
    invocation = ctypes.c_ulonglong(invocation)
    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Remove Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )
    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "   CSI                     : %.*s\n",
        csiName.contents.length, csiName.contents.__str__()
    )
    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "   CSI Flags               : 0x%d\n",
        csiFlags
    )

    saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)

############################################################################
#   Utility functions .
############################################################################

#
# clCompAppAMFPrintCSI
# --------------------
# Print information received in a CSI set request.
#

def clCompAppAMFPrintCSI(csiDescriptor, haState):
    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "CSI Flags : [%s]",
        CL_AMS_STRING_CSI_FLAGS(csiDescriptor.csiFlags)
    )

    if csiDescriptor.csiFlags != saAmf.SA_AMF_CSI_TARGET_ALL:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "CSI Name : [%s]",
            csiDescriptor.csiName.__str__()
        )

    if csiDescriptor.csiFlags == saAmf.SA_AMF_CSI_ADD_ONE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Name value pairs :"
        )
        for i in range(0, csiDescriptor.csiAttr.number):
            clprintf(
                eClLogSeverityT.CL_LOG_SEV_INFO,
                "Name : [%s]",
                csiDescriptor.csiAttr.attr[i].attrName
            )
            clprintf(
                eClLogSeverityT.CL_LOG_SEV_INFO,
                "Value : [%s]",
                csiDescriptor.csiAttr.attr[i].attrValue
            )

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "HA state : [%s]",
        CL_AMS_STRING_H_STATE(haState)
    )

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Active Descriptor :"
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Transition Descriptor : [%d]",
            csiDescriptor.csiStateDescriptor.activeDescriptor.transitionDescriptor
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Active Component : [%s]",
            csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.__str__()
        )
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Standby Descriptor :"
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Standby Rank : [%d]",
            csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Active Component : [%s]",
            csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.__str__()
        )

#
# Insert any other utility functions here.
#

def CL_AMS_STRING_NTF(S):
    if S == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED:
        return "partially assigned"
    elif S == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED:
        return "fully assigned"
    elif S == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_UNASSIGNED:
        return "unassigned"
    else:
        "unkown"

def amsNotficationCallback(notification):
    """
        ClAmsNotificationInfoT *notification
        return: ClRcT
    """

    ntfObj = notification.contents
    ntfType = ntfObj.type
    if (ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED
        or ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_UNASSIGNED):
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Received SI [%s] event",
            CL_AMS_STRING_NTF(ntfType)
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "SI name : [%.*s]",
            ntfObj.amsNotificationInfo.amsStateInfo.siName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.siName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "SU name : [%.*s]",
            ntfObj.amsNotificationInfo.amsStateInfo.suName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.suName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Last HA State : [%s]",
            clAmsUtils.CL_AMS_STRING_H_STATE(ntfObj.amsNotificationInfo.amsStateInfo.lastHAState),
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "New HA State : [%s]",
            clAmsUtils.CL_AMS_STRING_H_STATE(ntfObj.amsNotificationInfo.amsStateInfo.newHAState),
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Received SU HA state change event"
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "SU name : [%.*s]",
            ntfObj.amsNotificationInfo.amsStateInfo.suName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.suName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "SI name : [%.*s]",
            ntfObj.amsNotificationInfo.amsStateInfo.siName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.siName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Last HA State : [%s]",
            clAmsUtils.CL_AMS_STRING_H_STATE(ntfObj.amsNotificationInfo.amsStateInfo.lastHAState),
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "New HA State : [%s]",
            clAmsUtils.CL_AMS_STRING_H_STATE(ntfObj.amsNotificationInfo.amsStateInfo.newHAState),
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_OPER_STATE_CHANGE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Received operational state [%s - %s] notification for type [%s] entity [%.*s]",
            clAmsUtils.CL_AMS_STRING_O_STATE(ntfObj.amsNotificationInfo.amsStateInfo.lastOperState),
            clAmsUtils.CL_AMS_STRING_O_STATE(ntfObj.amsNotificationInfo.amsStateInfo.newOperState),
            clAmsUtils.CL_AMS_STRING_ENTITY_TYPE(ntfObj.amsNotificationInfo.amsStateInfo.entityType),
            ntfObj.amsNotificationInfo.amsStateInfo.entityName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.entityName.value,
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_ADMIN_STATE_CHANGE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Received admin state [%s - %s] notification for type [%s] entity [%.*s]",
            clAmsUtils.CL_AMS_STRING_O_STATE(ntfObj.amsNotificationInfo.amsStateInfo.lastAdminState),
            clAmsUtils.CL_AMS_STRING_O_STATE(ntfObj.amsNotificationInfo.amsStateInfo.newAdminState),
            clAmsUtils.CL_AMS_STRING_ENTITY_TYPE(ntfObj.amsNotificationInfo.amsStateInfo.entityType),
            ntfObj.amsNotificationInfo.amsStateInfo.entityName.length,
            ntfObj.amsNotificationInfo.amsStateInfo.entityName.value,
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_COMP_ARRIVAL:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Component arrival for [%.*s]",
            ntfObj.amsNotificationInfo.amsCompInfo.compName.length,
            ntfObj.amsNotificationInfo.amsCompInfo.compName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Comp node [%.*s]",
            ntfObj.amsNotificationInfo.amsCompInfo.nodeName.length,
            ntfObj.amsNotificationInfo.amsCompInfo.nodeName.value,
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_COMP_DEPARTURE:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Component [%s] for [%.*s]",
            "death" if ntfObj.amsNotificationInfo.amsCompInfo.operation == clCpmApi.eClCpmCompEventT.CL_CPM_COMP_DEATH else "departure",
            ntfObj.amsNotificationInfo.amsCompInfo.compName.length,
            ntfObj.amsNotificationInfo.amsCompInfo.compName.value,
        )
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_NOTICE,
            "Comp node [%.*s]",
            ntfObj.amsNotificationInfo.amsCompInfo.nodeName.length,
            ntfObj.amsNotificationInfo.amsCompInfo.nodeName.value,
        )
    elif ntfType == clAmsClientNotification.eClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NODE_DEPARTURE:
        operation = ntfObj.amsNotificationInfo.amsNodeInfo.operation
        if (operation == clCpmApi.eClCpmNodeEventT.CL_CPM_NODE_DEPARTURE
            or operation == clCpmApi.eClCpmNodeEventT.CL_CPM_NODE_DEATH):
            clprintf(
                eClLogSeverityT.CL_LOG_SEV_NOTICE,
                "Node [%s] for [%.*s], address [%d]",
                "death" if operation == clCpmApi.eClCpmNodeEventT.CL_CPM_NODE_DEATH else "departure",
                ntfObj.amsNotificationInfo.amsNodeInfo.nodeName.length,
                ntfObj.amsNotificationInfo.amsNodeInfo.nodeName.value,
                ntfObj.amsNotificationInfo.amsNodeInfo.nodeIocAddress
            )

    return clCommonErrors.CL_OK

def clAmsNotificationInitialize(callback):
    rc = clAmsClientNotification.clAmsClientNotificationInitialize(callback)
    if rc != clCommonErrors.CL_OK:
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_WARNING,
            "AMF notification initialize returned with [%#x]",
            rc
        )
    return rc

def clAmsNotificationFinalize():
    clAmsClientNotification.clAmsClientNotificationFinalize()

#
# Python entry point
#
if __name__ == "__main__":
    main()