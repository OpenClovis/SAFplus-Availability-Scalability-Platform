#pragma once
#ifndef CLPLUGINAPI_H
#define CLPLUGINAPI_H

#include <clCommon.h>

namespace SAFplus
{
    
/* You must return this structure from your initialization.  If you do not need a particular function, set it to NULL.  Derive or otherwise extend this structure with your APIs
 */
class ClPlugin
{
public:
  ClWordT pluginId;       /* Set to CL_CM_PLUGIN_ID.  This number ensures that this plugin is for the chassis manager */
  ClWordT pluginVersion;  /* Set to CL_CM_PLUGIN_VERSION */

  virtual ~ClPlugin()=0;
};


/* You must define a plugin initialization routine that returns a structure of plugin functions and other info */
extern ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion);

typedef ClPlugin* (*ClPluginInitializeFnType)(ClWordT preferredPluginVersion) ;

#define CL_PLUGIN_INIT_FN "clPluginInitialize"

/* Prepare this plugin to be unloaded and a new version loaded */
extern ClRcT clPluginUpgrade();
#define CL_PLUGIN_UPGRADE_FN "clPluginUpgrade"

/* Prepare this plugin to be unloaded and a new version loaded */
extern ClRcT clPluginFinalize();
#define CL_PLUGIN_FINALIZE_FN "clPluginFinalize"

typedef struct
{
    ClPlugin* pluginApi;
    void*     dlHandle;
    ClPluginInitializeFnType init;    
} ClPluginHandle;

/* If name is NULL search for default names. */
extern ClPluginHandle* clLoadPlugin(ClWordT pluginId,ClWordT preferredPluginVersion,const char* name);

};

#endif
