#include <clGroupIpi.hxx>


int main(int argc,char *argv[])
  {
  SAFplusI::GroupSharedMem gsm;
  gsm.init();
  gsm.dbgDump();
  return 0;
  }
