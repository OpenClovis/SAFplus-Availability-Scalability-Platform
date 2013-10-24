#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clHeapApi.h>
#include <clEoApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocParseConfig.h>
#include <clIocLogicalAddresses.h>
#include "rmdExternalDefs.h"
#include "clRmdIpi.h"
#include <saEvt.h>
#include <clEventApi.h>
#include <clEventExtApi.h>
#include <clLogApi.h>
// log external
#include "alarmClockLog.h"




//***********External***********************************
#define __LOGICAL_ADDRESS(a) CL_IOC_LOGICAL_ADDRESS_FORM(CL_IOC_STATIC_LOGICAL_ADDRESS_START + (a))
#define __RPC_SERVER_ADDRESS __LOGICAL_ADDRESS(4)
#define LOCAL_ADDRESS 4
extern ClRcT clRmdLibInitialize(ClPtrT pConfig);


//********************publisher********************
#define EVENT_CHANNEL_NAME_1 "TestEventChannel1"
#define PUBLISHER_NAME_1 "TestEventPublisher1"
typedef struct
{
    ClEoExecutionObjT             *tstRegEoObj;
    SaEvtHandleT		          evtInitHandle;
    SaEvtEventHandleT		      eventHandle;
    ClVersionT                    version;
    SaVersionT                    evtVersion;
    SaNameT                       evtChannelName;
    SaNameT                       publisherName;
    int                           running;
    int                           exiting;
} gTestInfoT;
SaAisErrorT openPublisherChannel();
static void testEvtMainLoop();
static gTestInfoT gTestInfo;

//*************************************************


//********************subscribe *******************
#define EVENT_CHANNEL_NAME "TestEventChannel"
#define PUBLISHER_NAME "TestEventPublisher"
SaNameT                 evtChannelName;
SaEvtChannelHandleT   evtChannelHandle = 0;
SaEvtHandleT      evtHandle;
//*************************************************




static void
appEventCallback( SaEvtSubscriptionIdT	subscriptionId,
                             SaEvtEventHandleT     eventHandle,
			     SaSizeT eventDataSize);


int main(int argc, char **argv)
{

    ClEoConfigT eoConfig =
    {
        CL_OSAL_THREAD_PRI_MEDIUM,    /* EO Thread Priority                       */
        2,                            /* No of EO thread needed                   */
        0,                            /* Required Ioc Port                        */
        (CL_EO_USER_CLIENT_ID_START + 0), 
        CL_EO_USE_THREAD_FOR_APP,     /* Thread Model                             */
        NULL,                         /* Application Initialize Callback          */
        NULL,                         /* Application Terminate Callback           */
        NULL,                         /* Application State Change Callback        */
        NULL                          /* Application Health Check Callback        */
    };    
    int ioc_address_local = LOCAL_ADDRESS;
    ClRcT rc = CL_OK;
    rc = clExtInitialize( ioc_address_local );
    if (rc != CL_OK)
    {
        printf("Error: failed to Initialize ASP libraries\n");
        exit(1);
    }
    if ((rc = clRmdLibInitialize(NULL)) != CL_OK)
    {
        printf("Error: RMD initialization failed with rc = 0x%x\n", rc);
        exit(1);
    }
    printf("Info: start rmd server\n");

    rc = clExtRmdServerInit(eoConfig);

    if(rc != CL_OK)
    {
        printf("Info: start rmd server ok\n");
    }
    else
    {
        //open a subscribe event channel and start
        const SaEvtCallbacksT evtCallbacks =
        {
            NULL,
            appEventCallback
        };
        SaVersionT  evtVersion = CL_EVENT_VERSION;
        printf("Event : Initializing and registering with CPM...\n");
        rc = saEvtInitialize(&evtHandle, &evtCallbacks, &evtVersion);
        if (rc != SA_AIS_OK)
        {
            printf("Failed to init event mechanism [0x%x]\n",rc);
            return rc;
        }

        saNameSet(&evtChannelName,EVENT_CHANNEL_NAME);
        rc = saEvtChannelOpen(evtHandle,&evtChannelName,
            (SA_EVT_CHANNEL_SUBSCRIBER |
                SA_EVT_CHANNEL_CREATE),
            (SaTimeT)SA_TIME_END,
            &evtChannelHandle);
        if (rc != SA_AIS_OK)
        {
            printf("Failure opening event channel[0x%x] at %ld\n",
                    rc, time(0L));
            goto errorexit;
        }
        sleep(20);
        printf("EVENT ");
        rc = saEvtEventSubscribe(evtChannelHandle, NULL, 1);
        if (rc != SA_AIS_OK)
        {
            printf("Failed to subscribe to event channel [0x%x]\n",
                        rc);
            goto errorexit;
        }
        //open a global log stream and write several records
        printf("Open a global log stream and write several records\n");        
        alarmClockLogInitialize();
        printf("close a global log stream and write several records\n");        
        //alarmClockLogFinalize();
        printf("open a publisher event channel\n");        
        // open a publisher event channel and 
        openPublisherChannel();
        printf("start public event\n");        
        testEvtMainLoop();
        printf("unsubscript public event\n");        
        rc = saEvtChannelClose(evtChannelHandle);
	if (rc != SA_AIS_OK) 
		printf("Channel unsubscribe result: %d\n", rc);
        testEvtMainLoop();

    }    
    do
    {
        sleep(20);
        printf("Info : running ....");
    }while(1);
    errorexit:
        printf ("Initialization error [0x%x]\n",rc);
}


