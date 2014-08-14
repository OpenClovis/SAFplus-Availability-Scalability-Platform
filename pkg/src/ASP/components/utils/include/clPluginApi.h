#ifndef CLPLUGINAPI_H
#define CLPLUGINAPI_H

#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

/* You must return this structure from your initialization.  If you do not need a particular function, set it to NULL.
 */
typedef struct
{
    ClWordT pluginId;       /* Set to CL_CM_PLUGIN_ID.  This number ensures that this plugin is for the chassis manager */
    ClWordT pluginVersion;  /* Set to CL_CM_PLUGIN_VERSION */
} ClPlugin;


/* You must define a plugin initialization routine that returns a structure of plugin functions and other info */
extern ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion);

typedef ClPlugin* (*ClPluginInitializeFnType)(ClWordT preferredPluginVersion) ;

#define CL_CM_PLUGIN_INIT_FN "clPluginInitialize"

/* Prepare this plugin to be unloaded and a new version loaded */
extern ClRcT clPluginUpgrade();
#define CL_CM_PLUGIN_UPGRADE_FN "clPluginUpgrade"

/* Prepare this plugin to be unloaded and a new version loaded */
extern ClRcT clPluginFinalize();
#define CL_CM_PLUGIN_FINALIZE_FN "clPluginFinalize"

typedef struct
{
    ClPlugin* pluginApi;
    void*     dlHandle;
    ClPluginInitializeFnType init;    
} ClPluginHandle;

/* If name is NULL search for default names. */
extern ClPluginHandle* clLoadPlugin(ClWordT pluginId,ClWordT preferredPluginVersion,const char* name);

#ifdef __cplusplus
}
#endif    
#endif
