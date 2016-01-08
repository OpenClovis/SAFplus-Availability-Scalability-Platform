#include <ctime>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <byteswap.h>
#include <saAis.h>
#include <clCommonErrors.h>
#include "platform.h"
#include <algorithm>
#include <clLogApi.h>
using namespace std;
using boost::asio::ip::tcp;

#include "../cloudComm.h"

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, "DYN", "SVR", __VA_ARGS__)


// Monotonically increasing number, so that work assignments are certain to be unique
unsigned int uniquifier=0;


void server()
{
  try
    {
      boost::asio::io_service io_service;

      tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), CLOUD_MGT_PORT));

      for (;;)
	{
	  tcp::socket socket(io_service);
	  acceptor.accept(socket);
	  uint16_t response = 0;
	  bool endianSwap = false;
	  char hdr[128];  // TODO
	  size_t received = socket.receive(boost::asio::buffer(hdr));
	  if (received < CLOUD_MSG_HDR_LEN) continue;
	  uint16_t magic;
	  memcpy(&magic, &hdr[0], sizeof(uint16_t));  // memcpy works on ARM and other machines that must do aligned accesses.
	  // Rather than swapping to BE on the prevalent LE machines and then swapping back this code detects if the endian of the sender is
	  // the "opposite" of mine and will swap if needed.
	  if (magic == CLOUD_MSG_HDR_ID_SWAP) endianSwap = true;
	  if (endianSwap) magic = __bswap_16(magic);

	  if (magic != CLOUD_MSG_HDR_ID) // bad header
	    {
	      continue;
	    }

	  uint16_t command; 
	  memcpy(&command, &hdr[sizeof(uint16_t)], sizeof(uint16_t));  // command is offset 2 (after magic).
	  if (endianSwap) command = __bswap_16(command);
	  try
	    {
	      switch (command)
		{
		case CLOUD_MSG_ADD_NODE:
		  {
		    char nodeName[SA_MAX_NAME_LENGTH];
		    int maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - 2*sizeof(uint16_t)));
		    strncpy(nodeName, &hdr[sizeof(uint16_t)+sizeof(uint16_t)],maxlen);
		    nodeName[maxlen-1] = 0;  // force null terminate if nodeName overruns
		    clprintf(CL_LOG_SEV_INFO,"Add node: %s", nodeName);

		    addNode(nodeName);
		    nodeStart(nodeName);
		    response = CLOUD_MSG_OK;
		  } break;
		case CLOUD_MSG_REM_NODE:
		  {
		    char nodeName[SA_MAX_NAME_LENGTH];
		    int maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - 2*sizeof(uint16_t)));
		    strncpy(nodeName, &hdr[sizeof(uint16_t)+sizeof(uint16_t)],maxlen);
		    nodeName[maxlen-1] = 0;  // force null terminate if nodeName overruns
		    clprintf(CL_LOG_SEV_INFO,"Remove node: %s", nodeName);
          
		    // done automatically by remove: nodeStop(nodeName);
		    removeNode(nodeName);
		    response = CLOUD_MSG_OK;
		  } break;
		case CLOUD_MSG_ADD_APP:
		  {
		    int pos;
		    char appName[SA_MAX_NAME_LENGTH];
		    int maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - 2*sizeof(uint16_t)));
		    strncpy(appName, &hdr[sizeof(uint16_t)+sizeof(uint16_t)],maxlen);
		    appName[maxlen-1] = 0;  // force null terminate if nodeName overruns

		    pos = sizeof(uint16_t)+sizeof(uint16_t) + strlen(appName)+1;  // 1 for the null termination 

		    char nodeName[SA_MAX_NAME_LENGTH];
		    maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - pos));
		    strncpy(nodeName, &hdr[pos],maxlen);
		    nodeName[maxlen-1] = 0;  // force null terminate if nodeName overruns
          
		    clprintf(CL_LOG_SEV_INFO,"Add app: %s, %s", appName, nodeName);

		    SafConfig cfg;
		    cfg.binName = appName;
		    cfg.compRestartDuration = 1000;     // wait a second.
		    cfg.compRestartDuration = 10000000;  // restart forever
          
		    if (!acExists(appName))  // Create the SG if it does not exist
		      {
			addAppCnt(appName,&cfg);
			acStart(appName);
		      }

                    // Extend the SG onto a particular node
		    const char* nodeNames[1];
		    nodeNames[0] = nodeName;
		    acExtend(appName, nodeNames, 1,NULL,&cfg);

                    // Create a work assignment so this node has something to do
                    char wrk[CL_MAX_NAME_LENGTH];
                    wrk[0] = 0;
                    snprintf(wrk,CL_MAX_NAME_LENGTH,"%s_%d_work",appName,uniquifier);
                    uniquifier++;
                    const char* activecfg = ".";
                    const char* standbycfg = ".";
                    acAddWork(appName, wrk, activecfg, standbycfg);
                    
		    response = CLOUD_MSG_OK;
            
		    //acStart(appCntName);     
		  } break;     
     
		case CLOUD_MSG_REM_APP:
                  {
		  int pos;
		  char appName[SA_MAX_NAME_LENGTH];
		  int maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - 2*sizeof(uint16_t)));
		  strncpy(appName, &hdr[sizeof(uint16_t)+sizeof(uint16_t)],maxlen);
		  appName[maxlen-1] = 0;  // force null terminate if nodeName overruns

		  pos = sizeof(uint16_t)+sizeof(uint16_t) + strlen(appName)+1;  // 1 for the null termination 

		  char nodeName[SA_MAX_NAME_LENGTH];
		  maxlen = min(SA_MAX_NAME_LENGTH,(int)(received - pos));
		  strncpy(nodeName, &hdr[pos],maxlen);
		  nodeName[maxlen-1] = 0;  // force null terminate if nodeName overruns
          
		  clprintf(CL_LOG_SEV_INFO,"Remove app: %s, %s", appName, nodeName);

		  const char* nodeNames[1];
		  nodeNames[0] = nodeName;       
		  acRetract(appName, nodeNames, 1,NULL);
#if 0   
		  if (empty)
		    {
		      void removeAppCnt(const char* safName);
		    }
#endif          
		  } break;
		default:
		  clprintf(CL_LOG_SEV_ERROR,"Unknown command [%d] sent", command);
		  // TODO: 
		  break;
		}
	    }
	  catch (AmfException& e)
	    {
              clprintf(CL_LOG_SEV_ERROR,"Exception raised.  Error [%x]", e.amfErrCode);
	      response = CL_GET_ERROR_CODE(e.amfErrCode);
	    }
	  boost::system::error_code ignored_error;
	  //boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
	  char data[sizeof(uint16_t)*2];
	  magic = CLOUD_MSG_HDR_ID;
	  memcpy(data, &magic, sizeof(uint16_t));
	  magic = CLOUD_MSG_HDR_ID;
	  memcpy(&data[sizeof(uint16_t)], &response, sizeof(uint16_t));
      
	  socket.send(boost::asio::buffer(data, sizeof(uint16_t)*2));
	}
    }
  catch (std::exception& e)
    {
      clprintf(CL_LOG_SEV_ERROR,"Exception raised.  Error [%s]", e.what());
      std::cerr << e.what() << std::endl;
    }

}