static void
generate_time_of_day(char **data, ClSizeT *data_len)
{
    time_t t;

    // minimal error checking
    if (data == 0 || data_len == 0)
    {
        printf("generate_time_of_day passed null pointer\n");
        return;
    }

    // magic number, but well, that's what ctime_r needs
    *data_len = 26;
    *data = (char*)clHeapAllocate(*data_len);
    if (*data == 0)
    {
        *data_len = 0;
        return;
    }
    time(&t);
    ctime_r(&t, *data);
    *(*data + 24) = 0;
    (*data_len) -= 1;
    return;
}

static void
generate_load_average(char **data, ClSizeT *data_len)
{
    int fd;
    char *tmp_ptr;
    char buf[500];                  //insane over doing it
    ssize_t num_read;

    // minimal error checking
    if (data == 0 || data_len == 0)
    {
        printf(
                "generate_load_average passed null pointer\n ");
        return;
    }
    if ((fd = open("/proc/loadavg", O_RDONLY, 0)) == -1)
    {
        printf( "failed to open /proc/loadavg\n");
        return;
    }
    num_read = read(fd, buf, sizeof buf);
    if (num_read == 0 || num_read == -1)
    {
        printf( "bogus result from read of loadavg\n");
        return;
    }
    close(fd);
    *data_len = num_read + 1;
    *data = (char*)clHeapAllocate(*data_len);
    if (data == 0)
    {
        printf(
                "failed to allocate memory for loadavg contents\n");
        *data_len = 0;
        close(fd);
        return;
    }
    *(*data + (*data_len) - 1) = 0;     // preemptively null-terminate the line
    strncpy(*data, buf, *data_len);

    //
    // Do MINIMAL parsing in that we look for the third space in the buffer
    // (which comes after the load average information proper) and we replace
    // the space with a nul character to terminate the string.
    // If there is no third space character, just return the buffer unchanged.
    tmp_ptr = strchr(*data, ' ');
    if (tmp_ptr == 0)
    {
        return;
    }
    tmp_ptr = strchr(tmp_ptr + 1, ' ');
    if (tmp_ptr == 0)
    {
        return;
    }
    tmp_ptr = strchr(tmp_ptr + 1, ' ');
    if (tmp_ptr == 0)
    {
        return;
    }
    *tmp_ptr = 0;
    return;
}
static ClRcT
appPublishEvent()
{
    ClEventIdT      eventId         = 0;
    static int      index           = 0;
    SaSizeT         data_len        = 0;
    SaAisErrorT	    saRc = SA_AIS_OK;
    char            *data           = 0;
    typedef void (*Generator)(char **, ClSizeT*);

    //
    // Note: to add a new generator, just define it above and then include
    // the new functions name in the generators list.
    // Next, maybe something that gets disk free info by way of getfsent
    // and statfs?
    static Generator generators[]   =
    {
        generate_time_of_day,
        generate_load_average
    };

    //
    // every time through increment index and then set index to
    // it's value modulo the number of entries in the generators
    // array.  This will cause us to cycle through the list of
    // generators as we're called to publish events.
    (*generators[index++])(&data, &data_len);
    index %= (int)(sizeof generators / sizeof generators[0]);
    if (data == 0 || data_len == 0)
    {
        printf("no event data generated\n");
        return CL_ERR_NO_MEMORY;
    }
    printf("Publishing Event: %.*s\n", (int)data_len, data);
    saRc = saEvtEventPublish(gTestInfo.eventHandle, (void *)data, data_len, &eventId);
    clHeapFree(data);

    return CL_OK;
}


