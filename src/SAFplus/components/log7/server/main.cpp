#include <clLogIpi.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace SAFplus;
using namespace SAFplusI;
using namespace boost::posix_time;

#define Dbg printf

class LogConfig
{
public:
  uint64_t logFlushInterval;
  LogConfig():logFlushInterval(5000) {}
};

LogConfig gConfig;

void postRecord(LogBufferEntry* rec, char* msg)
{
  printf("%s\n",msg);
  
}

int main(int argc, char* argv[])
{
  logInitializeSharedMem();

  while(1)
    {
      int recnum;
      //clLogHeader->clientMutex.wait();
      clientMutex.lock();
      int numRecords = clLogHeader->numRecords;  // move the value out of shared memory into a signed value
      //clLogHeader->clientMutex.post();
      clientMutex.unlock();
      if (numRecords)
        {
          char *base    = (char*) clLogBuffer->get_address();
          LogBufferEntry* rec = static_cast<LogBufferEntry*>((void*)(((char*)base)+clLogBufferSize-sizeof(LogBufferEntry)));  // Records start at the end and go upwards 
          for (recnum=0;recnum<numRecords-1;recnum++, rec--)
            {
              if (rec->offset == 0) continue;  // Bad record
              postRecord(rec,((char*) base)+rec->offset);
              rec->offset = 0;
            }

          // Now handle a few things that can't be done while clients are touching the records.
          clientMutex.lock();

          // Now get the last record and any that may have been added during our processing, 
          numRecords = clLogHeader->numRecords;    
          for (;recnum<numRecords;recnum++, rec--)
            {
              if (rec->offset == 0) continue;  // Bad record
              postRecord(rec,((char*) base)+rec->offset);
              rec->offset = 0;
            }

          clLogHeader->numRecords = 0; // No records left in the buffer
          clLogHeader->msgOffset = sizeof(LogBufferHeader); // All log messages consumed, so reset the offset.
          clientMutex.unlock(); // OK let the log clients back in
        }

      // Wait for more records
      if (serverSem.timed_lock(gConfig.logFlushInterval))
        {
          Dbg("timed_wait TRUE\n");
        }
      else
        {
          Dbg("timed_wait FALSE\n");
        }
    }
}
