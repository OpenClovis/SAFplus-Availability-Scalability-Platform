/* 
 * File Protocol.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#ifndef PROTOCOL_HXX_
#define PROTOCOL_HXX_

#include <map>
#include "clMgtObject.hxx"
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "Nodes.hxx"
#include <string>

namespace SAFplusMsgServer {

    class Protocol : public ClMgtObject {

        /* Apply MGT object factory */
        REGISTER(Protocol);

    public:

        /*
         * Takes as argument a default string.
         */
        ClMgtProv<std::string> defaultProtocol;

    public:
        Protocol();
        std::vector<std::string> *getChildNames();

        /*
         * XPATH: /SAFplusMsgServer/protocol/defaultProtocol
         */
        std::string getDefaultProtocol();

        /*
         * XPATH: /SAFplusMsgServer/protocol/defaultProtocol
         */
        void setDefaultProtocol(std::string defaultProtocolValue);

        /*
         * XPATH: /SAFplusMsgServer/protocol/nodes
         */
        Nodes* getNodes(std::string nameValue);

        /*
         * XPATH: /SAFplusMsgServer/protocol/nodes
         */
        void addNodes(Nodes *nodesValue);

        /*
         * XPATH: /SAFplusMsgServer/protocol/nodes
         */
        void addNodes(std::string nameValue);
        ~Protocol();

    };
}
/* namespace SAFplusMsgServer */
#endif /* PROTOCOL_HXX_ */
