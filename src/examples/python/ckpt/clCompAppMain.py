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

from clPythonBindings import saCkpt
from time import time
import socket, threading, time

seq = saAis.SaUint32T(0)
ha_state = 0

CKPT_NAME = "csa103Ckpt"
ckpt_lib_handle = saCkpt.SaCkptHandleT(0)
ckpt_handle = saCkpt.SaCkptCheckpointHandleT(0)
CKPT_SID_NAME = "csa103Ckpt_sec"
ckpt_sid = saCkpt.SaCkptSectionIdT(CKPT_SID_NAME)
ckpt_callbacks = saCkpt.SaCkptCallbacksT()
syncCount = saAis.SaInvocationT(0)

############################################################################
#   Application Life Cycle Management Functions.
############################################################################

def csa103CkptActive():
    global seq
    rc = eSaAisErrorT.SA_AIS_OK

    assert ha_state == saAmf.eSaAmfHAStateT.SA_AMF_HA_ACTIVE

    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Active thread has started")

    rc = saCkpt.saCkptActiveReplicaSet(ckpt_handle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "checkpoint_replica_activate failed [0x%x] in ActiveReplicaSet", rc)

    p_seq = ctypes.pointer(seq)
    checkpoint_read_seq(p_seq)

    while not unblockNow:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO,"Hello World! (seq=%d)", seq)
        seq.value += 1
        checkpoint_write_seq(seq)
        time.sleep(1)

    return eSaAisErrorT.SA_AIS_OK

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
    ckpt_dispatch_fd = saAis.SaSelectionObjectT(0)
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

    checkpoint_initialize()

    rc = saCkpt.saCkptSelectionObjectGet(
        ckpt_lib_handle,
        ckpt_dispatch_fd
    )

    if rc != eSaAisErrorT.SA_AIS_OK:
        errorexit(rc)

    EINTR = 4   # Interrupted system call
    while(not unblockNow):
        if select(dispatch_fd + 1, read_fds, None, None, None) < 0:
            if errno() == EINTR:
                continue
            clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Error in select()")
            break
        saAmfDispatch(amfHandle, eSaDispatchFlagsT.SA_DISPATCH_ALL)
        saCkpt.saCkptDispatch(ckpt_lib_handle, eSaDispatchFlagsT.SA_DISPATCH_ALL)

    #
    # Do the application specific finalization here.
    #


    checkpoint_finalize()

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
    global ha_state

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Set Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    clCompAppAMFPrintCSI(csiDescriptor, haState)

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Active state requested from state %d", ha_state)

        ha_state = eSaAmfHAStateT.SA_AMF_HA_ACTIVE

        thr = threading.Thread(target = csa103CkptActive)
        thr.start()

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Standby state requested from state %d", ha_state)
        ha_state = eSaAmfHAStateT.SA_AMF_HA_STANDBY

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCED:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "QUIESCED")
        ha_state = eSaAmfHAStateT.SA_AMF_HA_QUIESCED

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCING:
        clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "QUIESCING")
        ha_state = eSaAmfHAStateT.SA_AMF_HA_QUIESCING

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

def checkpointOpenCompleted(invocation, checkpointHandle, error):
    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Checkpoint open completed"
    )

def checkpointSynchronizeCompleted(invocation, error):
    invocation = saAis.SaUint64T(invocation)
    if ((invocation.value % 10) == 0):
        clprintf(
            eClLogSeverityT.CL_LOG_SEV_INFO,
            "Checkpoint synchronize [%llu] completed ",
            invocation
        )

