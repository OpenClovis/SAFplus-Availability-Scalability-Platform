#pragma once
#include <algorithm>
#include <assert.h>
#include <string.h>

namespace SAFplus
{

  template<class T> class DoublingBuffer
  {
  public:
    T *buf;
    unsigned int maxSize;
    unsigned int curSize;

    DoublingBuffer(unsigned int startSize)
    {
      buf = new T[startSize];
      assert(buf);
      maxSize = startSize;
      curSize = 0;
    }
    DoublingBuffer<T>& operator += (const T& item)
    {
      if (curSize == maxSize) resize(maxSize*2);
      if (curSize < maxSize)
        {
          buf[curSize] = item;
          curSize++;
        }
      else
        {
          assert(0);
        }
    }

    DoublingBuffer<T>& append (const T* items,int nItems)
    {
      if (curSize+nItems >= maxSize) resize(std::max(maxSize*2,curSize+nItems));
      if (curSize < maxSize)
        {
          memcpy(&buf[curSize],items,nItems*sizeof(T));
          curSize+=nItems;
        }
      else
        {
          assert(0);
        }
      return *this;
    }
    
    void resize(unsigned int amt)
    {
      T* oldbuf = buf;
      assert(amt > curSize);
      maxSize = amt;
      buf = new T[maxSize];
      memcpy(buf,oldbuf,curSize*sizeof(T));
      delete[] oldbuf;
    }

    unsigned int size()  // Returns the size in units of T
    {
      return curSize;
    }

    int fwrite(FILE* fp)
    {
      return ::fwrite(buf,curSize,sizeof(T),fp);
    }
    void consume()
    {
      curSize = 0;
    }
    operator T *() const{ return this->buf; }
    operator T *&(){ return this->buf; }
  };

  class DoublingCharBuffer: public DoublingBuffer<char>
  {
  public:
    DoublingCharBuffer(unsigned int startSize):DoublingBuffer<char>(startSize) {}
    
    DoublingCharBuffer& operator += (const char* s)
    {
      int len = strlen(s);
      append(s,len);
      return *this;
    }  
  };
  // This class is specific for log replication. The buffer is type of char* but it can contain other datatypes e.g short int not just string. We cannot use DoublingCharBuffer for this purpose
  class ReplicationMessageBuffer
  {
  public:
    unsigned int curSize;
    char* buf;
    ReplicationMessageBuffer()
    {
      curSize=0;
      buf=NULL;
    }
    ReplicationMessageBuffer& append(void* val, int valLen)
    {
      char* temp = buf;        
      buf = new char[curSize+valLen];
      if (temp)
      {
        memcpy(buf, temp, curSize);
      }
      memcpy(buf+curSize, val, valLen);
      curSize+=valLen;
      delete[] temp;      
      return *this;
    }
    void release()
    {
      curSize=0;
      if (buf)
      {
        delete[] buf;
        buf = NULL;
      }
    }    
  };
#if 0
  class DoublingShortBuffer: public DoublingBuffer<short>
  {
  public:
    DoublingShortBuffer(short startSize):DoublingBuffer<short>(startSize) {}
    
    DoublingShortBuffer& operator += (short val)
    {
      int len = 1;
      append(val,len);
      return *this;
    }
    DoublingShortBuffer& append (short item, int nItems)
    {
      if (curSize+nItems >= maxSize) resize(std::max(maxSize*2,curSize+nItems));
      if (curSize < maxSize)
        {
          memcpy(&buf[curSize],&item,nItems*sizeof(short));
          curSize+=nItems;
        }
      else
        {
          assert(0);
        }
      return *this;
    }
  };
#endif
};
