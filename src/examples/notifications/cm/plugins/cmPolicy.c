/*
 * A Chassis manager plugin that can be used by a customer/app to filter out
 * sensor events as some events might not be required to be processed
 * The HpiEventFilter function below for the cmPolicy.so plugin should return
 * CL_ERR_OP_NOT_PERMITTED accordingly when the user wants to skip processing
 * of a sensor event based on the sensor event received from HPI. 
 * CL_OK is returned otherwise for the chassis manager to process the event
 *
 */
#include <clCmPluginApi.h>

// This structure defines the plugin's interface with the program that loads it.
ClCmPluginApi api;

ClRcT HpiEventFilter(SaHpiSessionIdT session, SaHpiEventT* event, SaHpiRdrT* rdr, SaHpiRptEntryT* rpt)
{

    if ((event->EventType==SAHPI_ET_SENSOR)&&(event->Severity==SAHPI_MAJOR)&&(event->Source==0x10)&&(event->EventDataUnion.SensorEvent.SensorNum==4))
    {
        // return CL_ERR_OP_NOT_PERMITTED; // This will skip the event altogether

        // Change the event to be non-service impacting.
        event->Severity=SAHPI_MINOR;        
    }
    
    return CL_OK;
}


extern "C" ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
{
    // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

    // Initialize the pluginData structure
    api.pluginInfo.pluginId         = CL_CM_PLUGIN_ID;
    api.pluginInfo.pluginVersion    = CL_CM_PLUGIN_VERSION;
    api.clHpiEventFilter = HpiEventFilter;
    // return it
    return (ClPlugin*) &api;
}
