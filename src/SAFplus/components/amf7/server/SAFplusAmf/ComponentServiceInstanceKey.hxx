#pragma once 
#ifndef COMPONENTSERVICEINSTANCE_HXX__ 
#define COMPONENTSERVICEINSTANCE_HXX__ 


namespace SAFplus 
{

class ComponentServiceInstanceKey {
  public:
    std::string myName; 

    ComponentServiceInstanceKey(){};

    ComponentServiceInstanceKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ComponentServiceInstanceKey &k2) const
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
#endif //COMPONENTSERVICEINSTANCE_HXX