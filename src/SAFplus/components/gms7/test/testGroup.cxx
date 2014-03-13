// Standard includes
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <boost/functional/hash.hpp>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#include <clGroup.hxx>

using namespace SAFplus;

int main()
{
  Group gms("test");
  EntityIdentifier me = SAFplus::Handle::create();
  uint64_t credentials = 5;
  std::string data = "DATA";
  int dataLength = 4;
  uint capabilities = 10;
  gms.registerEntity(me,credentials,(void *)&data,dataLength,capabilities,false);

  for (Group::Iterator i = gms.begin();i != gms.end();i++)
  {
    SAFplus::Group::KeyValuePair& item = *i;
    EntityIdentifier *key = (EntityIdentifier *)&item.first;
    GroupIdentity    *val = (GroupIdentity *)&item.second;

    printf("%s ",val->data);
  }

  return 0;

}
