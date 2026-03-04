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
import clLogApi.ClLogClientLibrary;
import libc.LibC;
import libc.LibC.*;
import libc.Errno;
import libc.Errno.*;
import java.lang.ProcessHandle;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class ClLogExampleMain {

  private static interface CompCfgLibrary extends Library {
    public static final String JNA_LIBRARY_NAME = "clCompCfg";
    public static final CompCfgLibrary INSTANCE = Native.load(JNA_LIBRARY_NAME, CompCfgLibrary.class);
  };	
  
  private static final int CL_LOG_TST_BUFFER = 0; 
  static final int CL_LOG_TST_ASCII = 1;

  private static long mypid = 0;
  private static saAis.SaAisLibrary.SaNameT.ByReference appName = new saAis.SaAisLibrary.SaNameT.ByReference();  
  private static short svcId = 10;
  private static long myLogHandle = 0;
  private static long logSvcHandle = 0;
  //private static long logHandle = 0;

  private static CompCfgLibrary compCfgLib = CompCfgLibrary.INSTANCE;
  private static NativeLibrary mwNativeLib = clUtils.ClUtilsLibrary.MW_NATIVE_LIB;
  private static clUtils.ClUtilsLibrary utilsLib = clUtils.ClUtilsLibrary.UTILS_INSTANCE;
  private static clLogApi.ClLogClientLibrary logClientLib = clLogApi.ClLogClientLibrary.INSTANCE;
  private static NativeLibrary iocLib = NativeLibrary.getInstance("ClIoc");
  
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
            LogWrittingTest logWritting = new LogWrittingTest();
            Thread lwThread = new Thread(logWritting);
            lwThread.start();
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

    rc = saAmfLib.saAmfComponentNameGet(amfHandle.getValue(), appName);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    appName.read();
    rc = saAmfLib.saAmfComponentRegister(amfHandle.getValue(), appName.getPointer(), null);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      errorexit(rc,-1);
    }
    rc = logSvcInit();
    if (rc == 0) {
      clLogApi.ClWaterMarkT waterMark = new clLogApi.ClWaterMarkT();
      waterMark.lowLimit = 90;
      waterMark.highLimit = 99;
      waterMark.write();
      rc = logStreamOpen(logSvcHandle,
                    "test",
                    clUtils.ClUtilsLibrary.ClLogStreamScopeT.CL_LOG_STREAM_LOCAL, 
                    "hungta.log",
                    ".:/home/clovis/log",
                    150000000,
                    300,
                    clUtils.ClUtilsLibrary.ClLogFileFullActionT.CL_LOG_FILE_FULL_ACTION_ROTATE,
                    10,
                    20,
                    20000000,
                    waterMark);
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
    rc = logStreamClose(myLogHandle);
    rc = logSvcFinalize(logSvcHandle);
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
  static int logSvcInit() {
    int       rc      = 0;
    saAis.SaAisLibrary.SaVersionT version = new saAis.SaAisLibrary.SaVersionT();
    version.majorVersion = 1;
    version.majorVersion = 1;
    version.releaseCode = 'B';
    version.write();
    LongByReference logSvcHdl = new LongByReference();
    rc = logClientLib.clLogInitialize(logSvcHdl, null, version.getPointer());
    if (0 == rc ) {
      logSvcHandle = logSvcHdl.getValue();
    }
    return rc;
  }
  static int logStreamOpen(long hLog,
                    String streamName,
                    int streamScope, 
                    String fileName,
                    String fileLocation,
                    int fileSize,
                    int recordSize,
                    int fileAction,
                    int maxFiles,
                    int flushFreq,
                    int flushInterval,
                    clLogApi.ClWaterMarkT waterMark) {
    int                   rc         = 0;
    clLogApi.ClLogStreamAttributesT streamAttr = new clLogApi.ClLogStreamAttributesT();
    streamAttr.fileName = fileName;
    streamAttr.fileLocation = fileLocation;
    streamAttr.fileUnitSize = fileSize;
    streamAttr.recordSize = recordSize;
    streamAttr.haProperty = 0;
    streamAttr.fileFullAction = fileAction;
    streamAttr.maxFilesRotated = maxFiles;
    streamAttr.flushFreq = flushFreq;
    streamAttr.flushInterval = flushInterval;
    streamAttr.waterMark = waterMark;
    streamAttr.write();
    
    /*byte[] bytes = streamName.getBytes(StandardCharsets.UTF_8);
    clUtils.ClUtilsLibrary.ClNameT clStreamName = new clUtils.ClUtilsLibrary.ClNameT((short)bytes.length, bytes);
    clStreamName.write();*/
    
    /*byte[] bytes = EVENT_CHANNEL_NAME.getBytes(StandardCharsets.UTF_8);
    saAis.SaAisLibrary.SaNameT evtChannelName = new saAis.SaAisLibrary.SaNameT((short)bytes.length, bytes);
    evtChannelName.write();*/
    
    byte[] bytes = streamName.getBytes(StandardCharsets.UTF_8);
    clUtils.ClUtilsLibrary.ClNameT.ByValue clStreamName = new clUtils.ClUtilsLibrary.ClNameT.ByValue();
    clStreamName.clear();
    clStreamName.length = (short) bytes.length;
    System.arraycopy(bytes, 0, clStreamName.value, 0, bytes.length);
    clStreamName.write();
    /*clUtils.ClUtilsLibrary.ClNameT.ByReference clStreamName = new clUtils.ClUtilsLibrary.ClNameT.ByReference();
    utilsLib.clNameSet(clStreamName, streamName);
    clStreamName.read();*/
    
    LongByReference hStream = new LongByReference();
    rc = logClientLib.clLogStreamOpen(hLog, clStreamName, streamScope, 
                         streamAttr, clLogApi.ClLogClientLibrary.ClLogStreamOpenFlagsT.CL_LOG_STREAM_CREATE, 0,
                         hStream);
    if (rc == 0) {
      myLogHandle = hStream.getValue();
    }    
    return rc;
  }
  
  static int logStreamClose(long hStream) {
    int  rc = 0;

    rc = logClientLib.clLogStreamClose(hStream);

    return rc;
  }

  static int logSvcFinalize(long hLog) {
    int  rc = 0;

    rc = logClientLib.clLogFinalize(hLog);
    
    return rc;
  }

  static int logWrite(long   hStream, 
               int    writeType,
               int    severity, 
               String logStr) {
    int      rc  = 0;
    //int  num = 32;
    short svcId = 10;
    switch (writeType) {
      case CL_LOG_TST_BUFFER:
        rc = logClientLib.clLogWriteAsync(hStream, severity,
                                 svcId,
                                 (short)clLogApi.ClLogClientLibrary.CL_LOG_MSGID_BUFFER, 
                                 logStr, logStr.length());
        break;
      case CL_LOG_TST_ASCII:
        rc = logClientLib.clLogWriteAsync(hStream, severity,
                                 svcId,
                                 (short)clLogApi.ClLogClientLibrary.CL_LOG_MSGID_PRINTF_FMT, 
                                 logStr, logStr.length());            
        break;
      /*case CL_LOG_TST_TLV:
        rc = utils.clLogWriteAsync(hStream, severity,
                                 10,
                                 2, 
                                 logStr, logStr.length());
                                 
        rc = clLogWriteAsync(pLogTestData->hStream, severity,
                                 CL_LOG_DEFAULT_SERVICE_ID,
                                 *CL_LOG_MSGID_TLV, 
                                 CL_LOG_TLV_STRING(pLogStr),
                                 CL_LOG_TLV_UINT32(num),
                                 CL_LOG_TAG_TERMINATE);
        break;*/
    }
    return rc;
  }
  
  static class LogWrittingTest implements Runnable {
    public void run() {
      while (unblockNow == false) {      
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter dtFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String formattedDate = now.format(dtFormatter);
        logWrite(myLogHandle, CL_LOG_TST_ASCII, clLogApi.ClLogClientLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Logged at: %s", formattedDate));
        try {
          Thread.sleep(1000); // sleep for 1000 ms = 1 second: Keep the event publish rate reasonable for this tutorial
        }catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        }      
      }
    }
  }
}
