#include <clNameApi.hxx>
#include <stdio.h>

using namespace SAFplus;

int main(int argc, char* argv[])
  {
  SAFplus::ASP_NODEADDR = std::stoi(std::string(argv[1]));  // TODO: safplusInitialize should automatically find the node address!

  safplusInitialize( SAFplus::LibDep::NAME);
  for (NameRegistrar::Iterator it = name.begin(); it != name.end(); it++)
    {
    HandleData& hd = it.handle();
    if (hd.numHandles == 1)
      printf("%s -> %lx:%lx\n", it.name().c_str(),hd.handles[0].id[0],hd.handles[0].id[1]);
    else
      {
      printf("%s -> %s [ ", it.name().c_str(),(hd.mappingMode==NameRegistrar::MODE_REDUNDANCY ? "redundant" : (hd.mappingMode==NameRegistrar::MODE_ROUND_ROBIN ? "round robin" :"local preferred")));

      for (int i = 0; i < hd.numHandles; i++)
        {
        printf("%lx.%lx",hd.handles[i].id[0],hd.handles[i].id[1]);
        if (i < hd.numHandles-1) printf (", ");
        }
      printf ("]\n");
      }
    }
  }
