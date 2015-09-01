/* 
  * This file will not be changed once created and is for customers to
  * modify the source code to implement proprietary feature 
  */
#include <clMgtApi.hxx>
#include <UnitTestModule.hxx>
#include <unitTestCommon.hxx>

namespace unitTest
{
  /*
   * Definition of module instance variable
   */
   UnitTestModule unitTestCfg;

  /*
   * Implement application specific initialization here,
   * i.e YANG data, active/standby designations
   */
  void UnitTestInitialize()
  {
    //TODO: Your implementation here to initialize
  }

} /* namespace unitTest */

/*
 * SAFplus plugin architecture
 */
extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  unitTest::unitTestCfg.pluginId         = SAFplus::CL_MGT_MODULE_PLUGIN_ID;
  unitTest::unitTestCfg.pluginVersion    = SAFplus::CL_MGT_MODULE_PLUGIN_VER;

  /*
   * Do the application specific initialization here.
   */
   unitTest::UnitTestInitialize();

  // return module instance when plugin loaded
  return (SAFplus::ClPlugin*) &unitTest::unitTestCfg;
}
