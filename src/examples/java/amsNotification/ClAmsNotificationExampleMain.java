import com.sun.jna.*;
import com.sun.jna.ptr.*;
import saAmf.SaAmfLibrary;
import saAmf.SaAmfLibrary.*;
//import saAmf.SaAmfLibrary.SaAmfHAStateT.*;
import saAmf.SaAmfCallbacksT;
import saAmf.SaAmfCallbacksT.*;
import saAmf.SaAmfCSIDescriptorT;
import saAis.SaAisLibrary;
import saAis.SaAisLibrary.*;
import clEoLib.ClEoLibrary;
import clEoLib.ClEoLibrary.*;
import clUtils.*;
import clUtils.ClUtilsLibrary;
import clUtils.ClUtilsLibrary.*;
import libc.LibC;
import libc.LibC.*;
import libc.Errno;
import libc.Errno.*;
import java.lang.ProcessHandle;
import clAmsNotificationApi.ClAmsNotificationLibrary;
import clAmsNotificationApi.ClAmsNotificationLibrary.*;

public class ClAmsNotificationExampleMain {

  private static interface CompCfgLibrary extends Library {
    public static final String JNA_LIBRARY_NAME = "clCompCfg";
    public static final CompCfgLibrary INSTANCE = Native.load(JNA_LIBRARY_NAME, CompCfgLibrary.class);
  };	

  private static long mypid = 0;
  private static saAis.SaAisLibrary.SaNameT.ByReference appName = new saAis.SaAisLibrary.SaNameT.ByReference();  
  private static short svcId = 10;

  private static CompCfgLibrary compCfgLib = CompCfgLibrary.INSTANCE;
  private static NativeLibrary mwNativeLib = clUtils.ClUtilsLibrary.MW_NATIVE_LIB;
  private static clUtils.ClUtilsLibrary utilsLib = clUtils.ClUtilsLibrary.UTILS_INSTANCE;
  private static NativeLibrary iocLib = NativeLibrary.getInstance("ClIoc");
  private static ClAmsNotificationLibrary amsNotificationLib = ClAmsNotificationLibrary.INSTANCE;
  
  private static void clprintf(int severity, String fmtString, Object...varArgs) {
    StackTraceElement e = Thread.currentThread().getStackTrace()[2];
    Pointer symPtr = mwNativeLib.getGlobalVariableAddress("CL_LOG_HANDLE_APP");     
    long handleApp = symPtr.getLong(0);   
    utilsLib.clLogMsgWrite( handleApp,
                            severity,
                            svcId,
                            "---",
                            "---",
                            e.getFileName(),
                            e.getLineNumber(),
                            fmtString,
                            varArgs); 
    
  }
  private static saAmf.SaAmfLibrary saAmfLib = SaAmfLibrary.INSTANCE;  
  private static clEoLib.ClEoLibrary eoLib = clEoLib.ClEoLibrary.INSTANCE;
  private static LongByReference amfHandle = new LongByReference();  
  private static saAmf.SaAmfCallbacksT.ByReference callbacks = new saAmf.SaAmfCallbacksT.ByReference();
  private static boolean unblockNow = false; 
  
