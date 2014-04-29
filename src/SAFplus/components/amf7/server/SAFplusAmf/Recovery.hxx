/* 
 * File Recovery.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#ifndef RECOVERY_HXX_
#define RECOVERY_HXX_

#include "MgtEnumType.hxx"
#include <iostream>

namespace SAFplusAmf {

    enum class Recovery {
        NoRecommendation=1, Restart=2, Failover=3, NodeSwitchover=4, NodeFailover=5, NodeFailfast=6, ClusterReset=7, ApplicationRestart=8, ContainerRestart=9
    };
    std::ostream& operator<<(std::ostream& os, const Recovery& e);
    std::istream& operator>>(std::istream& is, Recovery& e);

    /*
     * This is the class that will handle the conversion for us.
     */
    class RecoveryManager : public SAFplus::MgtEnumType<RecoveryManager, Recovery> {
        RecoveryManager();  // private to prevent instantiation
    public:
        static const vec_t en2str_vec;  // This is the lookup table.
    };
}
/* namespace SAFplusAmf */
#endif /* RECOVERY_HXX_ */