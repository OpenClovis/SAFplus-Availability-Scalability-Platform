/*
 * clAmfNotification.cxx
 *
 *  Created on: Apr 25, 2019
 *      Author: thai.vo
 */
#include <clAmfNotification.hxx>
#include <rpcEvent.hxx>
#include <clLogApi.hxx>
#include <chrono>
using namespace SAFplus;
using namespace std;
SAFplus::EventClient eventClient;
void clAmfClientNotificationInitialize(SAFplus::Handle hdl,EventCallbackFunction cb)
{
	eventClient.eventInitialize(hdl,cb);
	eventClient.eventChannelSubscriber(CL_AMS_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_GLOBAL_CHANNEL);
    eventClient.eventChannelSubscriber(CL_CPM_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_GLOBAL_CHANNEL);
    
}

void clAmfClientNotificationFinalize()
{
	eventClient.eventChannelUnSubscriber(CL_AMS_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_GLOBAL_CHANNEL);  
	eventClient.eventChannelUnSubscriber(CL_CPM_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_GLOBAL_CHANNEL);
}

string createEventNotification(string name,string stateChange,const char* beforeChange,const char* afterChange)
{
	string ret = "";
	ret += name;
	ret += " ";
	ret += stateChange;
	ret += " ";
	ret += beforeChange;
	ret += " ";
	ret += afterChange;
	return ret;
}
