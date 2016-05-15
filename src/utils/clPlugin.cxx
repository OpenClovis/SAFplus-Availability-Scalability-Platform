#include <dlfcn.h>
#include <clPluginApi.hxx>
#define UTL "UTL"
#define PLG "PLG"

namespace SAFplus
  {

void clUnloadPlugin(ClPluginHandle* ph)
{
  ClPluginFinalizeFnType fn;
  fn =  (ClPluginFinalizeFnType) dlsym(ph->dlHandle, CL_PLUGIN_FINALIZE_FN);
  if (fn != NULL)
    {
      fn();
    }
  ph->pluginApi = 0;  // Finalize function should clean this up
  dlclose(ph->dlHandle);
  ph->dlHandle  = 0;
  SAFplusHeapFree(ph); 
}

ClPluginHandle* clLoadPlugin(uint_t pluginId, uint_t  version, const char* name)
{
    ClPluginHandle* ph = (ClPluginHandle*) SAFplusHeapAlloc(sizeof(ClPluginHandle));
    if (!ph)
    {
        clDbgResourceLimitExceeded(clDbgMemoryResource,0, "Failed to allocate [%u] bytes of memory.",(unsigned int) sizeof(ClPluginHandle) );
        goto errorOut;
    }

    ph->dlHandle = dlopen(name,RTLD_GLOBAL|RTLD_NOW);
    if (!ph->dlHandle)
    {
        char* err = dlerror();
        logWrite(LOG_SEV_ERROR, UTL, PLG, "Plugin [%s] error [%s].", name,err);
        goto errorOut;
    }

    *(void**)&ph->init =  dlsym(ph->dlHandle, CL_PLUGIN_INIT_FN);
    if (ph->init == NULL)
    {
        logWrite(LOG_SEV_ERROR, UTL, PLG, "Plugin [%s] does not contain an initialization function [%s].",name,CL_PLUGIN_INIT_FN);
        goto errorOut;
    }

    ph->pluginApi = (ClPlugin*) ph->init(version);
    if (ph->pluginApi == 0)
    {
        logWrite(LOG_SEV_ERROR, UTL, PLG, "Plugin [%s] failed initialization.",name);
        goto errorOut;
    }
    if (ph->pluginApi->pluginId != pluginId)
    {
        logWrite(LOG_SEV_ERROR, UTL, PLG, "Incorrect type of plugin [%s] (type identifiers do not match).  Expected 0x%x got 0x%x.",name,(unsigned int) pluginId, (unsigned int) ph->pluginApi->pluginId);
        goto errorOut;
    }
     if (ph->pluginApi->pluginVersion != version)
    {
        logWrite(LOG_SEV_ERROR, UTL, PLG, "Incompatible version for plugin [%s].  Expected 0x%x got 0x%x.",name,(unsigned int)version, (unsigned int)ph->pluginApi->pluginVersion);
        goto errorOut;
    }

    return ph;
errorOut:
    if (ph) SAFplusHeapFree(ph);
    return NULL;
}


ClPlugin::~ClPlugin() {}

};
