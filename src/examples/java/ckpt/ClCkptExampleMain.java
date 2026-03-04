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
import saCkpt.SaCkptLibrary;
import saCkpt.SaCkptCallbacksT;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import clCkptApi.ClCkptLibrary;

public class ClCkptExampleMain {

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
  private static saCkpt.SaCkptLibrary saCkptLib = saCkpt.SaCkptLibrary.INSTANCE;
  private static clCkptApi.ClCkptLibrary clCkptApiLib = clCkptApi.ClCkptLibrary.INSTANCE;
  
  private static void clprintf(int severity, String fmtString, Object...varArgs)
  {
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
  
  private static long ckptHandle = 0, ckptLibHandle = 0;
  
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
           String secId = "sec11";
           clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Reading data from section [%s] of checkpoint [0x%x]", secId, ckptHandle));
           byte[] data = new byte[256];
           long[] readSize = new long[1];
           int rc = ckptRead(data, readSize, secId);
           String readStr = new String(data, StandardCharsets.UTF_8);
           if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
              clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Data from section [%s] of checkpoint [0x%x], data length [%d]: %s", secId, ckptHandle, readSize[0], readStr));
           }
           else {
              clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Reading data from section [%s] of checkpoint [0x%x] failed rc [0x%x]", secId, ckptHandle, rc));
           }
           
