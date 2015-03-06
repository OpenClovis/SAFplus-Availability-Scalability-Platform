#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <exception>
#include <stdint.h>
#include <stdio.h>

// Structure describing SAF-level configuration like redundancy mode, instantiation
// delays, etc. It is used by the Application Container since the AppCnt is a SAF SG.
// It is your job to allocate and delete strings.
typedef struct
{
    const char* binName;  // Set to NULL for the default name
    int compRestartDuration;
    int compRestartCountMax; 
    
} SafConfig;

class AmfException: public std::exception
{
public:
  AmfException(int errcode)
  {
      amfErrCode = errcode;      
  }
  
    int amfErrCode;
};


// All functions can raise AmfException


// Create a new Node in the AMF
void addNode(const char* safName);
// Remove a node from the AMF
void removeNode(const char* safName);

// Create a new application container in the AMF
void addAppCnt(const char* safName,SafConfig* cfg=NULL);
// Remove the application container in the AMF.  This application container must not be "extended" on any nodes (call acRetract before this function)
void removeAppCnt(const char* safName);

void nodeStart(const char* nodeName);
void nodeStop(const char* nodeName);

void suStart(const char* name);
void suStop(const char* name);



void nodeFail(const char* nodeName);

// Start this app container running on these nodes (1+N redundancy).  This will
// start a SAF-aware process on each node specified.
void acExtend(const char* appCnt, const char* nodeNames[], int numNodes,const char* nameOverride=NULL,SafConfig* cfg=NULL);

// Start this app container running on these nodes (1+N redundancy).  This will
// start a SAF-aware process on each node specified.
// 
// Params:
//   appCnt:    The name of the application container (SAF SG)
//   nodeName:  What node to "extend" this application container on to.
//   compName:  What is the SAF name for the process to be run on that node (must be unique across the cluster)
//   cfg:       Lets you set detailed configuration for this operation (currently just the name of the executed binary can be changed).  Pass NULL for no extended config.
void acExtend(const char* appCnt, const char* nodeName, const char* compName,SafConfig *cfg=NULL);


// Stop this app container from running on these nodes (1+N redundancy)
void acRetract(const char* appCnt, const char* nodeNames[], int numNodes,const char* nameOverride=NULL);

// Stop this app container from running on these nodes (1+N redundancy)
// 
// Params:
//   appCnt:    The name of the application container (SAF SG)
//   nodeName:  What node to "extend" this application container on to.
//   compName:  What is the SAF name for the process to be run on that node (must be unique across the cluster)
void acRetract(const char* appCnt, const char* nodeName,const char* compName);

// Start this app container running on all configured nodes
void acStart(const char* appCnt);

// Stop this app container (and all applications inside it) on all configured nodes.
void acStop(const char* appCnt);

// Add an application to this app container.
// The effect of this is to create a new SAF Service Instance (SI)
// This SI will be assigned to the SG and therefore a process running on the cluster
// This process shall use the information in the SI to start the appropriate
// application running.
void acAddApp(const char* appCnt,const char* appName, const char* activeXml, const char* standbyXml);

// Remove an application from this container.
// The effect of this is to delete the SAF Service Instance that represents this
// application
void acRemoveApp(const char* appCnt, const char* appName);

// test interfaces
void extendApp(const char *app, const char *nodeNames[], int numNodes, 
               const char *nameModifier=NULL, SafConfig *cfg = NULL);

void extendApp(const char *app, const char *node, const char *comp, SafConfig *cfg = NULL);

void createApp(const char *app, SafConfig *cfg=NULL, 
               const char *activecfg=NULL, const char *standbycfg = NULL);

void removeApp(const char *app, const char *nodenames[], int numNodes);

void testCreateApp(const char *app, const char *node1, const char *node2, bool createNode = false);

void testRemoveApp(const char *app, const char *node1, const char *node2);

#endif
