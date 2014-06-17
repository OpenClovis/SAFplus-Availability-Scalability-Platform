#pragma once 
#ifndef ACTIVEWEIGHT_HXX__ 
#define ACTIVEWEIGHT_HXX__ 


namespace SAFplus 
{

class ActiveWeightKey {
  public:
    std::string resource; 

    ActiveWeightKey(){};

    ActiveWeightKey(std::string resourceVal)
    {
        resource = resourceVal;
    }

    bool operator<(const ActiveWeightKey &k2) const
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
#endif //ACTIVEWEIGHT_HXX