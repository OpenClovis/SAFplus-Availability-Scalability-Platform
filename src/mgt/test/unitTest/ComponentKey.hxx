#pragma once 
#ifndef __COMPONENTKEY_HXX__
#define __COMPONENTKEY_HXX__

#include <iostream>
#include <sstream>
#include <map>

namespace unitTest
{
  class ComponentKey {

    public:
      std::string name;
      unsigned int id;
      unsigned int key;

      /* default constructor/destructor */
      ComponentKey();
      ~ComponentKey();

      ComponentKey(std::string &nameValue, unsigned int &idValue, unsigned int &keyValue);

      /* building an instance from string list */
      void build(std::map<std::string,std::string> keyList);

      /* key xpath: [key1=value1,key2=value2] */
      std::string toXpath() const;

      /* xml string: <interface key1="value1" key2="value2"></interface> */
      std::string toString() const;

      /* Serialize keys */
      std::string str() const;

      friend std::ostream& operator<<(std::ostream& out, const ComponentKey &value);

      /* compare hash key */
      inline bool operator<(const ComponentKey &rhs) const
      {
        /* TODO: implement by user */
        return true;
      }

  };
}
/* namespace unitTest */
#endif /* COMPONENTKEY_HXX_ */
