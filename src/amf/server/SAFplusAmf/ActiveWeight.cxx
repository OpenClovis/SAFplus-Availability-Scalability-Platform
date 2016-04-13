/* 
 * File ActiveWeight.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtFactory.hxx"
#include "Capacity.hxx"
#include <string>
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "ActiveWeight.hxx"

using namespace  std;

namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(ActiveWeight, /SAFplusAmf/safplusAmf/ServiceInstance/activeWeight)

    ActiveWeight::ActiveWeight()
    {
        this->tag.assign("activeWeight");
    };

    ActiveWeight::ActiveWeight(const std::string& resourceValue)
    {
        this->resource.value =  resourceValue;
        this->tag.assign("activeWeight");
    };

    std::vector<std::string> ActiveWeight::getKeys()
    {
        std::string keyNames[] = { "resource" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* ActiveWeight::getChildNames()
    {
        std::string childNames[] = { "resource", "value" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    ActiveWeight::~ActiveWeight()
    {
    };

}
/* namespace ::SAFplusAmf */
