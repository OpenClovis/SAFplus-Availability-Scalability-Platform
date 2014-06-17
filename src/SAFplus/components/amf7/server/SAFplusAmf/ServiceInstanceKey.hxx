#pragma once 
#ifndef SERVICEINSTANCE_HXX__ 
#define SERVICEINSTANCE_HXX__ 


namespace SAFplus 
{

class ServiceInstanceKey {
  public:
    std::string myName; 

    ServiceInstanceKey(){};

    ServiceInstanceKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ServiceInstanceKey &k2) const
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
        std::map<std::string,std::string>::iterator iter; 

        iter = keyList.find("myName");
        if(iter != keyList.end())
        {
            myName = iter->second;
        }
    }
}; //end class
} //end namespace
#endif //SERVICEINSTANCE_HXX