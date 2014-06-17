#pragma once 
#ifndef CLUSTER_HXX__ 
#define CLUSTER_HXX__ 


namespace SAFplus 
{

class ClusterKey {
  public:
    std::string myName; 

    ClusterKey(){};

    ClusterKey(std::string myNameVal)
    {
        myName = myNameVal;
    }

    bool operator<(const ClusterKey &k2) const
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
#endif //CLUSTER_HXX