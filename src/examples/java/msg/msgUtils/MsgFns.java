package msgUtils;

import com.sun.jna.Pointer;
import saMsg.SaMsgCallbacksT;
import saAis.SaAisLibrary;
import saAis.SaAisLibrary.SaVersionT;
import saAis.SaAisLibrary.SaNameT;
import saAis.SaAisLibrary.SaAisErrorT;
import saMsg.SaMsgQueueCreationAttributesT;
import java.nio.charset.StandardCharsets;
import saMsg.SaMsgClientLibrary;
import com.sun.jna.ptr.LongByReference;
import saMsg.SaMsgClientLibrary.SaMsgQueueGroupChangesT;
import msgReceiver.ClReceiverMain;

public class MsgFns {
  //private static final String ACTIVE_COMP_QUEUE = "csa104msgqueue";
  public static final int QUEUE_LENGTH = 2048;
  public static final String[] queues = {"test1"};//,"test2", "abcd", "efghigke"};
  public static saMsg.SaMsgClientLibrary saMsgLib = saMsg.SaMsgClientLibrary.INSTANCE;
  private static LongByReference msgLibraryHandle = new LongByReference();
  public static LongByReference[] msgQueueHandle = new LongByReference[queues.length];
  public static int numQueues = 0;
  
  public static int msgInitialize() {
    for (int i = 0;i<msgQueueHandle.length;i++) {
      msgQueueHandle[i] = new LongByReference();
    }
    int rc;
    saMsg.SaMsgCallbacksT msgCallbacks = new saMsg.SaMsgCallbacksT(null,null,null,null);
    msgCallbacks.write();
    saAis.SaAisLibrary.SaVersionT ver = new saAis.SaAisLibrary.SaVersionT();
    ver.releaseCode = 'B';
    ver.majorVersion = 01;
    ver.minorVersion = 01;
    ver.write();
	  rc = saMsgLib.saMsgInitialize (msgLibraryHandle, msgCallbacks, ver.getPointer());
	  if (saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK != rc) {
		  assert false : String.format("Msg init failed [0x%X]", rc);      
    }
    return rc;
  }
  public static int msgOpen(String queueName, int bytesPerPriority) {
    int rc = saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK;
    //byte[] bytes = (queueName + "\0").getBytes(StandardCharsets.UTF_8);
    //byte[] bytes = queueName.getBytes(StandardCharsets.UTF_8);
    //saAis.SaAisLibrary.SaNameT saQueueName = new saAis.SaAisLibrary.SaNameT((short)(bytes.length), bytes);
    //saQueueName.write();
    
    byte[] bytes = queueName.getBytes(StandardCharsets.UTF_8);
    saAis.SaAisLibrary.SaNameT saQueueName = new saAis.SaAisLibrary.SaNameT();
    saQueueName.clear();
    saQueueName.length = (short) bytes.length;
    System.arraycopy(bytes, 0, saQueueName.value, 0, bytes.length);
    saQueueName.write();    
    
    saMsg.SaMsgQueueCreationAttributesT creationAttributes = new saMsg.SaMsgQueueCreationAttributesT();    
    
    int openFlags = saMsg.SaMsgClientLibrary.SA_MSG_QUEUE_CREATE;
    
    creationAttributes.creationFlags = 0;
    int i;
    for (i=0;i<saMsg.SaMsgClientLibrary.SA_MSG_MESSAGE_LOWEST_PRIORITY;i++) {
      creationAttributes.size[i] = bytesPerPriority;
    }
    creationAttributes.retentionTime = 0;
    creationAttributes.write();
    //ClReceiverMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("csa104: Opening msg queue name [%s] length [%d]", saQueueName.toNameString(), saQueueName.length));
    rc = saMsgLib.saMsgQueueOpen(msgLibraryHandle.getValue(), saQueueName, creationAttributes, openFlags, saAis.SaAisLibrary.SA_TIME_END, msgQueueHandle[numQueues]);    
    numQueues++;
    return rc;
  }
  public static int msgSend(String queueName, Pointer buffer, int length) {
    int rc = saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK;
    /*byte[] bytes = (queueName + "\0").getBytes(StandardCharsets.UTF_8);
    saAis.SaAisLibrary.SaNameT saQueueName = new saAis.SaAisLibrary.SaNameT((short)(queueName.length()+1), bytes);
    saQueueName.write();*/
    
    byte[] bytes = queueName.getBytes(StandardCharsets.UTF_8);
    saAis.SaAisLibrary.SaNameT saQueueName = new saAis.SaAisLibrary.SaNameT();
    saQueueName.clear();
    saQueueName.length = (short) bytes.length;
    System.arraycopy(bytes, 0, saQueueName.value, 0, bytes.length);
    saQueueName.write(); 
    
    saMsg.SaMsgMessageT message = new saMsg.SaMsgMessageT();   

    /* Load the SAF message structure */
    message.type = 0;
    saAis.SaAisLibrary.SaVersionT version = new saAis.SaAisLibrary.SaVersionT();
    version.releaseCode = 0;
    version.majorVersion=0;
    version.minorVersion=0;
    version.write();
    message.version = version;
    message.senderName = null;  /* You could put a SaNameT* in here if you wanted to pass a reply queue (for example) */
    message.size = length;
    message.data = buffer;
    message.priority = saMsg.SaMsgClientLibrary.SA_MSG_MESSAGE_HIGHEST_PRIORITY;
    message.write();
    rc = saMsgLib.saMsgMessageSend (msgLibraryHandle.getValue(), saQueueName.getPointer(), message, saAis.SaAisLibrary.SA_TIME_END);
    
    return rc;
  }
  /*public void msgReceiverLoop() {
    
  }*/
}
