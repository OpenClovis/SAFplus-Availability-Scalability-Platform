#pragma once 
#ifndef COMPONENT_HXX__ 
#define COMPONENT_HXX__ 


namespace SAFplus 
{

class ComponentKey {
  public:
    std::string myName; 

    ComponentKey(){};

    ComponentKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ComponentKey &k2) const
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
#endif //COMPONENT_HXX