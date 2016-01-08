#include <stdio.h>
#include <string.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "../app/cloudComm.h"

#define CLOUD_MGT_PORT 5743

using boost::asio::ip::tcp;
using namespace boost::asio::ip;

int main(int argc, char* argv[])
{
  
  
  try
  {
    if (argc < 3)
    {
      printf("cloudCfg <host> <cmd> [<cmd>] <args>\n");
      printf("Commands are:\n");
      printf("  add node <name>\n");
      printf("  rem node <name>\n");
      printf("  add app <app name> <node>\n");
      printf("  rem app <app name> <node>\n");
      return 1;
    }

    boost::asio::io_service io_service;
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(argv[1]);
    tcp::endpoint endpoint(addr, CLOUD_MGT_PORT);
    
    tcp::socket socket(io_service);
    socket.connect(endpoint);

    std::string app;
    std::string nodes;

    if (1)
    {
      uint16_t cmd = 0;
      if (strcmp(argv[2],"add")==0)
        {
          if (strcmp(argv[3],"node")==0)
            {
              cmd = CLOUD_MSG_ADD_NODE;
              nodes.append(argv[4]);
            }
          else if (strcmp(argv[3],"app")==0)
            {
              cmd = CLOUD_MSG_ADD_APP;
              app.append(argv[4]);
              nodes.append(argv[5]);
            }
         
        }
      if (strcmp(argv[2],"rem")==0)
        {
          if (strcmp(argv[3],"node")==0)
            {
              cmd = CLOUD_MSG_REM_NODE;
              nodes.append(argv[4]);
            }
          if (strcmp(argv[3],"app")==0)
            {
              app.append(argv[4]);
              nodes.append(argv[5]);
              cmd = CLOUD_MSG_REM_APP;
            }
        }
      if (cmd == 0)
        {
          printf("Invalid command %s %s\n", argv[2], argv[3]);
          socket.close();
          return 1;
        }
            
      char buf[4096];
      boost::system::error_code error;
      uint16_t magic = CLOUD_MSG_HDR_ID;
      memcpy(&buf[0], &magic, sizeof(uint16_t));
      memcpy(&buf[sizeof(uint16_t)],&cmd,sizeof(uint16_t));
      int pos = sizeof(uint16_t)*2;  // buffer position
      int slen = app.length();
      
      if (slen)  // If length 0 we don't need to send it... its not part of this message
        {
        memcpy(&buf[pos],app.c_str(),slen+1);  // +1 gets the null terminator
        pos += slen+1;
        }

      slen = nodes.length();
      memcpy(&buf[pos],nodes.c_str(),slen+1);  // +1 gets the null terminator
      
      socket.send(boost::asio::buffer(buf),slen + (2*sizeof(uint16_t)));
      
      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (len >= 2*sizeof(uint16_t))
        {
          uint16_t result=0;
          memcpy(&magic, &buf[0], sizeof(uint16_t));
          memcpy(&result, &buf[sizeof(uint16_t)],sizeof(uint16_t));
          if (magic != CLOUD_MSG_HDR_ID)
            {
              printf("Response error: invalid protocol identifier\n");
              socket.close();
              return 1;
            }
          if (result == CLOUD_MSG_OK)
            {
              printf("OK\n");
              socket.close();
              return 0;
            }
          else
            {
              printf("Response error: %d\n", result);
              socket.close();
              return result;
            }
        }

      socket.close();      
      if (error == boost::asio::error::eof)  // cleanly closed
        {
        }
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      //std::cout.write(buf.data(), len);
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
