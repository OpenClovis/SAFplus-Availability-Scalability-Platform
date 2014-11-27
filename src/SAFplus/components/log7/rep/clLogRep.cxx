#include "clLogRep.hxx"
#include <clMgtRoot.hxx>
#include <clGroup.hxx>
#include <Stream.hxx>
#include <Replicate.hxx>
#include <algorithm>
#include <string>
#include <boost/lexical_cast.hpp>

using namespace SAFplus;

extern HandleStreamMap hsMap;
LogRep SAFplus::logRep;

LogRep::LogRep()
{
  logcfg.serverConfig.maximumRecordsInPacket.value = 5; // This is an temporary assignment to do the replication test. This value (default) should have been read from MGT database
}

void LogRep::logReplicate(SAFplusI::LogBufferEntry* rec, char* msg)
{
  Handle& streamHdl = rec->stream;
  SAFplusLog::Stream* s = hsMap[streamHdl];
  if (s)
  {
      
#if 0 /* Debugging */ 
        char* pMsg = buf+headerLen;        
        for(int i=0;i<lh.numLogs;i++)
        {
          short logLen;
          memcpy(&logLen, pMsg, shortLen);
          pMsg+=shortLen;
          short severity;
          memcpy(&severity, pMsg, shortLen);
          pMsg+=shortLen;
          char log[logLen+1];
          memset(log, 0, logLen+1);
          memcpy(log, pMsg, logLen);                     
          //LogBufferEntry rec;
          //rec.severity = (LogSeverity) severity;
          //rec.stream = lh.streamHandle;
          //postRecord(&rec, log, NULL, false, true);
          printf("\n\n logLen [%d]; severity [%d]; log [%s]\n\n", logLen, severity, log);
          pMsg+=logLen;
        }
#endif
      //------------------------------------------------------------------------        
    ReplicationMessageBuffer& repMsgBuf = s->replicationMessageBuffer;
    int headerLen = sizeof(LogMsgHeader);
    short shortLen = sizeof(short);
    // The replication message buffer has format: |header(LogMsgHeader)|logLen(short)|severity|logMsg|logLen(short)|severity|logMsg|...
    if (s->numLogs == 0) // If this is the first time we pack the replication message buffer, we reserve a space for it. Later on, we only pack its body
    {
      // Here we reserve a space for the replication message buffer, we'll fill it when numLogs reach maximumRecInPackage      
      char reservedLogHeader[headerLen];
      // memset to reservedLogHeader any characters with NULL terminated so that DoublingBuffer calculates right
      // string len of the argument passed to operator +=
      memset(reservedLogHeader, 0, headerLen);
      repMsgBuf.append(reservedLogHeader, headerLen);    
    }    
    short msglen = strlen(msg);    
    repMsgBuf.append(&msglen, shortLen); // append logLen to the replication message buffer
    repMsgBuf.append(&rec->severity, shortLen); // append severity to the replication message buffer
    // Append the msg to replicationMessageBuffer
    repMsgBuf.append(msg, msglen); // append msg to the replication message buffer
    // Increase numLogs after packed one item into the replication message buffer
    s->numLogs++;
    if (s->numLogs == logcfg.serverConfig.maximumRecordsInPacket.value) // numLogs reach maximumRecordsInPacket, so send this buffer to all members of the group belonging to this stream
    {
#if 0
      // Fill the log message header
      LogMsgHeader lh;
      lh.idAndEndian = CLSS_ID;
      lh.version = 0x1;
      lh.extra = 0x0;
      lh.streamHandle = rec->stream;
      lh.numLogs = s->numLogs;
      // bring the log message header back to the beginning of the replication message buffer
      memcpy(repMsgBuf.buf, &lh, headerLen);
      // send the buffer
      s->group.send(repMsgBuf.buf, s->repMsgBufferLen, SAFplus::GroupMessageSendMode::SEND_BROADCAST);
      // reset some variable members of stream
      s->numLogs = 0;
      s->repMsgBufferLen = 0;
#endif
      sendBuffer(s, streamHdl);      
    }
  }
  //else
  //{
  //  printf("Stream obj for handle [%llx.%llx] not found\n", streamHdl.id[0], streamHdl.id[1]);
  //}
}
void LogRep::sendBuffer(SAFplusLog::Stream* s, SAFplus::Handle& streamHdl)
{
  ReplicationMessageBuffer& repMsgBuf = s->replicationMessageBuffer;
  // Fill the log message header
  LogMsgHeader lh;
  lh.idAndEndian = CLSS_ID;
  lh.version = 0x1;
  lh.extra = 0x0;
  lh.streamHandle = streamHdl;
  lh.numLogs = s->numLogs;
  // bring the log message header back to the beginning of the replication message buffer
  memcpy(repMsgBuf.buf, &lh, sizeof(LogMsgHeader));
#if 0
  int headerLen = sizeof(LogMsgHeader);
  int shortLen = sizeof(short);
  char* pMsg = s->replicationMessageBuffer.buf+headerLen;
      for(int i=0;i<lh.numLogs;i++)
      {
        short logLen;
        memcpy(&logLen, pMsg, shortLen);
        pMsg+=shortLen;
        short severity;
        memcpy(&severity, pMsg, shortLen);
        pMsg+=shortLen;
        char log[logLen+1];
        memset(log, 0, logLen+1);
        memcpy(log, pMsg, logLen);                     
          //LogBufferEntry rec;
          //rec.severity = (LogSeverity) severity;
          //rec.stream = lh.streamHandle;
          //postRecord(&rec, log, NULL, false, true);
        printf("\n\n logLen [%d]; severity [%d]; log [%s]\n\n", logLen, severity, log);
        pMsg+=logLen;
      }
#endif
  // send the buffer
  printf("Sending the buffer to all members of group\n");
  s->group.send(repMsgBuf.buf, repMsgBuf.curSize, SAFplus::GroupMessageSendMode::SEND_BROADCAST);
  // reset some variable members of stream
  s->numLogs = 0;  
  repMsgBuf.release();
}
void LogRep::flush(SAFplusLog::Stream* s)
{
  //This function sends the replication message buffer of all streams event if their numLogs haven't reached maximumRecordsInPacket yet because at this time there isn't any record in the shared memory in our log processing
  if (s->replicate != SAFplusLog::Replicate::NONE && s->numLogs > 0) // if this stream is replicatable and the replication message buffer has been packed before
  {
    Handle streamHdl;
    try  // Get the stream handle from the Name service
    {
      streamHdl = name.getHandle(s->getName());
    }
    catch (NameException& e)
    {
      printf("LogRep::flush got exception [%s]\n", e.what());
      return; // No stream handle found for this stream object name, so bypassing it and continue next stream
    }
    sendBuffer(s, streamHdl);
  }
}
