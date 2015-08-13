#include <clPluginApi.hxx>
#include <clFaultApi.hxx>
#include <clMgtApi.hxx>
#include <clAmfApi.hxx>
#include <amfOperations.hxx>

namespace SAFplusAmf
  {
  class SAFplusAmfModule;
  }

namespace SAFplus
  {

  enum
    {
    CL_AMF_POLICY_PLUGIN_ID = 0x53843922,
    CL_AMF_POLICY_PLUGIN_VER = 1
    };

  class ClAmfPolicyPlugin_1:public ClPlugin
    {
  public:
    AmfRedundancyPolicy     policyId;
    SAFplus::AmfOperations* amfOps;   // AMF gives the plugin access to this AMF functionality
    SAFplus::Fault*         fault;    // AMF gives the plugin access to the fault manager

      //? Run an AMF data audit as the active system controller.
    virtual void activeAudit(SAFplusAmf::SAFplusAmfModule* root) = 0;
      //? Run an AMF data audit as the standby system controller.
    virtual void standbyAudit(SAFplusAmf::SAFplusAmfModule* root) = 0;
    //? The AMF will call this function after your plugin is loaded
    virtual bool initialize(SAFplus::AmfOperations* amfOperations,SAFplus::Fault* fault);

    // The copy constructor is disabled to ensure that the only copy of this
    // class exists in the shared memory lib.
    // This will help allow policies to be unloaded and updated by discouraging
    // them from being copied willy-nilly.
    ClAmfPolicyPlugin_1(ClAmfPolicyPlugin_1 const&) = delete; 
    ClAmfPolicyPlugin_1& operator=(ClAmfPolicyPlugin_1 const&) = delete;
  protected:  // Only constructable from your derived class from within the .so
    ClAmfPolicyPlugin_1() {};


    };

  }
