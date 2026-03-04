import com.sun.jna.*;
import com.sun.jna.NativeLong;
import com.sun.jna.ptr.*;
import saAmf.SaAmfLibrary;
import saAmf.SaAmfLibrary.*;
import saAmf.SaAmfCallbacksT;
import saAmf.SaAmfCallbacksT.*;
import saAmf.SaAmfCSIDescriptorT;
import saAis.SaAisLibrary;
import saAis.SaAisLibrary.*;
import clEoLib.ClEoLibrary;
import clEoLib.ClEoLibrary.*;
import clUtils.*;
import clUtils.ClUtilsLibrary;
import libc.LibC;
import libc.LibC.*;
import libc.Errno;
import libc.Errno.*;
import java.lang.ProcessHandle;
import java.util.logging.*;
import java.nio.charset.StandardCharsets;
import saMsg.SaMsgClientLibrary;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import saEvt.SaEventClientLibrary;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.io.FileInputStream;
import java.io.IOException;

public class ClEventExampleMain {

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
  private static saEvt.SaEventClientLibrary saEvtLib = saEvt.SaEventClientLibrary.INSTANCE;
    
  public static void clprintf(int severity, String fmtString, Object...varArgs) {
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
  
  private final static String EVENT_CHANNEL_NAME = "csa11[23]TestEvents";
  private final static String PUBLISHER_NAME = "csa11[23]_Publisher";
  //private final int EVENT_TYPE = 5432;
  private static long evtChannelHandle = 0;
  private static long evtLibHandle = 0;
  private static long eventHandle = 0; 
  
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
           Thread publishTread = new Thread(new PublishThread());
           publishTread.start();
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
   
  private static saEvt.SaEventClientLibrary.SaEvtEventDeliverCallbackT appEventCallback = (subscriptionId, eventHandle, eventDataSize) -> {
    try {
      int rc = saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK;      
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,"We've got an event to receive\n");

      /* A high performance implementation would keep the buffer
       if it was big enough for the next event, OR even faster
       preallocate buffer(s) of the the maximum event size which
       can be known by controlling what processes publish to
       a particular event channel.

       This tutorial will simply free an realloc the event buffer.
      */
       Pointer resTest = new Memory(eventDataSize);

      /* This API can be used outside of the callback routine, which is why
       you need to pass the size of the buffer you've allocated. */
      LongByReference evtDataSize = new LongByReference();
      evtDataSize.setValue(eventDataSize);
      rc = saEvtLib.saEvtEventDataGet(eventHandle, resTest, evtDataSize);
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,"Failed to get event data [0x%x]",rc);
        return;
      }     
      byte[] data = resTest.getByteArray(0,(int)evtDataSize.getValue());      
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,String.format("received event: %s\n", Native.toString(data)));
    } catch (Throwable t) {
        t.printStackTrace();
    }
    return;
  };
   
  private static void errorexit(int rc, int status) {    
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Component [%s] : PID [%d]. Initialization error [0x%x]\n", Native.toString(appName.value), mypid, rc));
    System.exit(status);
  }

  static class PublishThread implements Runnable {    
    public void run() {
      while (unblockNow == false) {
        /* csa113: If I am active then I'll publish an event.
           Note, any application can publish and event regardless of
           HA state.  This tutorial simply uses HA state so only
           one of the two processes are publishing.
        */     
        csa113Comp_PublishEvent();
        try {
          Thread.sleep(1000); // sleep for 1000 ms = 1 second: Keep the event publish rate reasonable for this tutorial
        }catch (InterruptedException e) {
           Thread.currentThread().interrupt();
        }
     }
   }
 };

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
    /* csa113: initialize the event client library, open the event channel and subscribe to events */
    if (true) {
      saEvt.SaEvtCallbacksT evtCallbacks = new saEvt.SaEvtCallbacksT(null, appEventCallback);
      evtCallbacks.write();
      saAis.SaAisLibrary.SaVersionT  evtVersion = new saAis.SaAisLibrary.SaVersionT();
      evtVersion.releaseCode = 'B';
      evtVersion.majorVersion = 0x1;
      evtVersion.minorVersion = 0x1;
      evtVersion.write();
      LongByReference evtLibHdl = new LongByReference();
      rc = saEvtLib.saEvtInitialize(evtLibHdl, evtCallbacks, evtVersion.getPointer());
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed to init event mechanism [0x%x]\n", rc));
        errorexit(rc,-1);
      }
      evtLibHandle = evtLibHdl.getValue();
      byte[] bytes = EVENT_CHANNEL_NAME.getBytes(StandardCharsets.UTF_8);
      saAis.SaAisLibrary.SaNameT evtChannelName = new saAis.SaAisLibrary.SaNameT((short)bytes.length, bytes);
      evtChannelName.write();        
      // Open an event chanel so that we can subscribe to events on that channel
      LongByReference evtChannelHdl = new LongByReference();
      rc = saEvtLib.saEvtChannelOpen(evtLibHandle, evtChannelName, (saEvt.SaEventClientLibrary.SA_EVT_CHANNEL_PUBLISHER | saEvt.SaEventClientLibrary.SA_EVT_CHANNEL_SUBSCRIBER | saEvt.SaEventClientLibrary.SA_EVT_CHANNEL_CREATE), saAis.SaAisLibrary.SA_TIME_END, evtChannelHdl);
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter dtFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        String formattedDate = now.format(dtFormatter);
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failure opening event channel [0x%x] at %ld", rc, formattedDate));          
        errorexit(rc,-1);
      }
      evtChannelHandle = evtChannelHdl.getValue();
      rc = saEvtLib.saEvtEventSubscribe(evtChannelHandle, null, 1);
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {        
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed to subscribe to event channel [0x%x]",rc));
        errorexit(rc,-1);          
      }
    }    
    /*csa113: create an event definition to be published */
    if (true) {     
      byte[] bytes = PUBLISHER_NAME.getBytes(StandardCharsets.UTF_8);
      saAis.SaAisLibrary.SaNameT publisherName = new saAis.SaAisLibrary.SaNameT((short)bytes.length, bytes);
      publisherName.write();
      LongByReference evtHdl = new LongByReference();
      rc = saEvtLib.saEvtEventAllocate(evtChannelHandle, evtHdl);
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {        
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed to allocate event [0x%x]\n",rc));
        assert false : "Failed to allocate event";
      }
      eventHandle = evtHdl.getValue();
      rc = saEvtLib.saEvtEventAttributesSet(eventHandle, null, 1, 0, publisherName);
      if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {        
        clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed to set event attributes [0x%x]\n",rc));
        assert false : "Failed to set event attributes";
      }      
    }
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
    
    /* csa113: close the event channel, finalize the event client library */
    rc = saEvtLib.saEvtChannelClose(evtChannelHandle);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK)
    {
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed [0x%x] to close event channel",rc));
    }
    rc = saEvtLib.saEvtFinalize(evtLibHandle);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK)
    {
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Failed [0x%x] to finalize event library",rc));        
    }
    
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

    if (saAmf.SaAmfLibrary.SA_AMF_CSI_TARGET_ALL != csiDescriptor.csiFlags)
    {
        clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("CSI Name : [%s]", 
                  Native.toString(csiDescriptor.csiName.value)));
    }

    if (saAmf.SaAmfLibrary.SA_AMF_CSI_ADD_ONE == csiDescriptor.csiFlags)
    {
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

    if (saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_ACTIVE == haState)
    {
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
    else if (saAmf.SaAmfLibrary.SaAmfHaStateT.SA_AMF_HA_STANDBY == haState)
    {
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
  static int index = 0;
  static int csa113Comp_PublishEvent() {    
    int rc = saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK;
    //
    // Note: to add a new generator, just define it above and then include
    // the new functions name in the generators list.
    // Next, maybe something that gets disk free info by way of getfsent
    // and statfs?
    Generator[] generators = {ClEventExampleMain::generate_time_of_day, ClEventExampleMain::generate_load_average};    
    //
    // every time through increment index and then set index to
    // it's value modulo the number of entries in the generators
    // array.  This will cause us to cycle through the list of
    // generators as we're called to publish events.
    byte[] data = execute(generators[index++]);
    index %= generators.length;
    if ((data != null && data.length == 0) || (data == null))
    {
      clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR,"No event data generated.");
      return -1;
    }
    clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,String.format("Publishing Event: %s\n",Native.toString(data)));
    Pointer p = new Memory(data.length);
    p.write(0, data, 0, data.length);
    LongByReference eventId = new LongByReference();
    rc = saEvtLib.saEvtEventPublish(eventHandle, p, data.length, eventId);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO,String.format("Event publish attempt failed with error [%x]", rc));        
    }
    return 0;
  }
  
  @FunctionalInterface
  interface Generator {
    public byte[] generate();
  };
  
  static byte[] generate_time_of_day()
  {    
    LocalDateTime now = LocalDateTime.now();
    DateTimeFormatter dtFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
    String formattedDate = now.format(dtFormatter);
    byte[] data = formattedDate.getBytes(StandardCharsets.UTF_8);       
    return data;
  }
  
  static byte[] generate_load_average()
  {   
    // Now open the load average file in /proc, read the file into a local
    // buffer, allocate memory to hold the file contents, copy the contents
    // of the file into the newly allocated buffer.
    byte[] buf = new byte[500];
    int num_read = 0;
    String filePath = "/proc/loadavg";    
    try {
      FileInputStream fis = new FileInputStream(filePath);
      num_read = fis.read(buf);
      fis.close();
    }catch(IOException e) {
      e.printStackTrace();
    }    
    if (num_read == 0 || num_read == -1)
    {
      clprintf (clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR,"Bogus result from read of loadavg\n");      
      return null;
    }   
    byte[] data = new byte[num_read];
    System.arraycopy(buf,0,data,0,num_read);
    String tempStr = Native.toString(buf);
    // Do MINIMAL parsing in that we look for the third space in the buffer
    // (which comes after the load average information proper) and we replace
    // the space with a nul character to terminate the string.
    // If there is no third space character, just return the buffer unchanged.
    int idx = tempStr.indexOf(' ');
    if (idx == 0 || idx == -1) {
      return data;
    }
    idx++;
    if (tempStr.charAt(idx) != ' ') {
      return data;
    }
    idx++;
    if (tempStr.charAt(idx) != ' ') {
      return data;
    }
    byte[] tempData = new byte[idx];
    System.arraycopy(data,0,tempData,0,idx);
    return tempData;    
  }
 
  public static byte[] execute(Generator gen) {
    return gen.generate();
  }
 
}