  private static saAmf.SaAmfLibrary.SaAmfCSISetCallbackT csiSetCb = (invocation, compName, haState, csiDescriptor) -> {    
    /*
     * Take appropriate action based on state
     */
    try {
      csiDescriptor.read();
      compName.read();
      csiDescriptor.csiName.read();
    
      String msg = String.format("Component [%s] : PID [%d]. CSI Set Received\n", Native.toString(compName.value), mypid);             
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, msg);      
      csiDescriptor.csiStateDescriptor.setType(
            (haState == saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_ACTIVE)
                ? saAmf.SaAmfCSIActiveDescriptorT.class
                : saAmf.SaAmfCSIStandbyDescriptorT.class
      );    
      csiDescriptor.csiStateDescriptor.read();
      clCompAppAMFPrintCSI(csiDescriptor, haState);
      switch ( haState ) {
        case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_ACTIVE:
        {
            /*
             * AMF has requested application to take the active HA state 
             * for the CSI.
             */        	
            saAmfLib.saAmfResponse(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
            break;
        }

        case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_STANDBY:
        {
            /*
             * AMF has requested application to take the standby HA state 
             * for this CSI.
             */       	

            saAmfLib.saAmfResponse(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
            break;
        }

        case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_QUIESCED:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active or quiescing HA state. The application 
             * must stop work associated with the CSI immediately.
             */
        	
            saAmfLib.saAmfResponse(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
            break;
        }

        case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_QUIESCING:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active HA state. The application must stop work
             * associated with the CSI gracefully and not accept any new
             * workloads while the work is being terminated.
             */
        	
        	  saAmfLib.saAmfCSIQuiescingComplete(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
            break;
        }

        default:
        {
            assert false : "Invalid haState";
            break;
        }
      }
    }catch (Throwable t) {
      t.printStackTrace(); // don't let exceptions escape native callback
    }
    return;
  };
  private static saAmf.SaAmfLibrary.SaAmfCSIRemoveCallbackT csiRemoveCb = (invocation, compName, csiName, csiFlags) -> {    
    try {
      compName.read();
      csiName.read();
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Component [%s] : PID [%d]. CSI Remove Received\n", Native.toString(compName.value), mypid));
      clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("   CSI                     : %s\n", Native.toString(csiName.value)));
      clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("   CSI Flags               : 0x%d\n", csiFlags));
      
     /*
     * Add application specific logic for removing the work for this CSI.
     */
      
      saAmfLib.saAmfResponse(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
    }catch (Throwable t) {
        t.printStackTrace(); // avoid letting exceptions escape native callback
    }
    return;
  };
  
