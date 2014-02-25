#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>

#include <boost/unordered_map.hpp>     //boost::unordered_map

#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

#include <../clCkptIpi.hxx>
 
using namespace boost::interprocess;

typedef SAFplusI::BufferPtr   KeyType;
typedef SAFplusI::BufferPtr  MappedType;
typedef std::pair<const KeyType, MappedType> ValueType;

// Typedef the allocator
typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

// Alias an unordered_map of ints that uses the previous STL-like allocator.
typedef boost::unordered_map < KeyType, MappedType, boost::hash<KeyType>, SAFplusI::BufferPtrContentsEqual, ShmemAllocator> MyHashMap;

void remove()
{
   shared_memory_object::remove("test");
}

int main ()
{

   //Create shared memory
   managed_shared_memory segment(open_or_create, "test", 65536);

   //       void* temp = segment.allocate(16);
   //     segment.deallocate(temp);
   printf("Named: %lu, unique: %lu\n",segment.get_num_named_objects(),segment.get_num_unique_objects());

   SAFplusI::CkptBufferHeader *hdr=NULL;
   try
     {
         hdr = segment.construct<SAFplusI::CkptBufferHeader>("header") ();                                 // Ok it created one so initialize
         //hdr->handle = 0;
         hdr->serverPid = 0; //getpid();
         hdr->structId=SAFplusI::CL_CKPT_BUFFER_HEADER_STRUCT_ID_7;
     }
   catch (interprocess_exception &e)
     {
       if (e.get_error_code() == already_exists_error)
	 {
         hdr = segment.find_or_construct<SAFplusI::CkptBufferHeader>("header") ();                         //allocator instance
         hdr->serverPid++;
	 }
       else throw;
     }
   

   //Construct a shared memory hash map.
   //Note that the first parameter is the initial bucket count and
   //after that, the hash function, the equality function and the allocator
   MyHashMap *myhashmap = segment.find_or_construct<MyHashMap>("MyHashMap")  //object name
     ( 3, boost::hash<KeyType>(), SAFplusI::BufferPtrContentsEqual()                  //
      , segment.get_allocator<ValueType>());                         //allocator instance


   printf("PRE:\n");
   for(MyHashMap::const_iterator iter = myhashmap->cbegin(); iter != myhashmap->cend(); iter++)
     {
       MyHashMap::value_type t = *iter;
       if (t.first && t.second)
         printf("%s -> %s\n", t.first->data,t.second->data);
     }

      //Insert data in the hash map
   for(int i = 0; i < 1; ++i)
     {
       SAFplus::Buffer* k = new (segment.allocate(10+sizeof(SAFplus::Buffer))) SAFplus::Buffer (10);  // Place a new buffer object into the segment
       *k = (char) 0;
       snprintf(*k,k->len(),"k%3d",i);
       SAFplus::Buffer* j = new (malloc(10+sizeof(SAFplus::Buffer))) SAFplus::Buffer (10);  // Place a new buffer object into the segment
       *j = (char) 0;
       snprintf(*j,j->len(),"k%3d",i);
       SAFplusI::BufferPtr& t = (*myhashmap)[SAFplusI::BufferPtr(j)];
       if(t)
	 {
	   printf("%s ", t->data);
           // Lay it directly on if the buffer is same size and ref = 1
           //snprintf(t->data,t->len(),"v%5d",(hdr->serverPid*1000)+i);
	   SAFplus::Buffer* v = new (segment.allocate(10+sizeof(SAFplus::Buffer))) SAFplus::Buffer (10);  // Place a new buffer object into the segment
	   *v = (char) 0;
	   //snprintf(*v,v->len(),"v%5d",i);
           snprintf(*v,v->len(),"v%5d",(hdr->serverPid*1000)+i);
	   SAFplus::Buffer* old = t.get();
           t = v;
           if (old->ref()==1) segment.deallocate(old);
	 }
       else
	 {
           
	   SAFplus::Buffer* v = new (segment.allocate(10+sizeof(SAFplus::Buffer))) SAFplus::Buffer (10);  // Place a new buffer object into the segment
	   *v = (char) 0;
	   //snprintf(*v,v->len(),"v%5d",i);
           snprintf(*v,v->len(),"v%5d",(hdr->serverPid*1000)+i);
           
           SAFplusI::BufferPtr kb(k),kv(v);
           ValueType vt(kb,kv);
           myhashmap->erase(kb);
	   myhashmap->insert(vt);
	 }
}

   printf("\n POST:\n");
   for(MyHashMap::const_iterator iter = myhashmap->cbegin(); iter != myhashmap->cend(); iter++)
      {
       MyHashMap::value_type t = *iter;
       if (t.first) printf("%s ->", t.first->data);
       if (t.second)
	 printf("%s\n", t.second->data);
       else printf("nada\n");
     }

};
