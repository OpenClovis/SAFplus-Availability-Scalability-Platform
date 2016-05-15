#include <clNameApi.hxx>
#include <stdio.h>

using namespace SAFplus;

int main(int argc, char* argv[])
  {
    // SAFplus::ASP_NODEADDR = std::stoi(std::string(argv[1]));  // TODO: safplusInitialize should automatically find the node address!
  SafplusInitializationConfiguration sic;
  sic.iocPort     = 35;  // TODO: auto assign port
  sic.msgQueueLen = 10;
  sic.msgThreads  = 1;

  safplusInitialize( SAFplus::LibDep::NAME | SAFplus::LibDep::LOCAL,sic);
  for (NameRegistrar::Iterator it = name.begin(); it != name.end(); it++)
    {
    HandleData& hd = it.handle();
    if (hd.numHandles == 1)
      printf("%40s -> Node:%4d Port: %6d id:%8" PRIu64 "  Handle %" PRIx64 ":%" PRIx64 "\n", it.name().c_str(),hd.handles[0].getNode(), hd.handles[0].getPort(), hd.handles[0].getIndex(), hd.handles[0].id[0],hd.handles[0].id[1]);
    else
      {
      printf("%s -> %s [ ", it.name().c_str(),(hd.mappingMode==NameRegistrar::MODE_REDUNDANCY ? "redundant" : (hd.mappingMode==NameRegistrar::MODE_ROUND_ROBIN ? "round robin" :"local preferred")));

      for (int i = 0; i < hd.numHandles; i++)
        {
        printf("%" PRIx64 ".%" PRIx64 ,hd.handles[i].id[0],hd.handles[i].id[1]);
        if (i < hd.numHandles-1) printf (", ");
        }
      printf ("]\n");
      }
    }
  safplusFinalize();
  }