static void
testEvtMainLoop()
{
    /* Main loop: Keep printing and publishing unless we are suspended */
    int i=0;
    while (1)
    {
        appPublishEvent();        
        sleep(10);
        i++;
        if(i==10)
        {
            return;
        }
    }
}    

static void
appEventCallback( SaEvtSubscriptionIdT	subscriptionId,
                             SaEvtEventHandleT     eventHandle,
			     SaSizeT eventDataSize)
{
    SaAisErrorT  saRc = SA_AIS_OK;
    static ClPtrT   resTest = 0;
    static ClSizeT  resSize = 0;
    printf ("We've got an event to receive\n");
    if (resTest != 0)
    {
        // Maybe try to save the previously allocated buffer if it's big
        // enough to hold the new event message.
        clHeapFree((char *)resTest);
        resTest = 0;
        resSize = 0;
    }
    resTest = clHeapAllocate(eventDataSize + 1);
    if (resTest == 0)
    {
        printf("Failed to allocate space for event\n");
        return;
    }
    *(((char *)resTest) + eventDataSize) = 0;
    resSize = eventDataSize;
    saRc = saEvtEventDataGet(eventHandle, resTest, &resSize);
    if (saRc!= SA_AIS_OK)
    {
        printf("Failed to get event data [0x%x]\n",saRc);
    }
    printf ("received event: %s\n", (char *)resTest);
    return;
}

SaAisErrorT openPublisherChannel()
{
        SaAisErrorT  rc = SA_AIS_OK;
        gTestInfo.tstRegEoObj      = 0;
  	gTestInfo.evtInitHandle    = 0;
  	gTestInfo.eventHandle      = 0;
        gTestInfo.version.releaseCode                    = 'B';
        gTestInfo.version.majorVersion                   = 01;
        gTestInfo.version.minorVersion                   = 01;
        gTestInfo.evtVersion.releaseCode                    = 'B';
        gTestInfo.evtVersion.majorVersion                   = 01;
        gTestInfo.evtVersion.minorVersion                   = 01;
        saNameSet(&gTestInfo.evtChannelName,EVENT_CHANNEL_NAME_1);
        saNameSet(&gTestInfo.publisherName,PUBLISHER_NAME_1);
        gTestInfo.running          = 1;
        gTestInfo.exiting          = 0;
        SaEvtChannelHandleT evtChannelHandle      = 0;
        SaEvtCallbacksT     evtCallbacks          = {NULL, NULL};
        sleep(10);
        printf("initial event \n");
        rc = saEvtInitialize(&gTestInfo.evtInitHandle,
                       &evtCallbacks,
                       &gTestInfo.evtVersion);
        if (rc != SA_AIS_OK)
        {
            printf( "Failed to init event system[0x%x]\n",rc);
            return rc;
        }
        rc = saEvtChannelOpen (gTestInfo.evtInitHandle,
                             &gTestInfo.evtChannelName,
                            (SA_EVT_CHANNEL_PUBLISHER |
                             SA_EVT_CHANNEL_CREATE),
                             (ClTimeT)SA_TIME_END,
                             &evtChannelHandle);
        if (rc != SA_AIS_OK)
        {
            printf( "Failed to open event channel [0x%x]\n",rc);
            return rc;
        }

        rc = saEvtEventAllocate(evtChannelHandle, &gTestInfo.eventHandle);
        if (rc != SA_AIS_OK)
        {
            printf( "Failed to cllocate event [0x%x]\n",rc);
            return rc;
        }

        rc = saEvtEventAttributesSet(gTestInfo.eventHandle,
                NULL,
                1,
                0,
                &gTestInfo.publisherName);
        if (rc != SA_AIS_OK)
        {
            printf( "Failed to set event attributes [0x%x]\n",rc);
            return rc;
        }
        printf( "start publish event");
        return rc;
}

