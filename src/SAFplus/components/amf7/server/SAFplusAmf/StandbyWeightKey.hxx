#pragma once 
#ifndef STANDBYWEIGHT_HXX__ 
#define STANDBYWEIGHT_HXX__ 


namespace SAFplus 
{

class StandbyWeightKey {
  public:
    std::string resource; 

    StandbyWeightKey(){};

    StandbyWeightKey(std::string resourceVal)
    {
        resource = resourceVal;
    }

    bool operator<(const StandbyWeightKey &k2) const
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
#endif //STANDBYWEIGHT_HXX