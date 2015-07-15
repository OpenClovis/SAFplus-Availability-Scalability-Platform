#include <EntityByIdKey.hxx>

using namespace std;

namespace SAFplusAmf
{
  /* default constructor/destructor */
  EntityByIdKey::EntityByIdKey() 
  {
  }

  EntityByIdKey::~EntityByIdKey()
  {
  }

  EntityByIdKey::EntityByIdKey(unsigned short int idValue) 
  {
    id = idValue;
  }

  /* building an instance from string list */
  void EntityByIdKey::build(std::map<std::string,std::string> &keyList)
  {
    std::map<std::string,std::string>::iterator iter;
    
    iter = keyList.find("id");
    if(iter != keyList.end())
    {
      std::stringstream idStrStream;
      idStrStream << iter->second;
      idStrStream >> id;
    }
  }

  /* key xpath: [@key1="value1" and @key2="value2" and ...] */
  std::string EntityByIdKey::toXpath() const
  {
    std::stringstream ss;
    ss << "[" << "@id=\"" <<id <<"\"" << "]";
    return ss.str();
  }

  /* xml string: <interface key1="value1" key2="value2"></interface> */
  std::string EntityByIdKey::toString() const
  {
    std::stringstream ss;
    ss<< "id=\"" << id << "\"";
    return ss.str();
  }

  /* Serialize keys */
  std::string EntityByIdKey::str() const
  {
    std::stringstream ss;
    ss<< id;
    return ss.str();
  }

  std::ostream& operator<<(std::ostream& out, const EntityByIdKey &value)
  {
    out<< value.id;
    return out;
  }

}
/* namespace SAFplusAmf */
