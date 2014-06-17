#pragma once 
#ifndef APPLICATION_HXX__ 
#define APPLICATION_HXX__ 


namespace SAFplus 
{

class ApplicationKey {
  public:
    std::string myName; 

    ApplicationKey(){};

    ApplicationKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ApplicationKey &k2) const
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
#endif //APPLICATION_HXX