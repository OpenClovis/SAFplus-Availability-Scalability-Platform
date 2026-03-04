package msgReceiver;

import msgUtils.MsgFns;
import com.sun.jna.Pointer;
import com.sun.jna.*;
import com.sun.jna.ptr.LongByReference;
import java.nio.charset.StandardCharsets;
import saAis.SaAisLibrary.SaAisErrorT;
import saAis.SaAisLibrary.SaNameT;
import saAis.SaAisLibrary;
import saMsg.SaMsgMessageT;
import clUtils.ClUtilsLibrary;

public class Receiver implements Runnable {
  
  @Override
  public void run() {    
    int rc;
    saAis.SaAisLibrary.SaNameT.ByReference senderName = new saAis.SaAisLibrary.SaNameT.ByReference();
    Pointer data = new Memory(1024);
    LongByReference sendTime = new LongByReference();
    LongByReference senderId = new LongByReference();
    int curQ=0;
    while (true) {
        saMsg.SaMsgMessageT message = new saMsg.SaMsgMessageT();
        message.type = 0;
        saAis.SaAisLibrary.SaVersionT version = new saAis.SaAisLibrary.SaVersionT();
        version.releaseCode = 0;
        version.majorVersion=0;
        version.minorVersion=0;
        version.write();
        message.version = version;
        message.version.write();
        message.size       = 1024;
        message.senderName = senderName.getPointer();
        message.data       = data;
        message.priority = 1;
        message.write();
        rc = msgUtils.MsgFns.saMsgLib.saMsgMessageGet (msgUtils.MsgFns.msgQueueHandle[curQ].getValue(), message, sendTime, senderId, 100);        
        if (saAis.SaAisLibrary.SaAisErrorT.SA_AIS_OK != rc) {
          //clprintf ( CL_LOG_SEV_ERROR, "Msg saMsgMessageGet failed [0x%X]\n\r", rc );
          ClReceiverMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_ERROR, String.format("Msg saMsgMessageGet failed [0x%X]\n\r", rc));
        }
        else {
          senderName.read();          
          if (senderName.length > 0) {
            String sender = new String(senderName.value, StandardCharsets.UTF_8);
            //clprintf ( CL_LOG_SEV_INFO, "Sender Name   : %s\n", sender);
            ClReceiverMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Sender Name   : %s\n", sender));
          }
          byte[] bytes = message.data.getByteArray(0,(int)message.size);
          String msg = new String(bytes, StandardCharsets.UTF_8);
          //clprintf ( CL_LOG_SEV_INFO, "Received Message on [%s]  : %s\n", MsgFns.queues[curQ], msg);
          ClReceiverMain.clprintf(clUtils.ClUtilsLibrary.ClLogSeverityT.CL_LOG_SEV_INFO, String.format("Received Message on [%s]  : %s\n", MsgFns.queues[curQ], msg));
        }

        curQ++;
        if (curQ>=MsgFns.numQueues) {
          curQ=0;
        }
    }
  }  
}

