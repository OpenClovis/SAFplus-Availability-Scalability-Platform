#pragma once
#ifndef RESET_HXX_
#define RESET_HXX_

#include "MgtFactory.hxx"
#include "clMgtProv.hxx"
#include "clMgtRpc.hxx"
#include "clTransaction.hxx"
#include <string>
#include "myServiceCommon.hxx"
#include <cstdint>
#include <vector>

namespace myService
  {

    class reset : public SAFplus::MgtRpc {

        /* Apply MGT object factory */
        MGT_REGISTER(reset);

    public:

        /*
         * What TCP port to listen on
         */
        SAFplus::MgtProv<::uint16_t> port;

        /*
         * What directory on the server contains the pages
         */
        SAFplus::MgtProv<std::string> homeLocation;

    public:
        reset();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /myService/serviceCfg/port
         */
        ::uint16_t getPort();

        /*
         * XPATH: /myService/reset/port
         */
        void setPort(::uint16_t portValue, SAFplus::Transaction &txn=SAFplus::NO_TXN);

        /*
         * XPATH: /myService/reset/homeLocation
         */
        std::string getHomeLocation();

        /*
         * XPATH: /myService/reset/homeLocation
         */
        void setHomeLocation(std::string homeLocationValue, SAFplus::Transaction &txn=SAFplus::NO_TXN);

        virtual ClBoolT validate() {return CL_TRUE;}
        virtual ClBoolT invoke() {return CL_TRUE;}
        virtual ClBoolT postReply() {return CL_TRUE;}

        virtual ClRcT doRpc(const std::string &attrs = "");
        ~reset();

    };
}
/* namespace ::reset */
#endif /* RESET_HXX_ */
