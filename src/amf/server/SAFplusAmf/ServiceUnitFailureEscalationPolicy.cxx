/* 
 * File ServiceUnitFailureEscalationPolicy.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "EscalationPolicy.hxx"
#include "MgtFactory.hxx"
#include "ServiceUnitFailureEscalationPolicy.hxx"


namespace SAFplusAmf
  {

    ServiceUnitFailureEscalationPolicy::ServiceUnitFailureEscalationPolicy()
    {
        this->addChildObject(&failureCount, "failureCount");
        failureCount = 0;
        this->tag.assign("serviceUnitFailureEscalationPolicy");
    };

    std::vector<std::string>* ServiceUnitFailureEscalationPolicy::getChildNames()
    {
        std::string childNames[] = { "maximum", "duration", "failureCount" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    ServiceUnitFailureEscalationPolicy::~ServiceUnitFailureEscalationPolicy()
    {
    };

}
/* namespace ::SAFplusAmf */