           String dataToWrite = "openclovis; safplus; checkpoint; ckptWrite in csiSetCb";                      
           rc = ckptWrite(dataToWrite, secId);
           if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
             clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("writting data [%s] to section [%s] failed rc [0x%x]", dataToWrite, secId, rc));
           }
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
  
  private static clCkptApi.ClCkptLibrary.ClCkptNotificationCallbackT ckptNotifCb = (ckptHdl, pName, pIOVector, numSections, pCookie) -> {
    try {
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Reading data for checkpoint: [%s]", pName.toNameString()));
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Num sections: [%d]", numSections));
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("SectionId: [%s]", pIOVector.sectionId.id.getString(0)));
      byte[] data = pIOVector.dataBuffer.getByteArray(0,(int)pIOVector.dataSize);
      String readStr = new String(data, StandardCharsets.UTF_8);
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("data: [%s]", readStr));
    }catch (Throwable t) {
      t.printStackTrace(); // avoid letting exceptions escape native callback
    }
    return 0;
  };
  
  private static int ckptLibInit() {
    saAis.SaAisLibrary.SaVersionT ver = new saAis.SaAisLibrary.SaVersionT();
    ver.releaseCode = 'B';
    ver.majorVersion = 01;
    ver.minorVersion = 01;
    ver.write();
    ///LongBuffer buf = ByteBuffer.allocateDirect(8).asLongBuffer();
    LongByReference buf = new LongByReference();
    int rc = saCkptLib.saCkptInitialize(buf, null, ver.getPointer());
    if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
       ckptLibHandle = buf.getValue();
    }
    return rc;
  }
  
  private static int ckptOpen(String name) {
    saCkpt.SaCkptCheckpointCreationAttributesT ckptCreateAttr = new saCkpt.SaCkptCheckpointCreationAttributesT();
    ckptCreateAttr.creationFlags = saCkpt.SaCkptLibrary.SA_CKPT_WR_ALL_REPLICAS | clCkptApi.ClCkptLibrary.CL_CKPT_DISTRIBUTED;
    ckptCreateAttr.checkpointSize = 1024;
    ckptCreateAttr.retentionDuration= 60000000000L;
    ckptCreateAttr.maxSections= 2;
    ckptCreateAttr.maxSectionSize = 700;
    ckptCreateAttr.maxSectionIdSize = 50;
    ckptCreateAttr.write();		   
    int ckptOpenFlags = saCkpt.SaCkptLibrary.SA_CKPT_CHECKPOINT_CREATE|saCkpt.SaCkptLibrary.SA_CKPT_CHECKPOINT_READ|saCkpt.SaCkptLibrary.SA_CKPT_CHECKPOINT_WRITE;		
    byte[] toBytes = name.getBytes(StandardCharsets.UTF_8);
    saAis.SaAisLibrary.SaNameT ckptName = new saAis.SaAisLibrary.SaNameT((short)toBytes.length, toBytes);
    ckptName.write();
    //LongBuffer checkpointHandleBuf = ByteBuffer.allocateDirect(8).asLongBuffer();
    LongByReference hdl = new LongByReference();
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Opening checkpoint [%s]", name));
    int rc = saCkptLib.saCkptCheckpointOpen(ckptLibHandle, ckptName.getPointer(), ckptCreateAttr, ckptOpenFlags, 0, hdl);
    if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      ckptHandle = hdl.getValue();
    }
    return rc;
  }
  
  private static int ckptSectionCreate(String secId) {
    saCkpt.SaCkptSectionCreationAttributesT sectionCreationAttributes = new saCkpt.SaCkptSectionCreationAttributesT();
    //byte[] bytes = Native.toByteArray(secId); // automatically adds null terminator
    byte[] bytes = secId.getBytes(StandardCharsets.UTF_8);
    Pointer p = new Memory(bytes.length);
    p.write(0, bytes, 0, bytes.length);
    saCkpt.SaCkptSectionIdT sectionId = new saCkpt.SaCkptSectionIdT((short)bytes.length, p);
    sectionId.write();
    sectionCreationAttributes.sectionId = sectionId.getPointer(); 
    sectionCreationAttributes.expirationTime = saAis.SaAisLibrary.SA_TIME_END;
    sectionCreationAttributes.write();
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Creating section [%s] of checkpoint [0x%x]", secId, ckptHandle));
    String strInitData = "Default data in the section";
    ByteBuffer initialData = ByteBuffer.allocate(strInitData.length());
    initialData.put(strInitData.getBytes(StandardCharsets.UTF_8));		     
    int rc = saCkptLib.saCkptSectionCreate(ckptHandle, sectionCreationAttributes, initialData, 28);
    return rc;
  }
  
  private static int ckptWrite(String dataToWrite, String secId) {
    //byte[] bytes = Native.toByteArray(secId); // automatically adds null terminator
    byte[] bytes = secId.getBytes(StandardCharsets.UTF_8);
    Pointer p = new Memory(bytes.length);
    p.write(0, bytes, 0, bytes.length);
    saCkpt.SaCkptSectionIdT sectionId = new saCkpt.SaCkptSectionIdT((short)bytes.length, p);
    sectionId.write();
    saCkpt.SaCkptIOVectorElementT writeVector = new saCkpt.SaCkptIOVectorElementT();
    //byte[] data = Native.toByteArray(dataToWrite); // automatically adds null terminator
    byte[] data = dataToWrite.getBytes(StandardCharsets.UTF_8);
    Pointer pData = new Memory(data.length);
    pData.write(0, data, 0, data.length);
    writeVector.sectionId = sectionId;           
    writeVector.dataBuffer = pData;
    writeVector.dataSize = data.length;
    writeVector.dataOffset = 0;
    writeVector.readSize = 0;
    writeVector.write();
    clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Writing data [%s] to section [%s] of checkpoint [0x%x]", pData.getString(0), secId, ckptHandle));             
    IntByReference erroneousVectorIndex = new IntByReference();
    int rc = saCkptLib.saCkptCheckpointWrite(ckptHandle, writeVector, 1, erroneousVectorIndex);
    return rc;
  }
  
  private static int ckptSectionOverwite(String secId, String dataToWrite) {
    //byte[] bytes = Native.toByteArray(secId); // automatically adds null terminator
    byte[] bytes = secId.getBytes(StandardCharsets.UTF_8);
    Pointer p = new Memory(bytes.length);    
    p.write(0, bytes, 0, bytes.length);
    saCkpt.SaCkptSectionIdT sectionId = new saCkpt.SaCkptSectionIdT((short)bytes.length, p);
    sectionId.write();
		
    //byte[] bbytes = Native.toByteArray(dataToWrite); // automatically adds null terminator
    byte[] bbytes = dataToWrite.getBytes(StandardCharsets.UTF_8);
    Pointer pp = new Memory(bbytes.length);
    pp.write(0, bbytes, 0, bbytes.length);
    
    int rc = saCkptLib.saCkptSectionOverwrite(ckptHandle, sectionId, pp, bbytes.length);
    return rc;
  }
  
  private static int ckptRead(byte[] readData, /*Pointer outData,*/ long[] readSize, String secId) {    
    //byte[] bytes = Native.toByteArray(secId); // automatically adds null terminator
    byte[] bytes = secId.getBytes(StandardCharsets.UTF_8);
    Pointer pSecId = new Memory(bytes.length);
    pSecId.write(0, bytes, 0, bytes.length);
    saCkpt.SaCkptSectionIdT sectionId = new saCkpt.SaCkptSectionIdT((short)bytes.length, pSecId);
    sectionId.write();		       
    saCkpt.SaCkptIOVectorElementT readVector = new saCkpt.SaCkptIOVectorElementT();           
    Pointer pData = new Memory(readData.length);          
    readVector.sectionId = sectionId;           
    readVector.dataBuffer = pData;
    readVector.dataSize = readData.length;
    readVector.dataOffset = 0;
    //readVector.readSize = readData.length;
    readVector.write();    
    IntByReference erroneousVectorIndex = new IntByReference();
    int rc = saCkptLib.saCkptCheckpointRead(ckptHandle, readVector, 1, erroneousVectorIndex);
    //outData = pData;
    if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      byte[] bbytes = pData.getByteArray(0,(int)readVector.readSize);
      System.arraycopy(bbytes, 0, readData, 0, (int)readVector.readSize);
    }
    readSize[0] = readVector.readSize;
    return rc;
  }
  
  public static void main(String argv[])
  {
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
     //Begin creating checkpoint and writting data
     rc = ckptLibInit();
     if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
       String name = "ckpt_test_001";
       rc = ckptOpen(name);
       if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
         rc = clCkptApiLib.clCkptImmediateConsumptionRegister(ckptHandle, ckptNotifCb, null);
         if (rc != 0) {
           clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("clCkptImmediateConsumptionRegister for checkpoint [%s] failure [0x%x]", name, rc));
         }
         String secId = "sec11";
         rc = ckptSectionCreate(secId);
         if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
           String dataToWrite = "openclovis; safplus; checkpoint; ckptWrite";
           //String dataToWrite = "openclovis; safplus; checkpoint; secOverwrite";
           rc = ckptWrite(dataToWrite, secId);
           //rc = ckptSectionOverwite(secId, dataToWrite);
           if (rc == saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
             clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Wrote data [%s] to section [%s] of checkpoint [%s] successfully", dataToWrite, secId, name));
           }
           else {
             clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Wrote data [%s] to section [%s] of checkpoint [%s] failure", dataToWrite, secId, name));
           }
         }
         else {
           clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Creating section [%s] of checkpoint [%s] failed with rc [0x%x]", secId, name, rc));
         }
       }
       else {
         clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Ckpt open error [0x%x]", rc));
       } 
     }
     else {
       clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Ckpt lib initialized error [0x%x]", rc));       
     }    
    //End creating checkpoint and write data
    
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
    //LongByReference ckpt_selection_obj = new LongByReference();
    //rc = saCkptLib.saCkptSelectionObjectGet(ckptLibHandle, ckpt_selection_obj);
    //if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
    //clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("saCkptSelectionObjectGet: [0x%x]", rc));
    //}
    do {
        int err = libc.LibC.INSTANCE.select(dispatch_fd.getValue()+1, readfds, null, null, null);
        if (err < 0) {            
            if (libc.Errno.EINTR == Native.getLastError()) {    
                continue;
            }            
            break;
        }       
        saAmfLib.saAmfDispatch(amfHandle.getValue(), saAis.SaAisLibrary.SaDispatchFlagsT.SA_DISPATCH_ALL);
        //saCkptLib.saCkptDispatch(ckptLibHandle, saAis.SaAisLibrary.SaDispatchFlagsT.SA_DISPATCH_ALL);
    }while(!unblockNow); 
    
    /*
     * Do the application specific finalization here.
     */
     
    rc = saCkptLib.saCkptCheckpointClose(ckptHandle);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Closing checkpoint handle [0x%x] failed rc [0x%x]", ckptHandle, rc));
    }
    rc = saCkptLib.saCkptFinalize(ckptLibHandle);
    if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
      clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Closing checkpoint lib handle [0x%x] failed rc [0x%x]", ckptLibHandle, rc));
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
 
}
