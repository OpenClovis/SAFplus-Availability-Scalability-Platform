package msgSender;

import msgUtils.MsgFns;
import com.sun.jna.Pointer;
import com.sun.jna.*;
import java.nio.charset.StandardCharsets;
import clUtils.ClUtilsLibrary;
import saAis.SaAisLibrary;
//import msgReceiver.ClReceiverMain; 

public class Sender implements Runnable {
  //private String[] queues = new String[50];
  //private boolean unblockNow = false;
  private String appName = new String(); 
  public Sender(String appName) {//, boolean unblockNow) {
    //if (queues.length > this.queues.length)
    //  throw new IllegalArgumentException("Wrong array size !");
    //this.queues = queues;
    //this.unblockNow = unblockNow;
    this.appName = appName;
  }
  @Override
  public void run() {
    int count = 0;
    int rc = 0;
    //String msg = new String();
    while (!ClSenderMain.unblockNow) { // TODO: Need to test to see unblockNow is updated
        count++;        
        String msg = String.format("Msg [%4d] from [%s]", count, appName);
        byte[] bytes = msg.getBytes(StandardCharsets.UTF_8);
        Pointer p = new Memory(bytes.length);
        p.write(0, bytes, 0, bytes.length);
        //ClSenderMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("csa104: Sending Message: [%s]",msg));
        //clprintf(CL_LOG_SEV_INFO,"csa104: Sending Message: %s",msg);
        for (int i=0; i<MsgFns.queues.length;i++) {
          ClSenderMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("csa104: Sending Message: [%s] via queue [%s]",msg, MsgFns.queues[i]));
          rc = MsgFns.msgSend(MsgFns.queues[i],p,bytes.length);
          if (rc != saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK) {
            ClSenderMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("csa104: Message [%s] sent to queue [%s] failed [0x%x]",msg, MsgFns.queues[i], rc));
          }
        }        
        MsgFns.msgSend("nonExistentQueue",p,bytes.length);
        try {
          Thread.sleep(2000); // sleep for 2000 ms = 2 second
        }catch (InterruptedException e) {
           Thread.currentThread().interrupt();
        }        
     }
  }  
}

