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

from clPythonBindings import clAmsMgmtClientApi, clAmsTypes, clAmsEntities, clAmsMgmtCommon
from clPythonBindings import clCommonErrors, clHandleApi, clOsalApi, clHeapApi
import ctypes

WORKER0 = "WorkerI0"
WORKER1 = "WorkerI1"
NEW_COMP_PREFIX = "dynamicComp"
BASE_NAME = "dynamicTwoN"

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

    clprintf(
        eClLogSeverityT.CL_LOG_SEV_INFO,
        "Component [%.*s] : PID [%d]. CSI Set Received\n",
        compName.contents.length, compName.contents.__str__(), mypid
    )

    clCompAppAMFPrintCSI(csiDescriptor, haState)

    if haState == eSaAmfHAStateT.SA_AMF_HA_ACTIVE:
        dhaInfoPrint("Starting dynamic HA demo.")
        dhaInfoPrint("It will create 2N SG : %sSG", BASE_NAME)
        rc = clDhaDemoStart()
        if rc != clCommonErrors.CL_OK:
            dhaErrorPrint("Failed to start dynamic HA demo.")

        saAmf.saAmfResponse(amfHandle, invocation, saAis.eSaAisErrorT.SA_AIS_OK)

        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_STANDBY:
        saAmfResponse(amfHandle, invocation, eSaAisErrorT.SA_AIS_OK)
    elif haState == eSaAmfHAStateT.SA_AMF_HA_QUIESCED:
        rc = clDhaDemoStop()
        if rc != clCommonErrors.CL_OK:
            dhaErrorPrint("Failed to stop dynamic HA demo.")

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

