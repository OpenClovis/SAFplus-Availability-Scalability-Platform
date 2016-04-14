/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.
 */

#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>
#include <MsgReplyHandler.hxx>
#include <clFaultApi.hxx>


namespace SAFplus
{
    SafplusMsgServer safplusMsgServer;
  int msgServerInitCount=0;

  void msgServerFinalize()
  {
    if (msgServerInitCount > 0)
      {
        msgServerInitCount--;
        if (msgServerInitCount==0) safplusMsgServer.Stop();
      }
  }

    void msgServerInitialize(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads)
      {
        if (msgServerInitCount > 0) return;
        if (port == 0)
          {
          //? <cfg name="SAFPLUS_RECOMMENDED_MSG_PORT">Specifies the network port to use for SAFplus backplane communication to your process (via the safplusMsgServer object.  Typically, this environment variable is set by the SAFplus AMF when it starts your process, but you may need to set it if the AMF is not starting your process.  This is the RECOMMENDED port. Your process may choose to use another port (typically done if you want to use "well-known" ports to identify services) by setting the port field in the SafplusInitializationConfiguration object.</cfg>  
          char* temp = getenv("SAFPLUS_RECOMMENDED_MSG_PORT");
          if (temp)
            {
              port = atoi(temp);           
            }
          
          }
        if (port == 0)
          {
            throw Error(Error::SAFPLUS_ERROR, Error::MISCONFIGURATION, "Invalid messaging port [0]", __FILE__, __LINE__);
          }
      safplusMsgServer.init(port,maxPendingMsgs, maxHandlerThreads);
      safplusMsgServer.Start();
      msgServerInitCount++;  // Do this last in case exception throw (like port in use)
      }

    void SAFplus::SafplusMsgServer::init(ClWordT _port, ClWordT maxPendingMsgs, ClWordT maxHandlerThreads, Options flags)
      {
        MsgServer::Init(_port, maxPendingMsgs, maxHandlerThreads, flags);
        MsgHandler *replyHandler = new MsgReplyHandler();
        this->RegisterHandler(SAFplusI::CL_IOC_SAF_MSG_REPLY_PROTO, replyHandler, &msgReply);
        SAFplus::iocPort = port; // Set this global to be used as a unique identifier for this component across the node.  There can be many MsgServers per component but only one SafplusMsgServer.
        assert(SAFplus::faultAvailable());  // Fault must be initialized before the message server so we can learn if nodes die
        fault = new Fault();
        fault->init(handle);
      }

    void SAFplus::SafplusMsgServer::registerHandler(ClWordT type, MsgHandler *handler, ClPtrT cookie)
    {
        SAFplus::MsgServer::RegisterHandler(type, handler, cookie);
    }

    void SAFplus::SafplusMsgServer::removeHandler(ClWordT type)
    {
        SAFplus::MsgServer::RemoveHandler(type);
    }

  MsgReply *SafplusMsgServer::sendReply(Handle destination, void* buffer, ClWordT length, ClWordT msgtype, Wakeable *wakeable,uint retryDuration)
    {
        memset(&msgReply, 0, sizeof(MsgReply));
        /*
         * Send message
         */
        ScopedLock<> lockIt(msgSendReplyMutex);  // It is necessary to lock here so that the send and the wait happens atomically relative to the receipt of the response
        try
        {
            SendMsg(destination, buffer, length, msgtype);
        }
        catch (...)
        {
            return NULL;
        }
        /**
         * Sending Sync type, need to start listen on replying to wake
         */
        if (wakeable == NULL)
        {
            /**
             * Wait on condition
             */
            while (msgReply.len < 2)
            {
              if (!msgSendConditionMutex.timed_wait(msgSendReplyMutex, retryDuration)) // TODO: create customizable timeout
                  {
                    Handle h = destination;
                    
                    for (int i=0;i<3;i++)  // Loop around 3 times checking this handle, then node/process, and finally just the node
                      {
                        FaultState fs = fault->getFaultState(h);
                        if (fs==FaultState::STATE_UP)
                          {
                            fault->notifyNoResponse(h);  // Tell the fault manager that I'm having a problem.
                            SendMsg(destination, buffer, length, msgtype);  // retry
                            break;  // found what the fault manager is tracking so I'm done here
                          }
                        else if (fs==FaultState::STATE_UNDEFINED)
                          {
                            // Look at fault state of containing entity?
                            if (i==0) h = getProcessHandle(destination.getProcess(), destination.getNode());                      
                            if (i==1) h = getNodeHandle(destination.getNode());
                            if (i==2)  // No information is stored in the fault manager... should never happen
                              {
                                //            throw Error(Error::SAFPLUS_ERROR, Error::MISCONFIGURATION, "Invalid messaging port [0]", __FILE__, __LINE__);
                                logCritical("MSG","SVR", "Fault manager has no information about this destination [%" PRIx64 ":%" PRIx64 "]", destination.id[0],destination.id[1]);
                                return NULL; 
                              }
                          }
                        else if (fs==FaultState::STATE_DOWN)
                          {
                            return NULL;
                          }
                        else 
                          {
                            assert(!"Impossible fault state");
                          }
                      }

                  }
              else
                  {
                    return &msgReply;
                  }
            }
        }
        else
        {
            /*
             * GAS: using mutex on wakeable object
             */
            wakeable->wake(0, &msgReply);
        }
        return &msgReply;
    }

}
