/* 
 * File Protocol.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include <map>
#include "clMgtObject.hxx"
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "Nodes.hxx"
#include <string>
#include "Protocol.hxx"

using namespace std;

namespace SAFplusMsgServer {

    /* Apply MGT object factory */
    REGISTERIMPL(Protocol, /SAFplusMsgServer/protocol)

    Protocol::Protocol(): ClMgtObject("protocol"), defaultProtocol("defaultProtocol") {
        this->addChildObject(&defaultProtocol, "defaultProtocol");
    };

    vector<string> *Protocol::getChildNames() {
        string childNames[] = { "defaultProtocol", "nodes" };
        return new vector<string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusMsgServer/protocol/defaultProtocol
     */
    string Protocol::getDefaultProtocol() {
        return this->defaultProtocol.Value;
    };

    /*
     * XPATH: /SAFplusMsgServer/protocol/defaultProtocol
     */
    void Protocol::setDefaultProtocol(string defaultProtocolValue) {
        this->defaultProtocol.Value = defaultProtocolValue;
    };

    /*
     * XPATH: /SAFplusMsgServer/protocol/nodes
     */
    Nodes* Protocol::getNodes(string nameValue) {
        map<string, vector<ClMgtObject*>* >::iterator mapIndex = mChildren.find("nodes");
        /* Check if MGT module already exists in the database */
        if (mapIndex != mChildren.end()) {
            vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) (*mapIndex).second;
            for (unsigned int i = 0; i < objs->size(); i++) {
                ClMgtObject* childObject = (*objs)[i];
                if (((Nodes*)childObject)->getName() == nameValue) {
                    return (Nodes*)childObject;
                }
            }
        }
        return NULL;
    };

    /*
     * XPATH: /SAFplusMsgServer/protocol/nodes
     */
    void Protocol::addNodes(Nodes *nodesValue) {
        this->addChildObject(nodesValue, "nodes");
    };

    /*
     * XPATH: /SAFplusMsgServer/protocol/nodes
     */
    void Protocol::addNodes(string nameValue) {
        Nodes *objnodes = new Nodes(nameValue);
        this->addChildObject(objnodes, "nodes");
    };

    Protocol::~Protocol() {
    };

}
/* namespace SAFplusMsgServer */