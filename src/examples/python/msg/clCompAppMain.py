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

from clPythonBindings import saMsg
import ctypes, threading, time

standby = clCommon.CL_FALSE

ACTIVE_COMP_QUEUE = "csa104msgqueue"
QUEUE_LENGTH = 2048
msgLibraryHandle = saMsg.SaMsgHandleT(0)
msgQueueHandle = saMsg.SaMsgQueueHandleT(0)

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

    msgInitialize()

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
    global standby

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Set Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    clCompAppAMFPrintCSI(csiDescriptor, haState)

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "csa104: ACTIVE state requested; activating message queue receiver service")
        msgOpen(ACTIVE_COMP_QUEUE, QUEUE_LENGTH)
        thrd = threading.Thread(target = msgReceiverLoop)
        thrd.start()

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        standby = clCommon.CL_TRUE

        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "csa104: Standby state requested")

        thrd = threading.Thread(target = senderLoop)
        thrd.start()

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCED:
        standby = clCommon.CL_FALSE

        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "csa104: Acknowledging new state quiesced")

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCING:
        standby = clCommon.CL_FALSE

        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "csa104: Signaling completion of QUIESCING")

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

def msgInitialize():
    rc = eSaAisErrorT.SA_AIS_OK
    MsgCallbacks = saMsg.SaMsgCallbacksT()

    MsgCallbacks.saMsgQueueOpenCallback = saMsg.SaMsgQueueOpenCallbackT()
    MsgCallbacks.saMsgQueueGroupTrackCallback = saMsg.SaMsgQueueGroupTrackCallbackT()
    MsgCallbacks.saMsgMessageDeliveredCallback = saMsg.SaMsgMessageDeliveredCallbackT()
    MsgCallbacks.saMsgMessageReceivedCallback = saMsg.SaMsgMessageReceivedCallbackT()

    version = SaVersionT('B', 1, 1)

    rc = saMsg.saMsgInitialize(msgLibraryHandle, MsgCallbacks, version)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR, "Init failed [0x%X]", rc)
        exit(0)

def msgOpen(queuename, bytesPerPriority):
    rc = eSaAisErrorT.SA_AIS_OK
    saQueueName = SaNameT(queuename)
    CreationAttributes = saMsg.SaMsgQueueCreationAttributesT()
    OpenFlags = saMsg.SaMsgQueueOpenFlagsT(saMsg.SA_MSG_QUEUE_CREATE)
    timeout = saAis.SaTimeT(saAis.SA_TIME_MAX)

    CreationAttributes.creationFlags = 0
    for i in range(0, saMsg.SA_MSG_MESSAGE_LOWEST_PRIORITY):
        CreationAttributes.size[i] = bytesPerPriority
    CreationAttributes.retentionTime = 0

    rc = saMsg.saMsgQueueOpen(
        msgLibraryHandle,
        saQueueName,
        CreationAttributes,
        OpenFlags,
        timeout,
        msgQueueHandle
    )

    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR, "Msg QueueOpen failed [0x%X]\n\r", rc)

    return rc

def msgSend(queuename, buffer, length):
    rc = eSaAisErrorT.SA_AIS_OK
    saQueueName = SaNameT(queuename)
    message = saMsg.SaMsgMessageT()
    
    temp = ctypes.cast(buffer, ctypes.c_void_p)

    message.type = 0
    message.version = SaVersionT('B', 0, 0)
    message.senderName = None
    message.size = length
    message.data = temp
    message.priority = saMsg.SA_MSG_MESSAGE_HIGHEST_PRIORITY

    rc = saMsg.saMsgMessageSend(
        msgLibraryHandle,
        saQueueName,
        message,
        saAis.SA_TIME_MAX
    )

    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR, "Msg saMsgMessageSend to queue [%s] failed [0x%X]", saQueueName.value, rc)

    return rc

def msgReceiverLoop():
    bufferT = ctypes.c_char * 1024

    rc = eSaAisErrorT.SA_AIS_OK
    SenderName = SaNameT("")
    data = bufferT()
    SenderId = saMsg.SaMsgSenderIdT()
    SendTime = saAis.SaTimeT()

    while True:
        p_data = ctypes.cast(data, ctypes.c_void_p)

        message = saMsg.SaMsgMessageT()
        message.size = 1024
        message.senderName.contents = SenderName
        message.data = p_data

        rc = saMsg.saMsgMessageGet(
            msgQueueHandle,
            message,
            SendTime,
            SenderId,
            saAis.SA_TIME_MAX
        )

        if rc != eSaAisErrorT.SA_AIS_OK:
            clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR, "Msg saMsgMessageGet failed [0x%X]\n\r", rc)
            break

        if message.senderName.contents.length > 0:
            clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_INFO, "Sender Name   : %s\n", message.senderName.contents.value)

        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_INFO, "Received Message  : %s\n", data)

    rc = saMsg.saMsgQueueClose(msgQueueHandle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR, "Msg Queue Close failed [0x%X]\n\r", rc)

def senderLoop():
    bufferT = ctypes.c_char * 100
    count = ctypes.c_int(0)
    msg = bufferT()

    while standby:
        count.value += 1
        libc.snprintf(msg, 99, "Msg %4d from %.*s", count,appName.length,appName.value)

        clprintf(clLogApi.eClLogSeverityT.CL_LOG_SEV_INFO, "csa104: Sending Message: %s", msg)

        msgSend(ACTIVE_COMP_QUEUE, msg, libc.strlen(msg) + 1)
        time.sleep(2)

#
# Python entry point
#
if __name__ == "__main__":
    main()
