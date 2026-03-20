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

from clPythonBindings import saEvt, clOsalApi, clCommonErrors
import time, datetime
import ctypes

running = True
evtChannelName = SaNameT("Example_channel")

evtChannelHandle = saEvt.SaEvtChannelHandleT()
evtLibHandle = saEvt.SaEvtHandleT()
eventHandle = saEvt.SaEvtEventHandleT()

evtCallbacks = saEvt.SaEvtCallbacksT()

generatorIdx = 0

############################################################################
#   Application Life Cycle Management Functions.
############################################################################

def saEvtDispatchThread(arg):
    saEvt.saEvtDispatch(evtLibHandle, saAis.eSaDispatchFlagsT.SA_DISPATCH_BLOCKING)
    return None

def publishEventThread(arg):
    global generatorIdx
    while running:
        eventId = saEvt.SaEvtEventIdT(0)

        generators = [generate_time_of_day, generate_load_average]
        gen = generators[generatorIdx]

        generatorIdx = (generatorIdx + 1) % len(generators)

        content = gen()
        dataLen = len(content) + 1
        pData = clUtils.toCharP(content)

        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Publishing Event: %.*s", dataLen, content)
        rc = saEvt.saEvtEventPublish(eventHandle, pData, dataLen, eventId)
        if rc != eSaAisErrorT.SA_AIS_OK:
            clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Event publish attempt failed with error [%x]", rc)

        time.sleep(1)

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

    rc = saAmfComponentNameGet(amfHandle, appName)
    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    rc = saAmfComponentRegister(amfHandle, appName, None)
    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    eventInitialize()

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
    global running

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Set Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    clCompAppAMFPrintCSI(csiDescriptor, haState)

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        running = True
        threadRoutine = clOsalApi.TaskFunctionT(publishEventThread)
        rc = clOsalApi.clOsalTaskCreateDetached(
            "EVT-PUBLISH-THREAD",
            clOsalApi.eClOsalSchedulePolicyT.CL_OSAL_SCHED_OTHER,
            0,
            0,
            threadRoutine,
            None
        )
        if rc != clCommonErrors.CL_OK:
            clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Publish task create failed with rc 0x%x", rc)
            exit(rc)

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        running = False
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
    global running
    running = False
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

def eventInitialize():
    threadRoutine = clOsalApi.TaskFunctionT(saEvtDispatchThread)
    rc = clOsalApi.clOsalTaskCreateDetached(
        "EVT-DISPATCH-THREAD",
        clOsalApi.eClOsalSchedulePolicyT.CL_OSAL_SCHED_OTHER,
        0,
        0,
        threadRoutine,
        None
    )
    if rc != clCommonErrors.CL_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Dispatch task create failed with rc 0x%x", rc)
        exit(rc)

    evtCallbacks.saEvtChannelOpenCallback = saEvt.SaEvtChannelOpenCallbackT()
    evtCallbacks.saEvtEventDeliverCallback = saEvt.SaEvtEventDeliverCallbackT(appEventCallback)

    evtVersion = SaVersionT('B', 1, 1)

    rc = saEvt.saEvtInitialize(evtLibHandle, evtCallbacks, evtVersion)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to init event mechanism [0x%x]", rc)
        exit(rc)

    rc = saEvt.saEvtChannelOpen(
        evtLibHandle,
        evtChannelName,
        (saEvt.SA_EVT_CHANNEL_PUBLISHER | saEvt.SA_EVT_CHANNEL_SUBSCRIBER | saEvt.SA_EVT_CHANNEL_CREATE),
        saAis.SaTimeT(saAis.SA_TIME_END),
        evtChannelHandle
    )
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failure opening event channel [0x%x]", rc)
        exit(rc)

    rc = saEvt.saEvtEventSubscribe(evtChannelHandle, None, 1)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to subscribe to event channel [0x%x]", rc)
        exit(rc)

    publisherName = SaNameT("Example_publisher")
    rc = saEvt.saEvtEventAllocate(evtChannelHandle, eventHandle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to allocate event [0x%x]", rc)
        exit(rc)

    rc = saEvt.saEvtEventAttributesSet(eventHandle, None, 1, 0, publisherName)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to set event attributes [0x%x]", rc)
        exit(rc)

def generate_time_of_day():
    return datetime.datetime.now().isoformat()

def generate_load_average():
    with open("/proc/loadavg", "r") as f:
        rs = f.read()
    rs = rs[0:-1]   #get rid of newline character
    return rs

def appEventCallback(subscriptionId, eventHandle, eventDataSize):
    eventDataSize = saAis.SaSizeT(eventDataSize)
    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "We've got an event to receive")
    res = (clCommon.ClCharT * 100)()    # char res[100]
    rc = saEvt.saEvtEventDataGet(eventHandle, res, eventDataSize)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to get event data [0x%x]", rc)
        return
    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "received event: %s", res.value)

#
# Python entry point
#
if __name__ == "__main__":
    main()
