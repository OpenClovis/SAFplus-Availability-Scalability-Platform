#pragma once 
#ifndef ENTITYBYNAME_HXX__ 
#define ENTITYBYNAME_HXX__ 


namespace SAFplus 
{

class EntityByNameKey {
  public:
    std::string myName; 

    EntityByNameKey(){};

    EntityByNameKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const EntityByNameKey &k2) const
    {
        return myName < k2.myName;
    }

    std::string str()
    {
        std::stringstream ss;
        ss << myName ;
        return ss.str(); 
    }

    void build(std::map<std::string,std::string> keyList)
    {
        /*This list can not be configured*/;
    }
}; //end class
} //end namespace
#endif //ENTITYBYNAME_HXX