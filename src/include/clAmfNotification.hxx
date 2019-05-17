/*
 * clAmfNotification.hxx
 *
 *  Created on: Apr 25, 2019
 *      Author: thai.vo
 */

#ifndef SRC_INCLUDE_CLAMFNOTIFICATION_HXX_
#define SRC_INCLUDE_CLAMFNOTIFICATION_HXX_

#include <string>
#include <EventClient.hxx>
#include <EventChannel.hxx>
#include <clCommon6.h>
#include <clLogApi.hxx>
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>


using namespace SAFplusI;
using namespace SAFplus;
using namespace std;
#define CL_AMS_EVENT_CHANNEL_NAME	"AMS_EVENT_NOTIFICATION_CHANNEL"
#define CL_CPM_EVENT_CHANNEL_NAME	"CPM_EVENT_NOTIFICATION_CHANNEL"
void clAmfClientNotificationInitialize(SAFplus::Handle hdl,EventCallbackFunction cb);
void clAmfClientNotificationFinalize();
string createEventNotification(string name,string stateChange,const char* beforeChange,const char* afterChange);

#endif /* SRC_INCLUDE_CLAMFNOTIFICATION_HXX_ */
