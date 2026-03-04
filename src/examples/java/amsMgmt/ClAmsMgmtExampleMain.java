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
//import clUtils.ClUtilsLibrary.ClLogSeverityT.*;
import clAmsMgmtClientApi.ClAmsMgmtLibrary;
import clAmsMgmtClientApi.ClAmsMgmtLibrary.*;
import clAmsMgmtClientApi.ClAmsEntityT;
import libc.LibC;
import libc.LibC.*;
import libc.Errno;
import libc.Errno.*;
import java.lang.ProcessHandle;
import java.nio.charset.StandardCharsets;
import java.util.function.BooleanSupplier;
import java.util.function.IntFunction;
import java.util.function.IntSupplier;
import java.util.Arrays;

public class ClAmsMgmtExampleMain {

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
  private static ClAmsMgmtLibrary amsMgmtLib = ClAmsMgmtLibrary.INSTANCE;
  
  private static final String WORKER0 = "WorkerI0";
  private static final String WORKER1 = "WorkerI1";
  private static final String NEW_COMP_PREFIX = "dynamicComp";
  private static final String BASE_NAME = "dynamicTwoN";
  
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
            clDhaDemoCreateStart();
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
        	  clDhaDemoDeleteStart();
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
        	  clDhaDemoDeleteStart();
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
  static class DhaDemoCreate implements Runnable {
    public void run() {
      if (!unblockNow) {
        clDhaDemoCreate();
      }
    }
  };
  
  static void clDhaDemoCreateStart() {
    DhaDemoCreate dhaDemoCreate = new DhaDemoCreate();
    Thread dhaDemoCreateThread = new Thread(dhaDemoCreate);
    dhaDemoCreateThread.start();
  }
  
  static class DhaDemoDelete implements Runnable {
    public void run() {
      if (!unblockNow) {
        clDhaDemoDelete();
      }
    }
  };
  
  static void clDhaDemoDeleteStart() {
    DhaDemoDelete dhaDemoDelete = new DhaDemoDelete();
    Thread dhaDemoDeleteThread = new Thread(dhaDemoDelete);
    dhaDemoDeleteThread.start();
  }
  
