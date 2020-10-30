#include <clGlobals.hxx>
#include <clGroupIpi.hxx>
#include <string>
/* The function will get the view of cluster in the format: 

NodeName    NodeType       HAState  NodeAddr
Node0       controller     Active   108
Node1       controller     Standby  109
Node2       Payload        -        110

*/
namespace SAFplus {
std::string getClusterView();

}