  private static saAmf.SaAmfLibrary.SaAmfComponentTerminateCallbackT termCb = (invocation, compName) -> {
    try {
      int rc = saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK;    
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Component [%s] : PID [%d]. Terminating\n", Native.toString(compName.value), mypid));    
      /*
       * Unregister with AMF and respond to AMF saying whether the
       * termination was successful or not.
       */    
      compName.read();
      rc = saAmfLib.saAmfComponentUnregister(amfHandle.getValue(), compName.getPointer(), null);
      if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
        saAmfLib.saAmfResponse(amfHandle.getValue(), invocation, saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK);
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Component [%s] : PID [%d]. Terminated\n", Native.toString(compName.value), mypid)); 
        unblockNow = true;
      }
      else {
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Component [%s] : PID [%d]. Termination error [0x%x]\n", Native.toString(compName.value), mypid, rc));
      }
    }catch (Throwable t) {
        t.printStackTrace();
    }
    return;
  };
   
  private static void errorexit(int rc, int status) {    
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Component [%s] : PID [%d]. Initialization error [0x%x]\n", Native.toString(appName.value), mypid, rc));
    System.exit(status);
  }
  
  public static void main(String argv[]) {

    mypid = ProcessHandle.current().pid();
    
    saAis.SaAisLibrary.SaVersionT.ByReference version = new saAis.SaAisLibrary.SaVersionT.ByReference();
    version.releaseCode = 'B';
    version.majorVersion = 01;
    version.minorVersion = 01;
    version.write();
    
    callbacks.saAmfHealthcheckCallback = null;
    callbacks.saAmfComponentTerminateCallback = termCb;
    callbacks.saAmfCSISetCallback = csiSetCb;
    callbacks.saAmfCSIRemoveCallback = csiRemoveCb;
    callbacks.saAmfProtectionGroupTrackCallback = null;    
    callbacks.saAmfProxiedComponentInstantiateCallback = null;
    callbacks.saAmfProxiedComponentCleanupCallback = null;
    callbacks.write(); // sync struct
    int rc = saAmfLib.saAmfInitialize(amfHandle, callbacks, version);    
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    FDSet readfds = new FDSet();
    readfds.FD_ZERO();
    LongByReference dispatch_fd = new LongByReference();
    rc = saAmfLib.saAmfSelectionObjectGet(amfHandle.getValue(), dispatch_fd);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    readfds.FD_SET(dispatch_fd.getValue());
    
    /*
     * Do the application specific initialization here.
     */
    rc = amsNotificationLib.clAmsClientNotificationInitialize(amsNotificationCallback);
	  if (rc != SaAisErrorT.SA_AIS_OK) {
	    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, String.format("clAmsClientNotificationInitialize fail rc [0x%x]", rc));
	  }

    rc = saAmfLib.saAmfComponentNameGet(amfHandle.getValue(), appName);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    appName.read();
    rc = saAmfLib.saAmfComponentRegister(amfHandle.getValue(), appName.getPointer(), null);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    
    /*
     * Print out standard information for this component.
     */
    
    IntByReference iocPort = new IntByReference();
    eoLib.clEoMyEoIocPortGet(iocPort);   
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Component [%s] : PID [%d]. Initializing\n", Native.toString(appName.value), mypid));
    Function iocLocalAddrGetFunc = iocLib.getFunction("clIocLocalAddressGet");
    int myIocAddr = iocLocalAddrGetFunc.invokeInt(new Object[]{});  
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("   IOC Address             : 0x%x\n", myIocAddr));
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("   IOC Port                : 0x%x\n",iocPort.getValue()));    
    
    /*
     * Block on AMF dispatch file descriptor for callbacks
     */
    
    do {
        int err = libc.LibC.INSTANCE.select(dispatch_fd.getValue()+1, readfds, null, null, null);
        if (err < 0) {            
            if (libc.Errno.EINTR == Native.getLastError()) {
                continue;
            }            
            break;
        }       
        saAmfLib.saAmfDispatch(amfHandle.getValue(), saAis.SaAisLibrary.SaDispatchFlagsT.SA_DISPATCH_ALL);        
    }while(!unblockNow); 
    
    /*
     * Do the application specific finalization here.
     */
    amsNotificationLib.clAmsClientNotificationFinalize();
    
    rc = saAmfLib.saAmfFinalize(amfHandle.getValue());
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
  }
  
  private static String haStateToString(int haState) {    
    switch (haState) {
      case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_ACTIVE: 
        return "Active";
      case saAmf.SaAmfLibrary.SaAmfHaStateT. SA_AMF_HA_STANDBY:
         return "Standby";
      case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_QUIESCED:
         return "Quiesced";
      case saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_QUIESCING:
        return "Quiescing";
      default:
        return "Unknown";
    }
  }
  
  private static String csiFlagsToString(int csiFlags) {
    if ((csiFlags & saAmf.SaAmfLibrary.SA_AMF_CSI_ADD_ONE) != 0)
      return "Add One";
    if ((csiFlags & saAmf.SaAmfLibrary.SA_AMF_CSI_TARGET_ONE) != 0)
      return "Target One";
    if ((csiFlags & saAmf.SaAmfLibrary.SA_AMF_CSI_TARGET_ALL) != 0)
      return "Target All";
    return "Unknown";
  }
  
  /******************************************************************************
  * Utility functions 
  *****************************************************************************/

  /*
   * clCompAppAMFPrintCSI
   * --------------------
   * Print information received in a CSI set request.
   */
  
  private static void clCompAppAMFPrintCSI(saAmf.SaAmfCSIDescriptorT csiDescriptor,
                          int haState) {    
    clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,
              "CSI Flags : [%s]",
              csiFlagsToString(csiDescriptor.csiFlags));

    if (saAmf.SaAmfLibrary.SA_AMF_CSI_TARGET_ALL != csiDescriptor.csiFlags) {
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("CSI Name : [%s]", 
                  Native.toString(csiDescriptor.csiName.value)));
    }

    if (saAmf.SaAmfLibrary.SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags) {
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, "Name value pairs :");
        int count = csiDescriptor.csiAttr.number;
        Pointer ptr = csiDescriptor.csiAttr.attr;
        if (ptr != null && count > 0) {
            saAmf.SaAmfCSIAttributeT[] attrs = (saAmf.SaAmfCSIAttributeT[]) new saAmf.SaAmfCSIAttributeT(ptr).toArray(count);
            for (int i = 0; i < count; i++) {
               attrs[i].read();               
               clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Name :[%s]",attrs[i].getName()));                      
               clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Value : [%s]", attrs[i].getValue()));
            }
        }        
     }
    
    clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("HA state : [%s]",
              haStateToString(haState)));

    if (saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_ACTIVE == haState) {
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, "Active Descriptor :");        
        saAmf.SaAmfCSIActiveDescriptorT act = csiDescriptor.csiStateDescriptor.activeDescriptor;
        act.read();
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,
                  String.format("Transition Descriptor : [%d]",
                  act.transitionDescriptor));
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,
                  String.format("Active Component : [%s]",
                  Native.toString(act.activeCompName.value)));
    }
    else if (saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_STANDBY == haState) {
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, "Standby Descriptor :");
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,
                  String.format("Standby Rank : [%d]",
                  csiDescriptor.csiStateDescriptor.
                  standbyDescriptor.standbyRank));
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Active Component : [%s]",
                  Native.toString(csiDescriptor.csiStateDescriptor.
                  standbyDescriptor.activeCompName.value)));
    }
  }
  
  /*
  * Insert any other utility functions here.
  */
  
  static ClAmsClientNotificationCallbackT amsNotificationCallback = (notification) -> {        
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, "Enter amsNotificationCallback");
    int rc = 0;
    notification.read();
    notification.amsNotificationInfo.read();
    notification.amsNotificationInfo.amsNodeInfo.read();
    notification.amsNotificationInfo.amsNodeInfo.nodeName.read();
    notification.amsNotificationInfo.amsStateInfo.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Nodename: [%s]", notification.amsNotificationInfo.amsNodeInfo.nodeName.toNameString()));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("notifType: [%s]", getAmsNotificationString(notification.type)));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Node operType: [%d]", notification.amsNotificationInfo.amsNodeInfo.operation));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("stateInfo:type [%s]", getAmsNotificationString(notification.amsNotificationInfo.amsStateInfo.type)));
    notification.amsNotificationInfo.amsStateInfo.entityName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("stateInfo:entityName [%s]", notification.amsNotificationInfo.amsStateInfo.entityName.toNameString()));
    notification.amsNotificationInfo.amsStateInfo.siName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("stateInfo:siName [%s]", notification.amsNotificationInfo.amsStateInfo.siName.toNameString()));
    notification.amsNotificationInfo.amsStateInfo.suName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("stateInfo:suName [%s]", notification.amsNotificationInfo.amsStateInfo.suName.toNameString()));
    notification.amsNotificationInfo.amsCompInfo.read();
    notification.amsNotificationInfo.amsCompInfo.compName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("compInfo:compName [%s]", notification.amsNotificationInfo.amsCompInfo.compName.toNameString()));
    notification.amsNotificationInfo.amsCompInfo.nodeName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("compInfo:nodeName [%s]", notification.amsNotificationInfo.amsCompInfo.nodeName.toNameString()));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("compInfo:nodeIocAddr [%d]", notification.amsNotificationInfo.amsCompInfo.nodeIocAddress));
    notification.amsNotificationInfo.amsNodeInfo.read();
    notification.amsNotificationInfo.amsNodeInfo.nodeName.read();
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("nodeInfo:nodeName [%s]", notification.amsNotificationInfo.amsNodeInfo.nodeName.toNameString()));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, String.format("nodeInfo:nodeIocAddr [%d]", notification.amsNotificationInfo.amsNodeInfo.nodeIocAddress));
    clprintf (ClLogSeverityT.CL_LOG_SEV_INFO, "Leave amsNotificationCallback");
    return rc;
  };
  
  static String getAmsNotificationString(int evt) {
	  return evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NONE?"CL_AMS_NOTIFICATION_NONE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_FAULT?"CL_AMS_NOTIFICATION_FAULT":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SU_INSTANTIATION_FAILURE?"CL_AMS_NOTIFICATION_SU_INSTANTIATION_FAILURE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE?"CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED?"CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED?"CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_SI_UNASSIGNED?"CL_AMS_NOTIFICATION_SI_UNASSIGNED":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_COMP_ARRIVAL?"CL_AMS_NOTIFICATION_COMP_ARRIVAL":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_COMP_DEPARTURE?"CL_AMS_NOTIFICATION_COMP_DEPARTURE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NODE_ARRIVAL?"CL_AMS_NOTIFICATION_NODE_ARRIVAL":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NODE_DEPARTURE?"CL_AMS_NOTIFICATION_NODE_DEPARTURE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_ENTITY_CREATE?"CL_AMS_NOTIFICATION_ENTITY_CREATE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_ENTITY_DELETE?"CL_AMS_NOTIFICATION_ENTITY_DELETE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_OPER_STATE_CHANGE?"CL_AMS_NOTIFICATION_OPER_STATE_CHANGE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_ADMIN_STATE_CHANGE?"CL_AMS_NOTIFICATION_ADMIN_STATE_CHANGE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NODE_SWITCHOVER?"CL_AMS_NOTIFICATION_NODE_SWITCHOVER":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_NODE_FAILOVER?"CL_AMS_NOTIFICATION_NODE_FAILOVER":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_COMP_HA_STATE_CHANGE?"CL_AMS_NOTIFICATION_COMP_HA_STATE_CHANGE":
	         evt == ClAmsNotificationTypeT.CL_AMS_NOTIFICATION_MAX?"CL_AMS_NOTIFICATION_MAX":"Unknown";	
  }
}
