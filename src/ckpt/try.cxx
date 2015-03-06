#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>


#include <boost/unordered_map.hpp>     //boost::unordered_map


#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash
 
  using namespace boost::interprocess;

typedef int    KeyType;
typedef float  MappedType;
typedef std::pair<const int, float> ValueType;

// Typedef the allocator
typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

// Alias an unordered_map of ints that uses the previous STL-like allocator.
typedef boost::unordered_map < KeyType, MappedType, boost::hash<KeyType>, std::equal_to<KeyType>, ShmemAllocator> MyHashMap;


int main ()
{
   //Create shared memory
   managed_shared_memory segment(create_only, "test", 65536);

   //Construct a shared memory hash map.
   //Note that the first parameter is the initial bucket count and
   //after that, the hash function, the equality function and the allocator
   MyHashMap *myhashmap = segment.construct<MyHashMap>("MyHashMap")  //object name
      ( 3, boost::hash<int>(), std::equal_to<int>()                  //
      , segment.get_allocator<ValueType>());                         //allocator instance

      //Insert data in the hash map
   for(int i = 0; i < 100; ++i){
      myhashmap->insert(ValueType(i, (float)i));
   }
};
