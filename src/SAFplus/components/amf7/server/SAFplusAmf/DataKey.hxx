#pragma once 
#ifndef DATA_HXX__ 
#define DATA_HXX__ 


namespace SAFplus 
{

class DataKey {
  public:
    std::string myName; 

    DataKey(){};

    DataKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const DataKey &k2) const
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
#endif //DATA_HXX