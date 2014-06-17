#pragma once 
#ifndef SERVICEGROUP_HXX__ 
#define SERVICEGROUP_HXX__ 


namespace SAFplus 
{

class ServiceGroupKey {
  public:
    std::string myName; 

    ServiceGroupKey(){};

    ServiceGroupKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ServiceGroupKey &k2) const
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
#endif //SERVICEGROUP_HXX