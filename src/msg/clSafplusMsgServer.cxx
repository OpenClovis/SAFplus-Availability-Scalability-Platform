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


namespace SAFplus
{
    SafplusMsgServer safplusMsgServer;

    void msgServerInitialize(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads)
      {
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
      }

    void SAFplus::SafplusMsgServer::init(ClWordT _port, ClWordT maxPendingMsgs, ClWordT maxHandlerThreads, Options flags)
      {
        MsgServer::Init(_port, maxPendingMsgs, maxHandlerThreads, flags);
        MsgHandler *replyHandler = new MsgReplyHandler();
        this->RegisterHandler(SAFplusI::CL_IOC_SAF_MSG_REPLY_PROTO, replyHandler, &msgReply);
        SAFplus::iocPort = port; // Set this global to be used as a unique identifier for this component across the node.  There can be many MsgServers per component but only one SafplusMsgServer.
      }

    void SAFplus::SafplusMsgServer::registerHandler(ClWordT type, MsgHandler *handler, ClPtrT cookie)
    {
        SAFplus::MsgServer::RegisterHandler(type, handler, cookie);
    }

    void SAFplus::SafplusMsgServer::removeHandler(ClWordT type)
    {
        SAFplus::MsgServer::RemoveHandler(type);
    }

    MsgReply *SafplusMsgServer::sendReply(Handle destination, void* buffer, ClWordT length, ClWordT msgtype, Wakeable *wakeable)
    {
        memset(&msgReply, 0, sizeof(MsgReply));
        /*
         * Send message
         */
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
              if (!msgSendConditionMutex.timed_wait(msgSendReplyMutex, 4000)) // TODO: create customizable timeout
                  {
                    return NULL;  // TODO: TIMEOUT
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
