#include "ComponentKey.hxx"

using namespace std;

namespace unitTest
{
  /* default constructor/destructor */
  ComponentKey::ComponentKey() 
  {
  }

  ComponentKey::~ComponentKey()
  {
  }

  ComponentKey::ComponentKey(std::string &nameValue, unsigned int &idValue, unsigned int &keyValue) 
  {
    name = nameValue;
    id = idValue;
    key = keyValue;
  }

  /* building an instance from string list */
  void ComponentKey::build(std::map<std::string,std::string> keyList)
  {
    std::map<std::string,std::string>::iterator iter;
    
    iter = keyList.find("name");
    if(iter != keyList.end())
    {
      std::stringstream nameStrStream;
      nameStrStream << iter->second;
      nameStrStream >> name;
    }

    iter = keyList.find("id");
    if(iter != keyList.end())
    {
      std::stringstream idStrStream;
      idStrStream << iter->second;
      idStrStream >> id;
    }

    iter = keyList.find("key");
    if(iter != keyList.end())
    {
      std::stringstream keyStrStream;
      keyStrStream << iter->second;
      keyStrStream >> key;
    }
  }

  /* key xpath: [key1=value1,key2=value2] */
  std::string ComponentKey::toXpath() const
  {
    std::stringstream ss;
    ss << "[";
    ss << "name=" <<name<< "," << "id=" <<id<< "," << "key=" <<key;
    ss << "]";
    return ss.str();
  }

  /* xml string: <interface key1="value1" key2="value2"></interface> */
  std::string ComponentKey::toString() const
  {
    std::stringstream ss;
    ss<< "name=\"" << name << "\" "<<"id=\"" << id << "\" "<<"key=\"" << key << "\"";
    return ss.str();
  }

  /* Serialize keys */
  std::string ComponentKey::str() const
  {
    std::stringstream ss;
    ss<< name <<id <<key;
    return ss.str();
  }

  std::ostream& operator<<(std::ostream& out, const ComponentKey &value)
  {
    out<< value.name <<value.id <<value.key;
    return out;
  }

}
/* namespace unitTest */
