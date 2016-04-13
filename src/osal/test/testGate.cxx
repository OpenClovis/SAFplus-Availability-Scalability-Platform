#include <clThreadApi.hxx>

using namespace SAFplus;

int main(int argc, char** argv)
{
  ProcGate gate(1);
  bool quit = false;

  while(!quit)
    {
      printf("\n> ");
      fflush(stdout);
      char line[80];
      int i;
      if (fgets(line, sizeof(line), stdin)) 
        {
          if (strncmp("abort", line,5) == 0) abort();
          if (strncmp("exit", line,4) == 0) quit = true;
          if (strncmp("seg", line,3) == 0) { char *c= NULL; char d=*c; }
          if (strncmp("lock", line,4) == 0) 
            { 
              gate.lock();
              printf("gate locked\n");
            }
          if (strncmp("unlock", line,6) == 0) 
            { 
              gate.unlock();
              printf("gate unlocked\n");
            }
          if (strncmp("close", line,5) == 0) 
            { 
              gate.close();
              printf("gate closed\n");
            }
          if (strncmp("open", line,4) == 0) 
            { 
              gate.open();
              printf("gate opened\n");
            }


        }
    }

}
