#pragma once 
#ifndef SERVICEUNIT_HXX__ 
#define SERVICEUNIT_HXX__ 


namespace SAFplus 
{

class ServiceUnitKey {
  public:
    std::string myName; 

    ServiceUnitKey(){};

    ServiceUnitKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ServiceUnitKey &k2) const
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
#endif //SERVICEUNIT_HXX