  static int clDhaDemoCreate() {
    int  rc = 0;
    int retCode = 0;
    long mgmtHandle = 0;    
    ClVersionT version = new ClVersionT();
    version.releaseCode = 'B';
    version.majorVersion = 1;
    version.minorVersion = 1;
    version.write();
    long ccbHandle = 0;
    String pBaseName = BASE_NAME;
    clAmsMgmtClientApi.ClAmsEntityT entity = new clAmsMgmtClientApi.ClAmsEntityT();
    PointerByReference pEntityConfig = new PointerByReference();

    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "Running MGMT initialize");
    LongByReference mgmtHdl = new LongByReference();
    try {
    rc = amsMgmtLib.clAmsMgmtInitialize(mgmtHdl, null, version.getPointer());
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR, "AmsMgmt initialize returned [%#x]", rc);
      return rc;
    }
    mgmtHandle = mgmtHdl.getValue();
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Running MGMT CCB initialize");
    LongByReference ccbHdl = new LongByReference();
    rc = amsMgmtLib.clAmsMgmtCCBInitialize(mgmtHandle, ccbHdl);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR, "MGMT CCB initialize returned [%#x]", rc);
      //rc = amsMgmtLib.clAmsMgmtFinalize(mgmtHandle);
      return rc;
    }
    ccbHandle = ccbHdl.getValue();
    /* 
     * Create the entities only if they are not created already
     */
    String temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, "%sSG", BASE_NAME, entity);
    
    rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                  entity,
                                  pEntityConfig);
    
    utilsLib.clHeapFree(pEntityConfig.getValue());

    if (rc == 0) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "Not creating SG[%sSG], it already exist", BASE_NAME);        
        return rc;
    }

    /* SG doesn't exist, create SG and other entities dynamically*/
    
    /*First create the service hierarchies*/

    clprintf(ClLogSeverityT.CL_LOG_SEV_NOTICE,
             "Creating 2N SG [%sSG] and other entities(si, csi, su, comp, etc)",
             BASE_NAME);

    /* Create SG*/
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG,"%sSG", BASE_NAME, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating SG [%s]", temp);
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG [%s] create returned [%#x]",temp,rc);     
      return rc;
    }
    /* Create SI*/    
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, "%sSI", BASE_NAME, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating SI [%s]", temp);
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI [%s] create returned [%#x]",temp,rc);      
      return rc;
    }

    /* Create CSI*/
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI, "%sCSI", BASE_NAME, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating CSI [%s]", temp);
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI [%s] create returned [%#x]",temp,rc);      
      return rc;
    }

    /*Create 2 SUs*/    
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU,"%sSU0", BASE_NAME, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating SU [%s]", temp);
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU [%s] create returned [%#x]",temp,rc);      
      return rc;
    }
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU1", BASE_NAME,entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating SU [%s]", temp);
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU [%s] create returned [%#x]",temp,rc);      
      return rc;
    }    
  
    /*Create COMP*/
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP, "%s0", NEW_COMP_PREFIX, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating comp [%s]", Native.toString(entity.name.value));
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Comp [%s] create returned [%#x]",temp,rc);      
      return rc;
    }
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP, "%s1", NEW_COMP_PREFIX, entity);
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Creating comp [%s]", Native.toString(entity.name.value));
    
    rc = amsMgmtLib.clAmsMgmtCCBEntityCreate(ccbHandle, entity);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Comp [%s] create returned [%#x]",temp,rc);      
      return rc;
    }
    
    /* CCB Commit*/
    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"CCB Commit Create");
    rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
    if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CCB Commit returned [%#x]", rc);      
      return rc;
    }
    
    
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill SG config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, pBaseName);
   if (rc != 0) {
      clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG config fill returned [%#x]", rc);      
      return rc;
    }
   
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill SI config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, pBaseName);
   if (rc != 0) {
     clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI config fill returned [%#x]", rc);      
   }
   
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill CSI config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI, pBaseName);
   if (rc != 0) {
     clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI config fill returned [%#x]", rc);    
     return rc;
   }
   
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill SU config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, pBaseName);
   if (rc != 0) {
     clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU config fill returned [%#x]", rc);      
     return rc;
   }
   
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill NODE config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_NODE, pBaseName);
   if (rc != 0) {
     clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"NODE config fill returned [%#x]", rc);      
     return rc;
   }
   
   clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Fill COMP config");
   rc = clAmsMgmtTestFillConfig(mgmtHandle, ccbHandle, ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP, pBaseName);
   if (rc != 0) {
     clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"COMP config fill returned [%#x]", rc);
     return rc;
   }
   
   /*rc = clDhaExmpExec(
        "Unlock " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityUnlock(mgmtHandle, entity),
        err -> String.format("Unlock returned [%#x]", err));*/
   final long CCB_HANDLE = ccbHandle;
   final long MGMT_HANDLE = mgmtHandle;   
   rc = clDhaExmpExec(
                  "Unlock AMS entities",
                  () -> clAmsMgmtTestUnlock(MGMT_HANDLE, CCB_HANDLE, BASE_NAME),
                  err -> String.format("Unlock AMS entities returned [%#x]", err)); 
    }
    finally {
      final long CCB_HANDLE = ccbHandle;
      if (ccbHandle != 0) {
         clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Running CCB finalize");
         retCode = amsMgmtLib.clAmsMgmtCCBFinalize(ccbHandle);
         if (retCode != 0) {
           clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CCB finalize returned [%#x]", retCode);
         }
         
      }
      if (mgmtHandle != 0) {
         clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Running MGMT finalize");
         retCode = amsMgmtLib.clAmsMgmtFinalize(mgmtHandle);
         if (retCode != 0) {
           clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"MGMT finalize returned [%#x]", retCode);
         }
      }
      if (rc == 0) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_NOTICE,"Successfully created 2N SG [%sSG] and its entities(si, csi, su, comp, etc) dynamically",
                 pBaseName);
      }else {
        clprintf(ClLogSeverityT.CL_LOG_SEV_CRITICAL, "Error while dynamically creating 2N SG [%sSG] and its entities(si, csi, su, comp, etc)",
                 pBaseName);
      }
    }
    return rc;
  }
  
  static int clDhaDemoDelete() {
    int  rc = 0;
    int retCode = 0;    
    long mgmtHandle = 0;
    long ccbHandle = 0;
    Pointer pointer = null;
    ClVersionT version = new ClVersionT();
    version.releaseCode = 'B';
    version.majorVersion = 1;
    version.minorVersion = 1;
    version.write();
    String pBaseName = BASE_NAME;    
    clAmsMgmtClientApi.ClAmsEntityT entity = new clAmsMgmtClientApi.ClAmsEntityT();
    //clAmsMgmtClientApi.ClAmsEntityConfigT = new pEntityConfig = NULL;
    PointerByReference pEntityConfig = new PointerByReference();
    try {
      clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "Running MGMT initialize");    
      LongByReference mgmtHdl = new LongByReference();
      rc = amsMgmtLib.clAmsMgmtInitialize(mgmtHdl, null, version.getPointer());
      if(rc != 0) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR, "AmsMgmt initialize returned [%#x]", rc);
        return rc;
      }
      mgmtHandle = mgmtHdl.getValue();
      final long MGMT_HANDLE = mgmtHandle;
      LongByReference ccbHdl = new LongByReference();
      rc = clDhaExmpExec("Running MGMT CCB initialize",
                  () -> amsMgmtLib.clAmsMgmtCCBInitialize(MGMT_HANDLE, ccbHdl),
                  err -> String.format("MGMT CCB initialize returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }
      ccbHandle = ccbHdl.getValue();
      final long CCB_HANDLE = mgmtHandle;
     /* 
      * Delete the entities only if they exist
      */
      String temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, "%sSG", pBaseName,entity);
      rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                  entity,
                                  pEntityConfig);
      pointer = pEntityConfig.getValue();                              
      utilsLib.clHeapFree(pointer);
      if(rc != 0)
      {
        clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "Not deleting SG[%sSG],  SG config get returned [%#x]", pBaseName, rc);
        return rc;
      }

      /* SG exist, Delete SG and other entities dynamically*/

      clprintf(ClLogSeverityT.CL_LOG_SEV_NOTICE,
             "Deleting SG [%sSG] and other entities(si, csi, su, comp, etc)",
             pBaseName);

      /*First move entities to lock instantiated mode*/    
      rc = clDhaExmpExec("LockI AMS entities",
                  () -> clAmsMgmtTestLockI(MGMT_HANDLE, CCB_HANDLE, BASE_NAME),
                  err -> String.format("Unlock AMS entities returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }

      /*Delete 2 COMPs*/
      temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP,"%s0", NEW_COMP_PREFIX,entity);
      rc = clDhaExmpExec("Deleting COMP " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("COMP delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }      
      temp = fillName(NEW_COMP_PREFIX, "%s1", entity.name);
      rc = clDhaExmpExec("Deleting COMP " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("COMP delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }      

      /*Delete 2 SUs*/
      temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU0", NEW_COMP_PREFIX,entity);
      rc = clDhaExmpExec("Deleting SU " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("SU delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }      
      temp = fillName(pBaseName, "%sSU1", entity.name);
      rc = clDhaExmpExec("Deleting SU " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("SU delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }     

      /* Delete CSI*/
      temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI, "%sCSI", pBaseName,entity);
      rc = clDhaExmpExec("Deleting CSI " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("CSI delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }   

      /* Delete SI*/
      temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, "%sSI", pBaseName,entity);
      rc = clDhaExmpExec("Delete SI " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("SI delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }      

      /* Delete SG*/
      temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, "%sSG", pBaseName,entity);
      rc = clDhaExmpExec("Deleting SG " + temp,
                  () -> amsMgmtLib.clAmsMgmtCCBEntityDelete(CCB_HANDLE, entity),
                  err -> String.format("SG delete returned [%#x]", err));
      if (rc != 0) {
        return rc;
      }

      /* CCB Commit*/
      rc = clDhaExmpExec("CCB Commit Delete",
                  () -> amsMgmtLib.clAmsMgmtCCBCommit(CCB_HANDLE),
                  err -> String.format("CCB commit returned [%#x]", err));
                  
    }
    finally {
      if (ccbHandle != 0) {
         clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Running CCB finalize");
         retCode = amsMgmtLib.clAmsMgmtCCBFinalize(ccbHandle);
         if (retCode != 0) {
           clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CCB finalize returned [%#x]", retCode);
         }         
      }
      if (mgmtHandle != 0) {
         clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Running MGMT finalize");
         retCode = amsMgmtLib.clAmsMgmtFinalize(mgmtHandle);
         if (retCode != 0) {
          clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"MGMT finalize returned [%#x]", retCode);
         }
      }
      if (rc == 0) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_NOTICE,"Successfully deleted dynamically created SG [%sSG] and other entities(si, csi, su, comp, etc)",
                 pBaseName);
      }else {
        clprintf(ClLogSeverityT.CL_LOG_SEV_CRITICAL, "Error while deleting dynamically created SG [%sSG] and other entities(si, csi, su, comp, etc)",
                 pBaseName);
      }
      if (pointer != null) {
        utilsLib.clHeapFree(pointer);
      }
    }
    
    return rc;

  }
  
  /*static boolean clDhaExmpExec(String execInfo,
                    BooleanSupplier predicate,
                    String errorInfo) {

    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,execInfo);

    if (!predicate.getAsBoolean()) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR, errorInfo);
        return false;
    }
    return true;
  }*/
  static int clDhaExmpExec(
        String execInfo,
        IntSupplier action,
        IntFunction<String> errorInfo) {

    clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,execInfo);
    int rc = action.getAsInt();

    if (rc != 0) {
        clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR, errorInfo.apply(rc));
    }
    return rc;
  }
  
  static int clAmsMgmtTestUnlock(long mgmtHandle,
                                 long ccbHandle,
                                 String pBaseName) {
    int rc = 0;
    clAmsMgmtClientApi.ClAmsEntityT entity = new clAmsMgmtClientApi.ClAmsEntityT(); 

    /*
     * Step 1 - Unlock SUs
     */
    String temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU0", pBaseName, entity); 
    
    /*if (!clDhaExmpExec("LockA "+ entity.name.value,
                  () -> (rc = amsMgmtLib.clAmsMgmtEntityLockAssignment(mgmtHandle,
                                                      entity)) == 0,
                  String.format("LockA returned [%#x]", rc))) {
          
      return rc;
    }*/
    rc = clDhaExmpExec(
        "LockA " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(mgmtHandle, entity),
        err -> String.format("LockA returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }
    
    rc = clDhaExmpExec(
        "Unlock " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityUnlock(mgmtHandle, entity),
        err -> String.format("Unlock returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }
    
    temp = fillName(pBaseName, "%sSU1", entity.name);
    rc = clDhaExmpExec(
        "LockA " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(mgmtHandle, entity),
        err -> String.format("LockA returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }
    
    rc = clDhaExmpExec(
        "Unlock " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityUnlock(mgmtHandle, entity),
        err -> String.format("Unlock returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }
    
    /*
     * Step 2 - Unlock SI
     */
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, "%sSI", pBaseName, entity); 
    rc = clDhaExmpExec(
        "Unlock SI " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityUnlock(mgmtHandle, entity),
        err -> String.format("Unlock SI returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }   
    
    /*
     * Step 3 - Unlock SG
     */
    temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, "%sSG", pBaseName, entity); 
    rc = clDhaExmpExec(
        "LockA SG " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(mgmtHandle, entity),
        err -> String.format("LockA SG returned [%#x]", err));

    if (rc != 0) {
      return rc;
    }

    rc = clDhaExmpExec(
        "Unlock SG " + temp,
        () -> amsMgmtLib.clAmsMgmtEntityUnlock(mgmtHandle, entity),
        err -> String.format("Unlock SG returned [%#x]", err));
        
    return rc;
  }  
  
  static int clAmsMgmtTestLockI(long mgmtHandle,
                                long ccbHandle,
                                String pBaseName) {
    int rc = 0;
    clAmsMgmtClientApi.ClAmsEntityT entity = new clAmsMgmtClientApi.ClAmsEntityT();

    /*
     * Step 1 - Lock, Lock_I SUs
     */
    String temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU0", pBaseName,entity);
    rc = clDhaExmpExec("LockA " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(ccbHandle, entity),
                  err -> String.format("LockA returned [%#x]", err));
    if (rc != 0) {
      return rc;
    }
    rc = clDhaExmpExec("LockI " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockInstantiation(ccbHandle, entity),
                  err -> String.format("LockI returned [%#x]", err));
    if (rc != 0) {
      return rc;
    }
    
    temp = fillName(pBaseName, "%sSU1", entity.name);
    rc = clDhaExmpExec("LockA " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(ccbHandle, entity),
                  err -> String.format("LockA returned [%#x]", err));
    if (rc != 0) {
      return rc;
    }  
    rc = clDhaExmpExec("LockI " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockInstantiation(ccbHandle, entity),
                  err -> String.format("LockI returned [%#x]", err));
    if (rc != 0) {
      return rc;
    }
    /*
     * Step 2 - Lock SI
     */
   temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, "%sSI", pBaseName,entity);
   rc = clDhaExmpExec("LockA SI " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(ccbHandle, entity),
                  err -> String.format("LockA returned [%#x]", err));
   if (rc != 0) {
     return rc;
   }  

    /*
     * Step 3 - Lock, Lock_I SG
     */

   temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG, "%sSG", pBaseName,entity);
    rc = clDhaExmpExec("LockA SG " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockAssignment(ccbHandle, entity),
                  err -> String.format("LockA returned [%#x]", err));
    if (rc != 0) {
      return rc;
    }
    rc = clDhaExmpExec("LockI SG " + temp,
                  () -> amsMgmtLib.clAmsMgmtEntityLockInstantiation(ccbHandle,entity),
                  err -> String.format("LockI returned [%#x]", err));    

    return rc;
}

  
  static String fillEntity(int type, String fmt, String pBaseName, clAmsMgmtClientApi.ClAmsEntityT entity) {
    entity.type = type;
    String temp = String.format(fmt, pBaseName);
    byte[] bytes = temp.getBytes(StandardCharsets.UTF_8);
    //entity.name.value = bytes;
    Arrays.fill(entity.name.value, (byte)0);
    entity.name.length = (short)bytes.length;
    System.arraycopy(bytes, 0, entity.name.value, 0, entity.name.length); 
    entity.name.write();
    entity.write();
    return temp;
  }
  
  static void fillName(String name, ClNameT entName) {
    byte[] bytes = name.getBytes(StandardCharsets.UTF_8);
    //entName.value = bytes;
    Arrays.fill(entName.value, (byte)0);
    //entName.clear();
    entName.length = (short) bytes.length;
    System.arraycopy(bytes, 0, entName.value, 0, entName.length);    
    entName.write();
  }
  
  static String fillName(byte[] name, String fmt, ClNameT entName) {
    String temp = Native.toString(name);
    temp = fillName(temp, fmt, entName);
    return temp;
  }
  
  static String fillName(String name, String fmt, ClNameT entName) {    
    String temp = String.format(fmt,name);
    byte[] bytes = temp.getBytes(StandardCharsets.UTF_8);
    //entName.value = bytes;
    Arrays.fill(entName.value, (byte)0);
    //entName.clear();
    entName.length = (short) bytes.length;
    System.arraycopy(bytes, 0, entName.value, 0, entName.length); 
    entName.write();
    return temp;
  }
 
  static int clAmsMgmtTestFillConfig(long mgmtHandle,
                                     long ccbHandle,
                                     int type,
                                     String pBaseName) {
    int rc = 0;
    clAmsMgmtClientApi.ClAmsEntityT entity = new clAmsMgmtClientApi.ClAmsEntityT();
    clAmsMgmtClientApi.ClAmsEntityT targetEntity = new clAmsMgmtClientApi.ClAmsEntityT();
    //clAmsMgmtClientApi.ClAmsEntityConfigT pEntityConfig = new clAmsMgmtClientApi.ClAmsEntityConfigT();
    PointerByReference pEntityConfig = new PointerByReference();
    Pointer pointer = null;
    entity.type = type;
    switch (type) {
        case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SG:{
            /* 
             * By default AMS populates the SG config with 2N values
             */
            //clAmsMgmtClientApi.ClAmsSGConfigT sgConfig = new clAmsMgmtClientApi.ClAmsSGConfigT();
            String temp = fillName(pBaseName, "%sSG", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SG config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                          entity,
                                                          pEntityConfig);
            if (rc != 0) {
                clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG config get returned [%#x]", rc);
                break;
            }
            pointer = pEntityConfig.getValue();
            //utilsLib.clHeapFree(pointer);
            
            /*
             * Fill SG SI list
             */             
            temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI, "%sSI", pBaseName, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SG set SI [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetSGSIList(ccbHandle,
                                                       entity,
                                                       targetEntity);
            if (rc != 0) {
                clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG set SI returned [%#x]", rc);
                break;
            }
            
            /*
             * Fill SU list
             */
            temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU0", pBaseName, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SG set SU [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetSGSUList(ccbHandle,
                                                       entity,
                                                       targetEntity);
            if (rc != 0) {
                clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG set SU returned [%#x]", rc);
                break;
            }            
            temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU,"%sSU1", pBaseName, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SG set SU [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetSGSUList(ccbHandle,
                                                       entity,
                                                       targetEntity);
            if (rc != 0) {
                clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG set SU returned [%#x]", rc);
                break;
            }
            
            /*
             * Commit SG settings.
             */
           clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SG set commit");
           rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
           if (rc != 0) {
               clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SG commit returned [%#x]", rc);
               break;
           }
        }
        break;

        case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SI:{
            
            long bitMask = 0;
            String temp = fillName(pBaseName, "%sSI", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SI config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                         entity,
                                                         pEntityConfig);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI config get returned [%#x]", rc);
              break;
            }
            //memcpy(&siConfig, pEntityConfig, sizeof(siConfig));            
            pointer = pEntityConfig.getValue();
            clAmsMgmtClientApi.ClAmsSIConfigT siConfig = new clAmsMgmtClientApi.ClAmsSIConfigT(pointer);
            siConfig.read();
            //utilsLib.clHeapFree(pEntityConfig.getValue());

            siConfig.numCSIs = 1;
            siConfig.numStandbyAssignments = 1;
            siConfig.write();
            
            /*
             *We actually dont even have to fetch coz we can set
             *using the bitmask. But useful for code cov.
             */
            bitMask |= ClAmsMgmtLibrary.SI_CONFIG_NUM_CSIS | ClAmsMgmtLibrary.SI_CONFIG_NUM_STANDBY_ASSIGNMENTS;
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SI config set [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                            siConfig.entity,
                                                            bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI config set returned [%#x]", rc);
              break;
            }
            
            /*
             * Fill SI CSI list
             */
            temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI,"%sCSI", pBaseName, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SI set CSI [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetSICSIList(ccbHandle,
                                                    entity,
                                                    targetEntity);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI set CSI returned [%#x]", rc);
              break;
            }

            /*
             * Commit to AMS db
             */
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SI set commit");
            rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SI commit returned [%#x]", rc);
            }
        }
        break;
      case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_CSI:{
            //ClAmsCSIConfigT csiConfig = {{0}};
            long bitMask = 0;
            String temp = fillName(pBaseName,"%sCSI", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "CSI config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                    entity,
                                                    pEntityConfig);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI config get returned [%#x]", rc);
              break;
            }
            //memcpy(&csiConfig, pEntityConfig, sizeof(csiConfig));
            pointer = pEntityConfig.getValue();
            clAmsMgmtClientApi.ClAmsCSIConfigT csiConfig = new clAmsMgmtClientApi.ClAmsCSIConfigT(pointer);
            csiConfig.read();
            /*
             * Set CSI type
             */
            bitMask |= ClAmsMgmtLibrary.CSI_CONFIG_TYPE;
            csiConfig.type = new ClNameT();
            String temp2 = Native.toString(entity.name.value);
            temp = String.format("%sType", temp2);
            byte[] bytes = temp.getBytes(StandardCharsets.UTF_8);
            csiConfig.type.value = bytes;
            csiConfig.type.length = (short)bytes.length;
            csiConfig.write();
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO, "CSI type set [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                            csiConfig.entity,
                                                            bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI type set returned [%#x]", rc);
              break;
            }

            /*
             * Set CSI NVP list 
             */
            clAmsMgmtClientApi.ClAmsCSINameValuePairT nvp = new clAmsMgmtClientApi.ClAmsCSINameValuePairT();
            nvp.paramName = new ClNameT();
            fillName("model", nvp.paramName);
            nvp.paramValue = new ClNameT();
            fillName("twoN", nvp.paramValue);
            nvp.csiName = new ClNameT(entity.name.getPointer());
            nvp.write();
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"CSI set nvplist");
            rc = amsMgmtLib.clAmsMgmtCCBCSISetNVP(ccbHandle,
                                                      entity,
                                                      nvp);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI set nvplist returned [%#x]", rc);
              break;
            }

            /*
             * Commit CSI to AMS db
             */
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"CSI ccb commit");
            rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"CSI ccb commit returned [%#x]", rc);
              break;
            }
        }
        break;
      case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_NODE:{
            //ClAmsNodeConfigT nodeConfig = {{0}};
            String temp = fillName(WORKER1, "%s", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"NODE config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                         entity,
                                                         pEntityConfig);
            if (rc!=0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"NODE config get returned [%#x]", rc);
              break;
            }
            //memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
            pointer = pEntityConfig.getValue();
            //clAmsMgmtClientApi.ClAmsNodeConfigT nodeConfig = new clAmsMgmtClientApi.ClAmsNodeConfigT(pEntityConfig.getValue());
            utilsLib.clHeapFree(pointer);
    
            /*
             * Set Node SU list with redundant SUs
             */
            temp = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU, "%sSU1", pBaseName, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Node set SU [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetNodeSUList(ccbHandle,
                                                     entity,
                                                     targetEntity);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Node set SU returned [%#x]", rc);
              break;
            }
            temp = fillName(WORKER0, "%s", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"NODE config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                         entity,
                                                       pEntityConfig);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"NODE config get returned [%#x]", rc);
              break;
            }                                
                          
            //memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
            //ClAmsNodeConfigT nodeConfig
            pointer = pEntityConfig.getValue();
            //utilsLib.clHeapFree();
    
            temp = fillName(pBaseName, "%sSU0", targetEntity.name);
            
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Node set SU [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBSetNodeSUList(ccbHandle,
                                                          entity,
                                                          targetEntity);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Node set SU returned [%#x]", rc);
              break;
            }                          
            /*
             * Commit node to AMS db.
             */
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Node set commit");
            rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Node commit returned [%#x]", rc);              
            }
        }
        break;
      case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_SU:{
            //ClAmsSUConfigT suConfig = {{0}};
            long bitMask = 0;
            String temp = fillName(pBaseName, "%sSU0", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                        entity,
                                                        pEntityConfig);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU config get returned [%#x]", rc);
              break;
            }       
                          
            //memcpy(&suConfig, pEntityConfig, sizeof(suConfig));
            pointer = pEntityConfig.getValue();
            clAmsMgmtClientApi.ClAmsSUConfigT suConfig = new clAmsMgmtClientApi.ClAmsSUConfigT(pointer); 
            //utilsLib.clHeapFree(pEntityConfig.getValue());
            suConfig.read();
            /*
             * Set num components.
             */
            suConfig.numComponents = 1;
            bitMask |= ClAmsMgmtLibrary.SU_CONFIG_NUM_COMPONENTS;
            suConfig.write();
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU config set [%s]",temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                            suConfig.entity,
                                                            bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU config set returned [%#x]", rc);
              break;
            }                                               
            temp = fillName(pBaseName,"%sSU1", suConfig.entity.name);            
            suConfig.numComponents = 1;
            suConfig.write();
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU config set [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                            suConfig.entity,
                                                            bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU config set returned [%#x]", rc);
              break;
            }
            /*
             * Set SU comp list.
             */
            //To be continued...
            String temp2 = fillEntity(ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP, "%s0", NEW_COMP_PREFIX, targetEntity);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU [%s] add comp [%s]", Native.toString(entity.name.value), Native.toString(targetEntity.name.value));
            rc = amsMgmtLib.clAmsMgmtCCBSetSUCompList(ccbHandle,
                                                     entity,
                                                     targetEntity);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU add comp returned [%#x]", rc);
              break;
            }                                          
            
            temp = fillName(NEW_COMP_PREFIX, "%s1", targetEntity.name);            
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU [%s] add comp [%s]", Native.toString(suConfig.entity.name.value),
                           Native.toString(targetEntity.name.value));
            rc = amsMgmtLib.clAmsMgmtCCBSetSUCompList(ccbHandle,
                                                          suConfig.entity,
                                                          targetEntity);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU add comp returned [%#x]", rc);
              break;
            }
                          
            /* 
             * Commit to AMS db. 
             */
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"SU set commit");
            rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"SU commit returned [%#x]", rc);
            }                          
        }
        break;
      case ClAmsEntityTypeT.CL_AMS_ENTITY_TYPE_COMP:{
            //ClAmsCompConfigT compConfig = {{0}};
            long bitMask = 0;
            //ClNameT supportedCSIType = { 0 };
            String temp = fillName(NEW_COMP_PREFIX, "%s0", entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"COMP config get [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtEntityGetConfig(mgmtHandle,
                                                        entity,
                                                        pEntityConfig);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"COMP config get returned [%#x]", rc);
              break;
            }
                          
            //memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
            pointer = pEntityConfig.getValue();
            clAmsMgmtClientApi.ClAmsCompConfigT compConfig = new clAmsMgmtClientApi.ClAmsCompConfigT(pointer);
            //utilsLib.clHeapFree(pEntityConfig.getValue());
            compConfig.read();
            bitMask |= ClAmsMgmtLibrary.COMP_CONFIG_CAPABILITY_MODEL | ClAmsMgmtLibrary.COMP_CONFIG_TIMEOUTS | ClAmsMgmtLibrary.COMP_CONFIG_RECOVERY_ON_TIMEOUT;
            compConfig.capabilityModel = ClAmsCompCapModelT.CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY;
            compConfig.timeouts.instantiate = 30000;
            compConfig.timeouts.terminate = 30000;
            compConfig.timeouts.cleanup = 30000;
            compConfig.timeouts.quiescingComplete = 30000;
            compConfig.timeouts.csiSet = 30000;
            compConfig.timeouts.csiRemove = 30000;
            compConfig.timeouts.instantiateDelay = 10000;
            compConfig.recoveryOnTimeout = ClAmsRecoveryT.CL_AMS_RECOVERY_COMP_FAILOVER;
            
            bitMask |= ClAmsMgmtLibrary.COMP_CONFIG_SUPPORTED_CSI_TYPE;
            if (compConfig.pSupportedCSITypes != null)
                utilsLib.clHeapFree(compConfig.pSupportedCSITypes);
            
            ClNameT supportedCSIType = new ClNameT();
            fillName(pBaseName, "%sCSIType", supportedCSIType);
            compConfig.numSupportedCSITypes = 1;
            compConfig.pSupportedCSITypes = supportedCSIType.getPointer();           
            
            /*
             * invoke this process with dummy as the arg.
             */
            bitMask |= ClAmsMgmtLibrary.COMP_CONFIG_INSTANTIATE_COMMAND;
            compConfig.instantiateCommand = "dummyComp".getBytes(StandardCharsets.UTF_8);
            compConfig.write();
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Comp config set [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                            compConfig.entity,
                                                            bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Comp config set returned [%#x]", rc);
              break;
            }                                                
            
            /*
             *Set similar config to second comp.
             */
            temp = fillName(NEW_COMP_PREFIX, "%s1", compConfig.entity.name);
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Comp config set [%s]", temp);
            rc = amsMgmtLib.clAmsMgmtCCBEntitySetConfig(ccbHandle,
                                                       compConfig.entity,
                                                       bitMask);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Comp config set returned [%#x]", rc);
              break;
            }                                           
            
            /* 
             * Commit to AMS db.
             */
            clprintf(ClLogSeverityT.CL_LOG_SEV_INFO,"Comp set commit");
            rc = amsMgmtLib.clAmsMgmtCCBCommit(ccbHandle);
            if (rc != 0) {
              clprintf(ClLogSeverityT.CL_LOG_SEV_ERROR,"Comp commit returned [%#x]", rc);              
            }                          
        }
        break;
    }
    if (pointer != null) {
      utilsLib.clHeapFree(pointer);
    }
    return rc;
  }
 
}
