#include "../server/GroupServer.hxx"

using namespace SAFplus;
using namespace SAFplusI;
ClUint32T clAspLocalId= 1; 
ClBoolT gIsNodeRepresentative = CL_TRUE;
int main(int argc, char * argv[])
{
  char pause;
  std::cout << "Start server \n";
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging

  char* node = getenv("NODEADDR");
  if (node) clAspLocalId = atoi(node);
  GroupServer* gs = GroupServer::getInstance();

  gs->clGrpStartServer();
  sleep(30);
  if(clAspLocalId == 2)
  {
    std::cout << "Send election \n";
    gs->dumpClusterNodeGroup();
    gs->elect();
  }
  sleep(30);
  std::cin >> pause;
  gs->clGrpStopServer();
  sleep(5);
  return 0;
}
