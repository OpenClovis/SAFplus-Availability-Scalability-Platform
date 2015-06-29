#include <dlfcn.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clPluginApi.h>
#define UTL "UTL"
#define PLG "PLG"

ClPluginHandle* clLoadPlugin(ClWordT pluginId, ClWordT version, const char* name)
{
    ClPluginHandle* ph = clHeapAllocate(sizeof(ClPluginHandle));
    if (!ph)
    {
        clDbgResourceLimitExceeded(clDbgMemoryResource,0,("Failed to allocate [%u] bytes of memory.",(unsigned int) sizeof(ClPluginHandle)));
        goto errorOut;        
    }
    
    ph->dlHandle = dlopen(name,RTLD_GLOBAL|RTLD_NOW);
    if (!ph->dlHandle)
    {
        clLog(CL_LOG_DEBUG, UTL, PLG, "Plugin [%s] was not found.", name);
        goto errorOut;        
    }
    
    *(void**)&ph->init =  dlsym(ph->dlHandle,CL_CM_PLUGIN_INIT_FN);
    if (ph->init == NULL)
    {
        clLog(CL_LOG_ERROR, UTL, PLG, "Plugin [%s] does not contain an initialization function [%s].",name,CL_CM_PLUGIN_INIT_FN);
        goto errorOut;        
    }

    ph->pluginApi = (ClPlugin*) ph->init(version);
    if (ph->pluginApi == 0)
    {
        clLog(CL_LOG_ERROR, UTL, PLG, "Plugin [%s] failed initialization.",name);
        goto errorOut;        
    }
    if (ph->pluginApi->pluginId != pluginId)
    {
        clLog(CL_LOG_ERROR, UTL, PLG, "Incorrect type of plugin [%s] (type identifiers do not match).  Expected 0x%x got 0x%x.",name,(unsigned int) pluginId, (unsigned int) ph->pluginApi->pluginId);
        goto errorOut;
    }
     if (ph->pluginApi->pluginVersion != version)
    {
        clLog(CL_LOG_ERROR, UTL, PLG, "Incompatible version for plugin [%s].  Expected 0x%x got 0x%x.",name,(unsigned int)version, (unsigned int)ph->pluginApi->pluginVersion);
        goto errorOut;
    }
   

    return ph;
errorOut:
    if (ph) clHeapFree(ph);
    return NULL;
}
