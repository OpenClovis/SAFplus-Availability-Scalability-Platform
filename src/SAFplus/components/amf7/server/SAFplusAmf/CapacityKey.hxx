#pragma once 
#ifndef CAPACITY_HXX__ 
#define CAPACITY_HXX__ 


namespace SAFplus 
{

class CapacityKey {
  public:
    std::string resource; 

    CapacityKey(){};

    CapacityKey(std::string resourceVal)
    {
        resource = resourceVal;
    }

    bool operator<(const CapacityKey &k2) const
    {
        return resource < k2.resource;
    }

    std::string str()
    {
        std::stringstream ss;
        ss << resource ;
        return ss.str(); 
    }

    void build(std::map<std::string,std::string> keyList)
    {
        std::map<std::string,std::string>::iterator iter; 

        iter = keyList.find("resource");
        if(iter != keyList.end())
        {
            resource = iter->second;
        }
    }
}; //end class
} //end namespace
#endif //CAPACITY_HXX