def clDhaDemoCreate(arg):
    mgmtHandle = clAmsTypes.ClAmsMgmtHandleT(0)
    ccbHandle = clAmsTypes.ClAmsMgmtCCBHandleT(clHandleApi.CL_HANDLE_INVALID_VALUE)
    pBaseName = BASE_NAME

    version = clCommon.ClVersionT()
    version.releaseCode = ord('B')
    version.majorVersion = 0x1
    version.minorVersion = 0x1

    rc = dhaMgmtInit(mgmtHandle, version)
    if rc != clCommonErrors.CL_OK: return None
    rc = dhaMgmtCcbInit(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaSgCheck(mgmtHandle)
    if rc == clCommonErrors.CL_OK:
        dhaInfoPrint("Not creating SG[%sSG], it already exist", pBaseName)
        dhaCleanUp(mgmtHandle, ccbHandle)
        return None
    
    dhaInfoPrint("Creating 2N SG [%sSG] and other entities(si, csi, su, comp, etc)", pBaseName)

    rc = dhaSgCreate(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaSiCreate(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaCsiCreate(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    for idx in range(0, 2):
        rc = dhaSuCreate(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return None

    for idx in range(0, 2):
        rc = dhaCompCreate(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return None

    rc = dhaCommit(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill SG config")
    rc = dhaSgConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill SI config")
    rc = dhaSiConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill CSI config")
    rc = dhaCsiConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill SU config")
    rc = dhaSuConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill NODE config")
    rc = dhaNodeConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Fill COMP config")
    rc = dhaCompConfigFill(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaInfoPrint("Unlock AMS entities")
    rc = dhaEntitiesUnlock(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaCleanUp(mgmtHandle, ccbHandle)
    return None

_TaskRoutineT = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)
demoCreateRoutine = _TaskRoutineT(clDhaDemoCreate)
def clDhaDemoStart():
    rc = clOsalApi.clOsalTaskCreateDetached(
        "dhaDemoCreate",
        clOsalApi.eClOsalSchedulePolicyT.CL_OSAL_SCHED_OTHER,
        clOsalApi.eClOsalThreadPriorityT.CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
        clOsalApi.CL_OSAL_MIN_STACK_SIZE,
        demoCreateRoutine,
        None
    )
    return rc

def clDhaDemoDelete(arg):
    mgmtHandle = clAmsTypes.ClAmsMgmtHandleT(0)
    ccbHandle = clAmsTypes.ClAmsMgmtCCBHandleT(clHandleApi.CL_HANDLE_INVALID_VALUE)
    pBaseName = BASE_NAME

    version = clCommon.ClVersionT()
    version.releaseCode = ord('B')
    version.majorVersion = 0x1
    version.minorVersion = 0x1

    rc = dhaMgmtInit(mgmtHandle, version)
    if rc != clCommonErrors.CL_OK: return None
    rc = dhaMgmtCcbInit(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaSgCheck(mgmtHandle)
    if rc != clCommonErrors.CL_OK:
        dhaInfoPrint("Not deleting SG[%sSG], it doesn't exist", pBaseName)
        dhaCleanUp(mgmtHandle, ccbHandle)
        return None
    
    dhaInfoPrint("Deleting SG [%sSG] and other entities(si, csi, su, comp, etc)", pBaseName)

    rc = dhaEntitiesLockI(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    for idx in range(0, 2):
        rc = dhaCompDelete(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return None

    for idx in range(0, 2):
        rc = dhaSuDelete(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return None

    rc = dhaCsiDelete(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaSiDelete(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaSgDelete(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    rc = dhaCommit(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return None

    dhaCleanUp(mgmtHandle, ccbHandle)
    return None

demoDeleteRoutine = _TaskRoutineT(clDhaDemoDelete)
def clDhaDemoStop():
    rc = clOsalApi.clOsalTaskCreateDetached(
        "dhaDemoDelete",
        clOsalApi.eClOsalSchedulePolicyT.CL_OSAL_SCHED_OTHER,
        clOsalApi.eClOsalThreadPriorityT.CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
        clOsalApi.CL_OSAL_MIN_STACK_SIZE,
        demoDeleteRoutine,
        None
    )
    return rc



#
#   Dha Operations
#

def dhaInfoPrint(fmtString, *va_args):
    _, fileName, loc = getCallerInfo()

    clLogApi.clLogMsgWrite(
        CL_LOG_HANDLE_APP,
        clLogApi.eClLogSeverityT.CL_LOG_SEV_INFO,
        11,
        "DHA",
        "DMO",
        fileName,
        loc,
        fmtString,
        *va_args
    )

def dhaErrorPrint(fmtString, *va_args):
    _, fileName, loc = getCallerInfo()

    clLogApi.clLogMsgWrite(
        CL_LOG_HANDLE_APP,
        clLogApi.eClLogSeverityT.CL_LOG_SEV_ERROR,
        11,
        "DHA",
        "DMO",
        fileName,
        loc,
        fmtString,
        *va_args
    )

#
# Clean up utils
#

def out(pBaseName, rc):
    if rc == clCommonErrors.CL_OK:
        dhaInfoPrint("Successfully created/deleted 2N SG [%sSG] and its entities(si, csi, su, comp, etc) dynamically", pBaseName)
    else:
        dhaErrorPrint("Error while dynamically creating 2N SG [%sSG] and its entities(si, csi, su, comp, etc)", pBaseName)

def out1(mgmtHandle, pBaseName, rc):
    dhaInfoPrint("Running MGMT finalize")
    retcode = clAmsMgmtClientApi.clAmsMgmtFinalize(mgmtHandle)
    if retcode != clCommonErrors.CL_OK:
        dhaErrorPrint("MGMT finalize returned [%#x]", retcode)
    out(pBaseName, rc)

def out2(mgmtHandle, ccbHandle, pBaseName, rc):
    dhaInfoPrint("Running CCB finalize")
    retcode = clAmsMgmtClientApi.clAmsMgmtCCBFinalize(ccbHandle)
    if retcode != clCommonErrors.CL_OK:
        dhaErrorPrint("CCB finalize returned [%#x]", retcode)
    out1(mgmtHandle, pBaseName, rc)

#
# Setting up
#

def dhaMgmtInit(mgmtHandle, version):
    pBaseName = BASE_NAME
    dhaInfoPrint("Running MGMT initialize")
    rc = clAmsMgmtClientApi.clAmsMgmtInitialize(mgmtHandle, None, version)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("AmsMgmt initialize returned [%#x]", rc)
        out1(mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaMgmtCcbInit(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    dhaInfoPrint("Running MGMT CCB initialize")
    rc = clAmsMgmtClientApi.clAmsMgmtCCBInitialize(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("MGMT CCB initialize returned [%#x]", rc)
        out1(mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCleanUp(mgmtHandle, ccbHandle):
    out2(mgmtHandle, ccbHandle, BASE_NAME, clCommonErrors.CL_OK)

#
# Entity creating functions
#

def dhaSgCheck(mgmtHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()
    pEntityConfig = clAmsEntities.ClAmsSGConfigT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, pEntityConfig)
    
    return rc

def dhaSgCreate(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))
    dhaInfoPrint("Creating SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityCreate(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SG create returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSiCreate(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(entity.name, "{0}SI".format(pBaseName))
    dhaInfoPrint("Creating SI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityCreate(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SI create returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCsiCreate(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI
    clCommon.clNameSet(entity.name, "{0}CSI".format(pBaseName))
    dhaInfoPrint("Creating CSI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityCreate(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CSI create returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSuCreate(mgmtHandle, ccbHandle, idx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(entity.name, "{0}SU{1}".format(pBaseName, idx))
    dhaInfoPrint("Creating SU [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityCreate(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SU create returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCompCreate(mgmtHandle, ccbHandle, idx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP
    clCommon.clNameSet(entity.name, "{0}{1}".format(NEW_COMP_PREFIX, idx))
    dhaInfoPrint("Creating COMP [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityCreate(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("COMP create returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCommit(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME

    dhaInfoPrint("CCB Commit")
    rc = clAmsMgmtClientApi.clAmsMgmtCCBCommit(ccbHandle)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CCB Commit returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

#
# Entity creating functions
#

def dhaCompDelete(mgmtHandle, ccbHandle, idx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP
    clCommon.clNameSet(entity.name, "{0}{1}".format(NEW_COMP_PREFIX, idx))
    dhaInfoPrint("Delete COMP [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityDelete(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("COMP delete returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSuDelete(mgmtHandle, ccbHandle, idx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(entity.name, "{0}SU{1}".format(pBaseName, idx))
    dhaInfoPrint("Delete SU [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityDelete(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SU delte returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCsiDelete(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI
    clCommon.clNameSet(entity.name, "{0}CSI".format(pBaseName))
    dhaInfoPrint("Delete CSI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityDelete(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CSI delete returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSiDelete(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(entity.name, "{0}SI".format(pBaseName))
    dhaInfoPrint("Delete SI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityDelete(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SI delete returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSgDelete(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))
    dhaInfoPrint("Delete SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntityDelete(ccbHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SG delete returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

#
# Config Filling Functions
#

def dhaSgSetSu(mgmtHandle, ccbHandle, sgEntity, suIdx):
    targetEntity = clAmsEntities.ClAmsEntityT()
    targetEntity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(targetEntity.name, "{0}SU{1}".format(BASE_NAME, suIdx))
    dhaInfoPrint("SG set SU [%s]", targetEntity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBSetSGSUList(ccbHandle, sgEntity, targetEntity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SG set SU returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, BASE_NAME, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSgConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()
    targetEntity = clAmsEntities.ClAmsEntityT()

    sgConfig = clAmsEntities.ClAmsSGConfigT()
    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))

    dhaInfoPrint("SG config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, sgConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SG config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Fill SG SI list
    targetEntity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(targetEntity.name, "{0}SI".format(pBaseName))
    dhaInfoPrint("SG set SI [%s]", targetEntity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBSetSGSIList(ccbHandle, entity, targetEntity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SG set SI returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Fill SG SU list
    rc = dhaSgSetSu(mgmtHandle, ccbHandle, entity, 0)
    if rc != clCommonErrors.CL_OK: return rc
    rc = dhaSgSetSu(mgmtHandle, ccbHandle, entity, 1)
    if rc != clCommonErrors.CL_OK: return rc

    # Commit
    return dhaCommit(mgmtHandle, ccbHandle)

def dhaSiConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()
    targetEntity = clAmsEntities.ClAmsEntityT()

    siConfig = clAmsEntities.ClAmsSIConfigT()
    bitMask = clCommon.ClUint64T(0)

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(entity.name, "{0}SI".format(pBaseName))
    dhaInfoPrint("SI config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, siConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SI config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc


    siConfig.numCSIs = 1
    siConfig.numStandbyAssignments = 1

    bitMask.value |= (clAmsMgmtCommon.SI_CONFIG_NUM_CSIS
                      | clAmsMgmtCommon.SI_CONFIG_NUM_STANDBY_ASSIGNMENTS)
    dhaInfoPrint("SI config set [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntitySetConfig(ccbHandle, siConfig.entity, bitMask)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SI config set returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Fill SI CSI list
    targetEntity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI
    clCommon.clNameSet(targetEntity.name, "{0}CSI".format(pBaseName))
    dhaInfoPrint("SI set CSI [%s]", targetEntity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBSetSICSIList(ccbHandle, entity, targetEntity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SI set CSI returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Commit
    return dhaCommit(mgmtHandle, ccbHandle)

def dhaCsiConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    csiConfig = clAmsEntities.ClAmsCSIConfigT()
    bitMask = clCommon.ClUint64T(0)

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI
    clCommon.clNameSet(entity.name, "{0}CSI".format(pBaseName))
    dhaInfoPrint("CSI config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, csiConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CSI config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Set CSI type
    bitMask.value |= clAmsMgmtCommon.CSI_CONFIG_TYPE
    typeName = "{0}Type".format(entity.name.__str__())
    clCommon.clNameSet(csiConfig.type, typeName)
    dhaInfoPrint("CSI type set [%s]", csiConfig.type.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntitySetConfig(ccbHandle, csiConfig.entity, bitMask)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CSI type set returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Set CSI NVP list
    nvp = clAmsEntities.ClAmsCSINameValuePairT()
    clCommon.clNameSet(nvp.paramName, "model")
    clCommon.clNameSet(nvp.paramValue, "twoN")
    clCommon.clNameCopy(nvp.csiName, entity.name)

    dhaInfoPrint("CSI set nvplist")
    rc = clAmsMgmtClientApi.clAmsMgmtCCBCSISetNVP(ccbHandle, entity, nvp)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("CSI set nvplist returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    return dhaCommit(mgmtHandle, ccbHandle)

def dhaNodeSetSu(mgmtHandle, ccbHandle, nodeEntity, suIdx):
    targetEntity = clAmsEntities.ClAmsEntityT()
    targetEntity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(targetEntity.name, "{0}SU{1}".format(BASE_NAME, suIdx))
    dhaInfoPrint("Node set SU [%s]", targetEntity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBSetNodeSUList(ccbHandle, nodeEntity, targetEntity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("Node set SU returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, BASE_NAME, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaNodeConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    nodeConfig = clAmsEntities.ClAmsNodeConfigT()
    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_NODE
    clCommon.clNameSet(entity.name, WORKER0)
    dhaInfoPrint("NODE config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, nodeConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("NODE config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    # Set Node SU list with redundant SUs
    rc = dhaNodeSetSu(mgmtHandle, ccbHandle, entity, 0)
    if rc != clCommonErrors.CL_OK: return rc

    clCommon.clNameSet(entity.name, WORKER1)
    rc = dhaNodeSetSu(mgmtHandle, ccbHandle, entity, 1)
    if rc != clCommonErrors.CL_OK: return rc

    return dhaCommit(mgmtHandle, ccbHandle)

def dhaSuSetComp(mgmtHandle, ccbHandle, suConfig, bitMask, suIdx):
    pBaseName = BASE_NAME
    targetEntity = clAmsEntities.ClAmsEntityT()

    clCommon.clNameSet(suConfig.entity.name, "{0}SU{1}".format(pBaseName, suIdx))

    dhaInfoPrint("SU config set [%s]", suConfig.entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntitySetConfig(ccbHandle, suConfig.entity, bitMask)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SU config set returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    targetEntity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP
    clCommon.clNameSet(targetEntity.name, "{0}{1}".format(NEW_COMP_PREFIX, suIdx))
    dhaInfoPrint("SU [%s] add comp [%s]", suConfig.entity.name.value, targetEntity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBSetSUCompList(ccbHandle, suConfig.entity, targetEntity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SU add comp returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSuConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    suConfig = clAmsEntities.ClAmsSUConfigT()
    bitMask = clCommon.ClUint64T()
    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(entity.name, "{0}SU0".format(pBaseName))
    dhaInfoPrint("SU config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, suConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("SU config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    suConfig.numComponents = 1
    bitMask.value |= clAmsMgmtCommon.SU_CONFIG_NUM_COMPONENTS

    rc = dhaSuSetComp(mgmtHandle, ccbHandle, suConfig, bitMask, 0)
    if rc != clCommonErrors.CL_OK: return rc
    rc = dhaSuSetComp(mgmtHandle, ccbHandle, suConfig, bitMask, 1)
    if rc != clCommonErrors.CL_OK: return rc

    return dhaCommit(mgmtHandle, ccbHandle)

def dhaCompSetConfig(mgmtHandle, ccbHandle, compConfig, bitMask, compIdx):
    pBaseName = BASE_NAME
    clCommon.clNameSet(compConfig.entity.name, "{0}{1}".format(NEW_COMP_PREFIX, compIdx))

    dhaInfoPrint("Comp config set [%s]", compConfig.entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtCCBEntitySetConfig(ccbHandle, compConfig.entity, bitMask)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("Comp config set returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaCompConfigFill(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()
    supportedCSIType = clCommon.ClNameT("")

    compConfig = clAmsEntities.ClAmsCompConfigT()
    bitMask = clCommon.ClUint64T()

    clCommon.clNameSet(entity.name, "{0}0".format(NEW_COMP_PREFIX))
    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP
    dhaInfoPrint("COMP config get [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityGetConfig(mgmtHandle, entity, compConfig)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("COMP config get returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    bitMask.value |= (clAmsMgmtCommon.COMP_CONFIG_CAPABILITY_MODEL
                      | clAmsMgmtCommon.COMP_CONFIG_TIMEOUTS
                      | clAmsMgmtCommon.COMP_CONFIG_RECOVERY_ON_TIMEOUT)

    compConfig.capabilityModel = clAmsTypes.eClAmsCompCapModelT.CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY
    compConfig.timeouts.instantiate = 30000
    compConfig.timeouts.terminate = 30000
    compConfig.timeouts.cleanup = 30000
    compConfig.timeouts.quiescingComplete = 30000
    compConfig.timeouts.csiSet = 30000
    compConfig.timeouts.csiRemove = 30000
    compConfig.timeouts.instantiateDelay = 10000
    compConfig.recoveryOnTimeout = clAmsTypes.eClAmsRecoveryT.CL_AMS_RECOVERY_COMP_FAILOVER

    bitMask.value |= clAmsMgmtCommon.COMP_CONFIG_SUPPORTED_CSI_TYPE

    compConfig.numSupportedCSITypes = 1
    compConfig.pSupportedCSITypes = ctypes.pointer(supportedCSIType)

    clCommon.clNameSet(supportedCSIType, "{0}CSIType".format(pBaseName))

    bitMask.value |= clAmsMgmtCommon.COMP_CONFIG_INSTANTIATE_COMMAND
    compConfig.instantiateCommand = b"dummyComp"

    rc = dhaCompSetConfig(mgmtHandle, ccbHandle, compConfig, bitMask, 0)
    if rc != clCommonErrors.CL_OK: return rc
    rc = dhaCompSetConfig(mgmtHandle, ccbHandle, compConfig, bitMask, 1)
    if rc != clCommonErrors.CL_OK: return rc

    return dhaCommit(mgmtHandle, ccbHandle)

#
# AMF Operations
#

def dhaSuUnlock(mgmtHandle, ccbHandle, suIdx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(entity.name, "{0}SU{1}".format(pBaseName, suIdx))

    dhaInfoPrint("LockA [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockAssignment(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockA returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    dhaInfoPrint("Unlock [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityUnlock(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("Unlock returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    return clCommonErrors.CL_OK

def dhaSiUnlock(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(entity.name, "{0}SI".format(pBaseName))

    dhaInfoPrint("Unlock SI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityUnlock(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("Unlock returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    return clCommonErrors.CL_OK

def dhaSgUnlock(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))

    dhaInfoPrint("LockA SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockAssignment(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockA returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    dhaInfoPrint("Unlock SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityUnlock(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("Unlock returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc

    return clCommonErrors.CL_OK

def dhaEntitiesUnlock(mgmtHandle, ccbHandle):
    for idx in range(0, 2):
        rc = dhaSuUnlock(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return rc

    rc = dhaSiUnlock(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return rc

    rc = dhaSgUnlock(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return rc

    return clCommonErrors.CL_OK

def dhaSuLockI(mgmtHandle, ccbHandle, suIdx):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU
    clCommon.clNameSet(entity.name, "{0}SU{1}".format(pBaseName, suIdx))

    dhaInfoPrint("LockA [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockAssignment(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockA returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    
    dhaInfoPrint("LockI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockInstantiation(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockI returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSiLockA(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI
    clCommon.clNameSet(entity.name, "{0}SI".format(pBaseName))

    dhaInfoPrint("LockA SI [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockAssignment(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockA returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaSgLockI(mgmtHandle, ccbHandle):
    pBaseName = BASE_NAME
    entity = clAmsEntities.ClAmsEntityT()

    entity.type = clAmsEntities.eClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG
    clCommon.clNameSet(entity.name, "{0}SG".format(pBaseName))

    dhaInfoPrint("LockA SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockAssignment(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockA returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    
    dhaInfoPrint("LockI SG [%s]", entity.name.value)
    rc = clAmsMgmtClientApi.clAmsMgmtEntityLockInstantiation(mgmtHandle, entity)
    if rc != clCommonErrors.CL_OK:
        dhaErrorPrint("LockI returned [%#x]", rc)
        out2(ccbHandle, mgmtHandle, pBaseName, rc)
        return rc
    return clCommonErrors.CL_OK

def dhaEntitiesLockI(mgmtHandle, ccbHandle):
    for idx in range(0, 2):
        rc = dhaSuLockI(mgmtHandle, ccbHandle, idx)
        if rc != clCommonErrors.CL_OK: return rc

    rc = dhaSiLockA(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return rc

    rc = dhaSgLockI(mgmtHandle, ccbHandle)
    if rc != clCommonErrors.CL_OK: return rc

    return clCommonErrors.CL_OK
#
# Python entry point
#
if __name__ == "__main__":
    main()