def checkpoint_initialize():
    global ckpt_lib_handle
    global ckpt_handle
    global ckpt_callbacks

    rc = eSaAisErrorT.SA_AIS_OK
    ckpt_version = SaVersionT('B', 1, 1)
    ckpt_name = SaNameT(CKPT_NAME)
    attrs = saCkpt.SaCkptCheckpointCreationAttributesT()

    attrs.creationFlags     = saCkpt.SA_CKPT_WR_ACTIVE_REPLICA_WEAK | saCkpt.SA_CKPT_CHECKPOINT_COLLOCATED
    attrs.checkpointSize    = ctypes.sizeof(saAis.SaUint32T)
    attrs.retentionDuration = saAis.SaTimeT(10)
    attrs.maxSections       = saAis.SaUint32T(2)
    attrs.maxSectionSize    = ctypes.sizeof(saAis.SaUint32T)
    attrs.maxSectionIdSize  = saAis.SaSizeT(64)

    ckpt_callbacks.saCkptCheckpointOpenCallback = saCkpt.SaCkptCheckpointOpenCallbackT(checkpointOpenCompleted)
    ckpt_callbacks.saCkptCheckpointSynchronizeCallback = saCkpt.SaCkptCheckpointSynchronizeCallbackT(checkpointSynchronizeCompleted)

    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Checkpoint Initialize")

    rc = saCkpt.saCkptInitialize(ckpt_lib_handle, ckpt_callbacks, ckpt_version)

    if rc != saAis.eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed to initialize checkpoint service with rc [%#x]", rc)
        return rc

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO, "Checkpoint service initialized (handle=0x%llx)", ckpt_lib_handle)

    rc = saCkpt.saCkptCheckpointOpen(
        ckpt_lib_handle,
        ckpt_name,
        attrs,
        (saCkpt.SA_CKPT_CHECKPOINT_READ | saCkpt.SA_CKPT_CHECKPOINT_WRITE | saCkpt.SA_CKPT_CHECKPOINT_CREATE),
        saAis.SaTimeT(saAis.SA_TIME_MAX),
        ckpt_handle
    )

    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed [0x%x] to open checkpoint", rc)
        return rc

    clprintf(eClLogSeverityT.CL_LOG_SEV_INFO, "Checkpoint opened (handle=0x%llx)", ckpt_handle)
    return rc

def checkpoint_finalize():
    rc = eSaAisErrorT.SA_AIS_OK
    rc = saCkpt.saCkptCheckpointClose(ckpt_handle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed [0x%x] to close checkpoint handle 0x%llx", rc, ckpt_handle)

    rc = saCkpt.saCkptFinalize(ckpt_lib_handle)
    if rc != eSaAisErrorT.SA_AIS_OK:
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed [0x%x] to finalize checkpoint", rc)

def checkpoint_write_seq(seq):
    rc = eSaAisErrorT.SA_AIS_OK
    seq_no = saAis.SaUint32T(0)

    seq_no.value = socket.htonl(seq.value)

    rc = saCkpt.saCkptSectionOverwrite(ckpt_handle, ckpt_sid, clUtils.byref(seq_no), ctypes.sizeof(saAis.SaUint32T))

    if rc != eSaAisErrorT.SA_AIS_OK:
        if (rc == 0x1000a) or (rc == eSaAisErrorT.SA_AIS_ERR_NOT_EXIST):
            p_ckpt_sid = ctypes.pointer(ckpt_sid)
            section_cr_attr = saCkpt.SaCkptSectionCreationAttributesT(
                p_ckpt_sid,
                saAis.SA_TIME_END
            )
            p_seq_no = ctypes.pointer(seq_no)
            p_data = ctypes.cast(p_seq_no, ctypes.POINTER(saAis.SaUint8T))

            rc = saCkpt.saCkptSectionCreate(ckpt_handle, section_cr_attr, p_data, ctypes.sizeof(saAis.SaUint32T))

        if rc != eSaAisErrorT.SA_AIS_OK:
            clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Failed [0x%x] to write to section", rc)

    if rc == eSaAisErrorT.SA_AIS_OK:
        global syncCount
        rc = saCkpt.saCkptCheckpointSynchronizeAsync(ckpt_handle, syncCount)
        syncCount.value += 1

    return rc

def checkpoint_read_seq(seq):
    """
    arg: ClUint32T *, return: SaAisErrorT
    """
    rc = eSaAisErrorT.SA_AIS_OK
    err_idx = saAis.SaUint32T(0)
    seq_no = saAis.SaUint32T(0xFFFFFFFF)
    p_seq_no = ctypes.cast(ctypes.byref(seq_no), ctypes.c_void_p)

    iov = saCkpt.SaCkptIOVectorElementT(
        ckpt_sid,
        p_seq_no,
        ctypes.sizeof(saAis.SaUint32T),
        saAis.SaOffsetT(0),
        ctypes.sizeof(saAis.SaUint32T)
    )

    rc = saCkpt.saCkptCheckpointRead(ckpt_handle, iov, 1, err_idx)
    if rc != eSaAisErrorT.SA_AIS_OK:
        if rc != eSaAisErrorT.SA_AIS_ERR_NOT_EXIST:
            return eSaAisErrorT.SA_AIS_OK
        clprintf(eClLogSeverityT.CL_LOG_SEV_ERROR, "Error: [0x%x] from checkpoint read, err_idx = %u", rc, err_idx)

    seq.contents.value = socket.ntohl(seq_no.value) # *seq = ntohl(seq_no)
    return eSaAisErrorT.SA_AIS_OK

#
# Python entry point
#
if __name__ == "__main__":
    main()
