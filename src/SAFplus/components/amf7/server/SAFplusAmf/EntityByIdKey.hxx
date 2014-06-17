#pragma once 
#ifndef ENTITYBYID_HXX__ 
#define ENTITYBYID_HXX__ 


namespace SAFplus 
{

class EntityByIdKey {
  public:
    unsigned short int id; 

    EntityByIdKey(){};

    EntityByIdKey(unsigned short int idVal)
    {
        id = idVal;
    }

    bool operator<(const EntityByIdKey &k2) const
    {
        return id < k2.id;
    }

    std::string str()
    {
        std::stringstream ss;
        ss << id ;
        return ss.str(); 
    }

    void build(std::map<std::string,std::string> keyList)
    {
        /*This list can not be configured*/;
    }
}; //end class
} //end namespace
#endif //ENTITYBYID_HXX