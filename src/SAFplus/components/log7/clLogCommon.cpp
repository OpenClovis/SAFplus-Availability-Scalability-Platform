#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <clLogIpi.hpp>

using namespace boost::interprocess;
using namespace SAFplusI;
using namespace SAFplus;

bool SAFplus::logCodeLocationEnable=true;

namespace SAFplusI
{
// Shared memory variables
shared_memory_object* clLogSharedMem=NULL;
mapped_region* clLogBuffer;
int clLogBufferSize=0;
LogBufferHeader* clLogHeader;

// Client mutex starts "given", server wakeup starts "taken"  
SAFplus::ProcSemT clientMutex("LogClientMutex",1);
SAFplus::ProcSemT serverSem("LogServerSem",0);


void logInitializeSharedMem()
{
    // If the shared memory buffer has size 0 then I better set it properly.
    offset_t shmsize=0;

    // Create or open the shared memory object.  If this process wins the race to create then it needs to create the mutex and initialize the header.
    // Otherwise place the header onto the location
    try  
      {
        clLogSharedMem= new shared_memory_object(create_only,"SAFplusLog",read_write);
        
        clLogSharedMem->truncate(SAFplus::CL_LOG_BUFFER_DEFAULT_LENGTH);
        clLogBuffer = new mapped_region(*clLogSharedMem,read_write);
        
        // Get the address of the mapped region
        void * addr       = clLogBuffer->get_address();   
       //shared_memory_buffer * data = new (addr) shared_memory_buffer;
        clLogHeader = new (addr) LogBufferHeader;
        clLogHeader->numRecords = 0;
        clLogHeader->msgOffset = sizeof(*clLogHeader);  /* First message text starts after the header */
        clLogHeader->structId = CL_LOG_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
      }
    catch (interprocess_exception &e)
      {
        clLogSharedMem = new shared_memory_object(open_only,"SAFplusLog",read_write);
        clLogBuffer    = new mapped_region(*clLogSharedMem,read_write);
        void * addr    = clLogBuffer->get_address();   
       //shared_memory_buffer * data = new (addr) shared_memory_buffer;
        clLogHeader =  static_cast<LogBufferHeader*>(addr);
        int retries = 0;
        while ((clLogHeader->structId != CL_LOG_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        if (retries>=2)
          {
            clLogHeader->numRecords = 0;
            clLogHeader->msgOffset = sizeof(*clLogHeader);
            clLogHeader->structId = CL_LOG_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
          }
      }
    
    // Get the shared memory size in case someone else created it.
    clLogSharedMem->get_size(shmsize);
    clLogBufferSize=shmsize;
    assert(clLogBufferSize != 0);
}  

void logTimeGet(char   *pStrTime, int maxBytes)
{
    struct timeval  tv = {0};
    ClCharT *tBuf = NULL;
    gettimeofday(&tv, NULL);
    ctime_r((time_t *) &tv.tv_sec, pStrTime);
    pStrTime[strlen(pStrTime) - 1] = 0;
    if( (tBuf = strrchr(pStrTime, ' ')) )
    {
        ClUint32T len;
        if((len = strlen(tBuf)) + 4 < maxBytes )
        {
            memmove(tBuf + 4, tBuf, len);
            snprintf(tBuf, 5, ".%.3d", (ClUint32T)tv.tv_usec/1000);
            tBuf[4] = ' ';
        }
    }
}

  
}
