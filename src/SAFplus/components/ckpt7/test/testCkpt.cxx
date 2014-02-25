#include <clCommon.hpp>
#include <clLogApi.hpp>
#include <clGlobals.hpp>

#include <clCkptApi.hxx>

using namespace SAFplus;


void test_readwrite()
{
  Checkpoint c1(Checkpoint::SHARED | Checkpoint::LOCAL);
  c1.stats();
  c1.dump();
  //c1.remove();
  if (1)
    {
      char vdata[sizeof(Buffer)-1+sizeof(int)*10];
      Buffer* val = new(vdata) Buffer(sizeof(int)*10);
#if 0
      for (int i=0;i<100;i++)
        {
          for (int j=0;j<10;j++)  // Set the data to something verifiable
            {
              ((int*)val->data)[j] = i+j;
            }
          c1.write(i,*val);
        }
#endif
      for (int i=0;i<100;i++)
        {
          const Buffer& output = c1.read(i);
          if (&output == NULL)
            {
              printf("NOT FOUND i:%d \n", i);
              
            }
          else
            {
          for (int j=0;j<10;j++)
            {
              int tmp = ((int*)output.data)[j];
              if (tmp != i+j)
                {
                  printf("MISCOMPARE i:%d, j:%d, expected:%d, got:%d\n", i,j,i+j,tmp);
                }
            }
            }
        }

      printf("ITERATOR: \n");
      for (Checkpoint::Iterator i=c1.begin();i!=c1.end();i++)
        {
          SAFplus::Checkpoint::KeyValuePair& item = *i;
          int tmp = *((int*) (*item.first).data);
          printf("key: %d, value: %s\n",tmp,(*item.second).data);
        }
    }
}

  
int main(int argc, char* argv[])
{
  logInitialize();
  utilsInitialize();
  test_readwrite();
}
