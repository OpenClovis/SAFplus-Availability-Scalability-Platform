/* 
 * File Recovery.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef RECOVERY_HXX_
#define RECOVERY_HXX_

#include "SAFplusAmfCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"

namespace SAFplusAmf
  {

    enum class Recovery
      {
        None=0,NoRecommendation=1, Restart=2, Failover=3, NodeSwitchover=4, NodeFailover=5, NodeFailfast=6, ClusterReset=7, ApplicationRestart=8, ContainerRestart=9
      };
    std::ostream& operator<<(std::ostream& os, const Recovery& e);
    std::istream& operator>>(std::istream& is, Recovery& e);
    const char* c_str(const Recovery& e);

    /*
     * This is the class that will handle the conversion for us.
     */
    class RecoveryManager : public SAFplus::MgtEnumType<RecoveryManager, Recovery> {
        RecoveryManager();  // private to prevent instantiation
    public:
        static const map_t en2str_map;  // This is the lookup table.
    };
}
/* namespace ::SAFplusAmf */
#endif /* RECOVERY_HXX_ */
