#include "../server/GroupServer.hxx"

using namespace SAFplus;
using namespace SAFplusI;
ClUint32T clAspLocalId = atoi(getenv("NODEADDR"));
ClBoolT gIsNodeRepresentative = CL_TRUE;
int main(int argc, char * argv[])
{
  char pause;
  std::cout << "Start server \n";
  GroupServer::getInstance()->clGrpStartServer();
  sleep(30);
  if(atoi(getenv("NODEADDR")) == 2)
  {
    std::cout << "Send election \n";
    GroupServer::getInstance()->dumpClusterNodeGroup();
    GroupServer::getInstance()->elect();
  }
  sleep(30);
  std::cin >> pause;
  GroupServer::getInstance()->clGrpStopServer();
  sleep(5);
  return 0;
}
