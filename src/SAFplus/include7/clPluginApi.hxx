#pragma once
#ifndef CLPLUGINAPI_H
#define CLPLUGINAPI_H

#include <clCommon.hxx>

namespace SAFplus
{
    
/* You must return this structure from your initialization.  If you do not need a particular function, set it to NULL.  Derive or otherwise extend this structure with your APIs
 */
class ClPlugin
{
public:
  uint_t pluginId;       /* Create a unique number for your plugin target so a plugin for one application  cannot be accidentally loaded by another. */
  uint_t pluginVersion;  /* Plugin format.  Set to CL_PLUGIN_VERSION */

  virtual ~ClPlugin()=0;
};


/* You must define a plugin initialization routine that returns a structure of plugin functions and other info */
extern ClPlugin* clPluginInitialize(uint_t preferredPluginVersion);

typedef ClPlugin* (*ClPluginInitializeFnType)(uint_t preferredPluginVersion) ;

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
extern ClPluginHandle* clLoadPlugin(uint_t pluginId,uint_t preferredPluginVersion,const char* name);

};

#endif
