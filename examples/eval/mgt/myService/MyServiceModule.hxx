/* 
 * File MyServiceModule.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef MYSERVICEMODULE_HXX_
#define MYSERVICEMODULE_HXX_

#include "ServiceCfg.hxx"
#include "ServiceStats.hxx"
#include "MgtFactory.hxx"
#include "clMgtModule.hxx"
#include "myServiceCommon.hxx"
#include "clMgtList.hxx"
//#include "AccessCounts.hxx"
#include <vector>

namespace myService
  {

    class MyServiceModule : public SAFplus::MgtModule {

        /* Apply MGT object factory */
        MGT_REGISTER(MyServiceModule);

    public:
        ::myService::ServiceCfg serviceCfg;
        ::myService::ServiceStats serviceStats;

        /*
         * 
         */
        SAFplus::MgtList<std::string> subscribersList;

    public:
        MyServiceModule();
        std::vector<std::string>* getChildNames();
        ~MyServiceModule();

    };
}
/* namespace ::myService */
#endif /* MYSERVICEMODULE_HXX_ */
