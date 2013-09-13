/*
 * A Chassis manager event channel subscriber example.
 * Listens for Chassis manager sensor and fru-state transition events 
 * published on the event channel for chassis manager.
 * Requires the installation of SAFplus-Platform-Support package
 * 
 * cmNotificationInitialize function below subscribes to the CM event channel
 * cmNotificationFinalize function below unsubscribes/finalizes the CM event channel
 */

#include <clCommon.h>
#include <clLogApi.h>
#include <clEventApi.h>
#include <clAlarmApi.h>
#include <clCmApi.h>

#define CM_SUBSCRIPTION_ID	1

static SaNameT gCmEvtChannelName = {
    .length = sizeof(CL_CM_EVENT_CHANNEL)-1,
    .value = CL_CM_EVENT_CHANNEL
};

static ClEventInitHandleT      gCmEvtHandle = 0;
static ClEventChannelHandleT   gCmEvtChannelHandle = 0;

static void
cmEventCallback(ClEventSubscriptionIdT subscriptionId,
                const ClEventHandleT eventHandle,
                const ClSizeT eventDataSize);

static ClEventCallbacksT cmEvtCallbacks = {
    NULL,
    cmEventCallback
};

#define MAXBUF 128

typedef struct __StrTable {
    int num;
    const char *str;
} StrTable;


/* HPI Bool Type Table */
static const StrTable boolTypeStrings[] = {
    {SAHPI_FALSE,  "False"},
    {SAHPI_TRUE,   "True"},
    { 0, NULL }
};

/* HPI Event Type Table */
static const StrTable eventTypeStrings[] = {
    {SAHPI_ET_RESOURCE,             "SAHPI_ET_RESOURCE"},
    {SAHPI_ET_DOMAIN,               "SAHPI_ET_DOMAIN"},
    {SAHPI_ET_SENSOR,               "SAHPI_ET_SENSOR"},
    {SAHPI_ET_SENSOR_ENABLE_CHANGE, "SAHPI_ET_SENSOR_ENABLE_CHANGE"},
    {SAHPI_ET_HOTSWAP,              "SAHPI_ET_HOTSWAP"},
    {SAHPI_ET_WATCHDOG,             "SAHPI_ET_WATCHDOG"},
    {SAHPI_ET_HPI_SW,               "SAHPI_ET_HPI_SW"},
    {SAHPI_ET_OEM,                  "SAHPI_ET_OEM"},
    {SAHPI_ET_USER,                 "SAHPI_ET_USER"},
    { 0, NULL }
};

/* HPI Sensor Type Table */
static const StrTable sensorTypeStrings[] = {
    {SAHPI_TEMPERATURE,             "SAHPI_TEMPERATURE"},
    {SAHPI_VOLTAGE,                 "SAHPI_VOLTAGE"},
    {SAHPI_CURRENT,                 "SAHPI_CURRENT"},
    {SAHPI_FAN,                     "SAHPI_FAN"},
    {SAHPI_PHYSICAL_SECURITY,       "SAHPI_PHYSICAL_SECURITY"},
    {SAHPI_PLATFORM_VIOLATION,      "SAHPI_PLATFORM_VIOLATION"},
    {SAHPI_PROCESSOR,               "SAHPI_PROCESSOR"},
    {SAHPI_POWER_SUPPLY,            "SAHPI_POWER_SUPPLY"},
    {SAHPI_POWER_UNIT,              "SAHPI_POWER_UNIT"},
    {SAHPI_COOLING_DEVICE,          "SAHPI_COOLING_DEVICE"},
    {SAHPI_OTHER_UNITS_BASED_SENSOR, "SAHPI_OTHER_UNITS_BASED_SENSOR"},
    {SAHPI_MEMORY,                  "SAHPI_MEMORY"},
    {SAHPI_DRIVE_SLOT,              "SAHPI_DRIVE_SLOT"},
    {SAHPI_POST_MEMORY_RESIZE,      "SAHPI_POST_MEMORY_RESIZE"},
    {SAHPI_SYSTEM_FW_PROGRESS,      "SAHPI_SYSTEM_FW_PROGRESS"},
    {SAHPI_EVENT_LOGGING_DISABLED,  "SAHPI_EVENT_LOGGING_DISABLED"},
    {SAHPI_RESERVED1,               "SAHPI_RESERVED1"},
    {SAHPI_SYSTEM_EVENT,            "SAHPI_SYSTEM_EVENT"},
    {SAHPI_CRITICAL_INTERRUPT,      "SAHPI_CRITICAL_INTERRUPT"},
    {SAHPI_BUTTON,                  "SAHPI_BUTTON"},
    {SAHPI_MODULE_BOARD,            "SAHPI_MODULE_BOARD"},
    {SAHPI_MICROCONTROLLER_COPROCESSOR, "SAHPI_MICROCONTROLLER_COPROCESSOR"},
    {SAHPI_ADDIN_CARD,              "SAHPI_ADDIN_CARD"},
    {SAHPI_CHASSIS,                 "SAHPI_CHASSIS"},
    {SAHPI_CHIP_SET,                "SAHPI_CHIP_SET"},
    {SAHPI_OTHER_FRU,               "SAHPI_OTHER_FRU"},
    {SAHPI_CABLE_INTERCONNECT,      "SAHPI_CABLE_INTERCONNECT"},
    {SAHPI_TERMINATOR,              "SAHPI_TERMINATOR"},
    {SAHPI_SYSTEM_BOOT_INITIATED,   "SAHPI_SYSTEM_BOOT_INITIATED"},
    {SAHPI_BOOT_ERROR,              "SAHPI_BOOT_ERROR"},
    {SAHPI_OS_BOOT,                 "SAHPI_OS_BOOT"},
    {SAHPI_OS_CRITICAL_STOP,        "SAHPI_OS_CRITICAL_STOP"},
    {SAHPI_SLOT_CONNECTOR,          "SAHPI_SLOT_CONNECTOR"},
    {SAHPI_SYSTEM_ACPI_POWER_STATE, "SAHPI_SYSTEM_ACPI_POWER_STATE"},
    {SAHPI_RESERVED2,               "SAHPI_RESERVED2"},
    {SAHPI_PLATFORM_ALERT,          "SAHPI_PLATFORM_ALERT"},
    {SAHPI_ENTITY_PRESENCE,         "SAHPI_ENTITY_PRESENCE"},
    {SAHPI_MONITOR_ASIC_IC,         "SAHPI_MONITOR_ASIC_IC"},
    {SAHPI_LAN,                     "SAHPI_LAN"},
    {SAHPI_MANAGEMENT_SUBSYSTEM_HEALTH, "SAHPI_MANAGEMENT_SUBSYSTEM_HEALTH"},
    {SAHPI_BATTERY,                 "SAHPI_BATTERY"},
    {SAHPI_OPERATIONAL,             "SAHPI_OPERATIONAL"},
    {SAHPI_OEM_SENSOR,               "SAHPI_OEM_SENSOR"},
    { 0, NULL }
};

/* HPI Event Category Table */
static const StrTable eventCategoryStrings[] = {
    {SAHPI_EC_UNSPECIFIED, "SAHPI_EC_UNSPECIFIED"},
    {SAHPI_EC_THRESHOLD, "SAHPI_EC_THRESHOLD"},
    {SAHPI_EC_USAGE, "SAHPI_EC_USAGE"},
    {SAHPI_EC_STATE, "SAHPI_EC_STATE"},
    {SAHPI_EC_PRED_FAIL, "SAHPI_EC_PRED_FAIL"},
    {SAHPI_EC_LIMIT, "SAHPI_EC_LIMIT"},
    {SAHPI_EC_PERFORMANCE, "SAHPI_EC_PERFORMANCE"},
    {SAHPI_EC_SEVERITY, "SAHPI_EC_SEVERITY"},
    {SAHPI_EC_PRESENCE, "SAHPI_EC_PRESENCE"},
    {SAHPI_EC_ENABLE, "SAHPI_EC_ENABLE"},
    {SAHPI_EC_AVAILABILITY, "SAHPI_EC_AVAILABILITY"},
    {SAHPI_EC_REDUNDANCY, "SAHPI_EC_REDUNDANCY"},
    {SAHPI_EC_SENSOR_SPECIFIC, "SAHPI_EC_SENSOR_SPECIFIC"},
    {SAHPI_EC_GENERIC, "SAHPI_EC_GENERIC"},
    { 0, NULL }
};

/* HPI Event Threshold State Table */
static const StrTable thresholdStateStrings[] = {
    {SAHPI_ES_LOWER_MINOR,     "Lower Minor"},
    {SAHPI_ES_LOWER_MAJOR,     "Lower Major"},
    {SAHPI_ES_LOWER_CRIT,      "Lower Critical"},
    {SAHPI_ES_UPPER_MINOR,     "Upper Minor"},
    {SAHPI_ES_UPPER_MAJOR,     "Upper Major"},
    {SAHPI_ES_UPPER_CRIT,      "Upper Critical"},
    { 0, NULL }
};

static const StrTable hotswapStateStrings[] = {
    {SAHPI_HS_STATE_INACTIVE,           "Inactive"},
    {SAHPI_HS_STATE_INSERTION_PENDING,  "Insertion pending"},
    {SAHPI_HS_STATE_ACTIVE,             "Active"},
    {SAHPI_HS_STATE_EXTRACTION_PENDING, "Extraction pending"},
    {SAHPI_HS_STATE_NOT_PRESENT,        "Not present"},
    { 0, NULL }
};


/*------------------------------------------------------------------------------
  - Function:     lookupString
  - Abstract:     Convert enum to string
  - Parameters:   num - [in] enum value
  -               p - [in] pointer to Enum String Table
  - Return Value: char * - converted string
  - Remarks:      None.
  ----------------------------------------------------------------------------*/
static const char *lookupString(int num, const StrTable *p)
{
    static char buf[MAXBUF];

    while (p->str)
    {
        if(num == p->num)
            return p->str;
        p++;
    }

    /* no match */
    snprintf(buf, sizeof(buf), "Enum %d not found", num);

    return buf;
}

/*------------------------------------------------------------------------------
  - Function:     sensorReadingToString
  - Abstract:     Convert a Sensor Reading to a string
  - Parameters:   buf - [in] pointer to a thread safe string buffer
  -               p - [in] pointer to Sensor reading struct.
  - Return Value: char * - converted string.
  - Remarks:      None.
  ----------------------------------------------------------------------------*/
static char *sensorReadingToString(char *buf, SaHpiSensorReadingT *p)
{
    /* validate parameters */
    if(p == NULL)
    {
        /* invalid reading struct */
        buf[0] = 0;
        strncat(buf, "Error", CL_MIN(5, MAXBUF-1));
    }
    else
    {
        switch(p->Type)
        {
        case SAHPI_SENSOR_READING_TYPE_INT64:
            snprintf(buf, MAXBUF, "%lld", p->Value.SensorInt64);
            break;

        case SAHPI_SENSOR_READING_TYPE_UINT64:
            snprintf(buf, MAXBUF, "0x%llx", p->Value.SensorUint64);
            break;

        case SAHPI_SENSOR_READING_TYPE_FLOAT64:
            snprintf(buf, MAXBUF, "%lf", p->Value.SensorFloat64);
            break;
        case SAHPI_SENSOR_READING_TYPE_BUFFER:
            snprintf(buf, MAXBUF, "%.*s", SAHPI_SENSOR_BUFFER_LENGTH, p->Value.SensorBuffer);
            break;

        default:
            buf[0] = 0;
            strncat(buf, "Illegal type", CL_MIN(strlen("illegal type"), MAXBUF-1));
            break;
        }
    }

    return buf;
}

static void printEventState(SaHpiEventCategoryT category, SaHpiEventStateT state)
{
    const char *s = "";

    if (state == SAHPI_ES_UNSPECIFIED)
    {
        s = "SAHPI_ES_UNSPECIFIED";
    }

    switch (category)
    {
        case SAHPI_EC_THRESHOLD:
        {
            switch(state)
            {
                case SAHPI_ES_LOWER_MINOR: s = "SAHPI_ES_LOWER_MINOR"; break;
                case SAHPI_ES_LOWER_MAJOR: s = "SAHPI_ES_LOWER_MAJOR"; break;
                case SAHPI_ES_LOWER_CRIT: s = "SAHPI_ES_LOWER_CRIT"; break;
                case SAHPI_ES_UPPER_MINOR: s = "SAHPI_ES_UPPER_MINOR"; break;
                case SAHPI_ES_UPPER_MAJOR: s = "SAHPI_ES_UPPER_MAJOR"; break;
                case SAHPI_ES_UPPER_CRIT: s = "SAHPI_ES_UPPER_CRIT"; break;
            }
            break;
        }
        case SAHPI_EC_USAGE:
        {
            switch(state)
            {
                case SAHPI_ES_IDLE: s = "SAHPI_ES_IDLE"; break;
                case SAHPI_ES_ACTIVE: s = "SAHPI_ES_ACTIVE"; break;
                case SAHPI_ES_BUSY: s = "SAHPI_ES_BUSY"; break;
            }
            break;
        }
        case SAHPI_EC_STATE:
        {
            switch(state)
            {
                case SAHPI_ES_STATE_DEASSERTED: s = "SAHPI_ES_STATE_DEASSERTED"; break;
                case SAHPI_ES_STATE_ASSERTED: s = "SAHPI_ES_STATE_ASSERTED"; break;
            }
            break;
        }
        case SAHPI_EC_PRED_FAIL:
        {
            switch(state)
            {
                case SAHPI_ES_PRED_FAILURE_DEASSERT: 
                    s = "SAHPI_ES_PRED_FAILURE_DEASSERT";
                    break;
                case SAHPI_ES_PRED_FAILURE_ASSERT:
                    s = "SAHPI_ES_PRED_FAILURE_ASSERT";
                    break;
            }
            break;
        }
        case SAHPI_EC_LIMIT:
        {
            switch(state)
            {
                case SAHPI_ES_LIMIT_NOT_EXCEEDED: s = "SAHPI_ES_LIMIT_NOT_EXCEEDED"; break;
                case SAHPI_ES_LIMIT_EXCEEDED: s = "SAHPI_ES_LIMIT_EXCEEDED"; break;
            }
            break;
        }
        case SAHPI_EC_PERFORMANCE:
        {
            switch(state)
            {
                case SAHPI_ES_PERFORMANCE_MET: s = "SAHPI_ES_PERFORMANCE_MET"; break;
                case SAHPI_ES_PERFORMANCE_LAGS: s = "SAHPI_ES_PERFORMANCE_LAGS"; break;
            }
            break;
        }
        case SAHPI_EC_SEVERITY:
        {
            switch(state)
            {
                case SAHPI_ES_OK: s = "SAHPI_ES_OK"; break;
                case SAHPI_ES_MINOR_FROM_OK: s = "SAHPI_ES_MINOR_FROM_OK"; break;
                case SAHPI_ES_MAJOR_FROM_LESS: s = "SAHPI_ES_MAJOR_FROM_LESS"; break;
                case SAHPI_ES_CRITICAL_FROM_LESS: s = "SAHPI_ES_CRITICAL_FROM_LESS"; break;
                case SAHPI_ES_MINOR_FROM_MORE: s = "SAHPI_ES_MINOR_FROM_MORE"; break;
                case SAHPI_ES_MAJOR_FROM_CRITICAL: s = "SAHPI_ES_MAJOR_FROM_CRITICAL"; break;
                case SAHPI_ES_CRITICAL: s = "SAHPI_ES_CRITICAL"; break;
                case SAHPI_ES_MONITOR: s = "SAHPI_ES_MONITOR"; break;
                case SAHPI_ES_INFORMATIONAL: s = "SAHPI_ES_INFORMATIONAL"; break;
            }
            break;
        }
        case SAHPI_EC_PRESENCE:
        {
            switch(state)
            {
                case SAHPI_ES_ABSENT: s = "SAHPI_ES_ABSENT"; break;
                case SAHPI_ES_PRESENT: s = "SAHPI_ES_PRESENT"; break;
            }
            break;
        }
        case SAHPI_EC_ENABLE:
        {
            switch(state)
            {
                case SAHPI_ES_DISABLED: s = "SAHPI_ES_DISABLED"; break;
                case SAHPI_ES_ENABLED: s = "SAHPI_ES_ENABLED"; break;
            }
            break;
        }

        case SAHPI_EC_AVAILABILITY:
        {
            switch(state)
            {
                case SAHPI_ES_RUNNING: s = "SAHPI_ES_RUNNING"; break;
                case SAHPI_ES_TEST: s = "SAHPI_ES_TEST"; break;
                case SAHPI_ES_POWER_OFF: s = "SAHPI_ES_POWER_OFF"; break;
                case SAHPI_ES_ON_LINE: s = "SAHPI_ES_ON_LINE"; break;
                case SAHPI_ES_OFF_LINE: s = "SAHPI_ES_OFF_LINE"; break;
                case SAHPI_ES_OFF_DUTY: s = "SAHPI_ES_OFF_DUTY"; break;
                case SAHPI_ES_DEGRADED: s = "SAHPI_ES_DEGRADED"; break;
                case SAHPI_ES_POWER_SAVE: s = "SAHPI_ES_POWER_SAVE"; break;
                case SAHPI_ES_INSTALL_ERROR: s = "SAHPI_ES_INSTALL_ERROR"; break;
            }
            break;
        }
        case SAHPI_EC_REDUNDANCY:
        {
            switch(state)
            {
                case SAHPI_ES_FULLY_REDUNDANT: s = "SAHPI_ES_FULLY_REDUNDANT"; break;
                case SAHPI_ES_REDUNDANCY_LOST: s = "SAHPI_ES_REDUNDANCY_LOST"; break;
                case SAHPI_ES_REDUNDANCY_DEGRADED: s = "SAHPI_ES_REDUNDANCY_DEGRADED"; break;
                case SAHPI_ES_REDUNDANCY_LOST_SUFFICIENT_RESOURCES:
                      s = "SAHPI_ES_REDUNDANCY_LOST_SUFFICIENT_RESOURCES"; break;
                case SAHPI_ES_NON_REDUNDANT_SUFFICIENT_RESOURCES:
                    s = "SAHPI_ES_NON_REDUNDANT_SUFFICIENT_RESOURCES"; break;
                case SAHPI_ES_NON_REDUNDANT_INSUFFICIENT_RESOURCES:
                    s = "SAHPI_ES_NON_REDUNDANT_INSUFFICIENT_RESOURCES"; break;
                case SAHPI_ES_REDUNDANCY_DEGRADED_FROM_FULL:
                    s = "SAHPI_ES_REDUNDANCY_DEGRADED_FROM_FULL"; break;
                case SAHPI_ES_REDUNDANCY_DEGRADED_FROM_NON:
                    s = "SAHPI_ES_REDUNDANCY_DEGRADED_FROM_NON"; break;
            }
            break;
        }

        case SAHPI_EC_GENERIC:
        case SAHPI_EC_SENSOR_SPECIFIC:
        {
            switch (state)
            {
                case SAHPI_ES_STATE_00: s = "SAHPI_ES_STATE_00"; break;
                case SAHPI_ES_STATE_01: s = "SAHPI_ES_STATE_01"; break;
                case SAHPI_ES_STATE_02: s = "SAHPI_ES_STATE_02"; break;
                case SAHPI_ES_STATE_03: s = "SAHPI_ES_STATE_03"; break;
                case SAHPI_ES_STATE_04: s = "SAHPI_ES_STATE_04"; break;
                case SAHPI_ES_STATE_05: s = "SAHPI_ES_STATE_05"; break;
                case SAHPI_ES_STATE_06: s = "SAHPI_ES_STATE_06"; break;
                case SAHPI_ES_STATE_07: s = "SAHPI_ES_STATE_07"; break;
                case SAHPI_ES_STATE_08: s = "SAHPI_ES_STATE_08"; break;
                case SAHPI_ES_STATE_09: s = "SAHPI_ES_STATE_09"; break;
                case SAHPI_ES_STATE_10: s = "SAHPI_ES_STATE_10"; break;
                case SAHPI_ES_STATE_11: s = "SAHPI_ES_STATE_11"; break;
                case SAHPI_ES_STATE_12: s = "SAHPI_ES_STATE_12"; break;
                case SAHPI_ES_STATE_13: s = "SAHPI_ES_STATE_13"; break;
                case SAHPI_ES_STATE_14: s = "SAHPI_ES_STATE_14"; break;
            }
            break;
        }
    }
    clLogNotice("EVT", "HPI",
        "        EventState : %s\n", s);
}

static void
cmEventCallback (ClEventSubscriptionIdT subscriptionId,
                 ClEventHandleT eventHandle,
                 ClSizeT eventDataSize)
{
    ClRcT rc = CL_OK;
    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    SaNameT publisherName = { 0 };
    ClEventIdT eventId = 0;
    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    ClPtrT  resTest = NULL;
    ClSizeT resSize;
    ClInt32T i;

    resTest = clHeapCalloc(1, eventDataSize + 1);
    if (resTest == NULL)
    {
        clLogError("EVT","HPI","Failed to allocate space for event");
        goto out_free;
    }
    resSize = eventDataSize;
    rc = clEventDataGet(eventHandle, resTest, &resSize);
    if (rc != CL_OK)
    {
        clLogError("EVT", "HPI", "Event data get failed with [%#x]", rc);
        goto out_free;
    }

    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
                              &retentionTime, &publisherName, 
                              &publishTime, &eventId);

    clLogMultiline(CL_LOG_NOTICE, "EVT","HPI",
                   "-------------------------------------------------------\n"
                   "!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n"
                   "-------------------------------------------------------\n"
                   "             Subscription ID   : %#X\n"
                   "             Event Priority    : %#X\n"
                   "             Retention Time    : 0x%llX\n"
                   "             Publisher Name    : [%.*s]\n"
                   "             EventID           : %#llX\n",
                   subscriptionId,
                   priority,
                   retentionTime,
                   publisherName.length, publisherName.value,
                   eventId);

    if (resSize > 0)
    {
        ClAlarmInfoPtrT pAlarmPayload;
        ClCmFruEventInfoT *pFruEventPayload;
       
        if(!strncmp(CL_CM_FRU_STATE_TRANSITION_EVENT_STR,
                    (ClCharT *)patternArray.pPatterns[0].pPattern,
                    sizeof(CL_CM_FRU_STATE_TRANSITION_EVENT_STR)-1))
        {
            pFruEventPayload = (ClCmFruEventInfoT*)resTest;
      
            clLogMultiline(CL_LOG_NOTICE, "EVT","HPI",
                           "             FruId   : [%d]\n"
                           "             physicalSlot : [%d]\n"
                           "             previousState : [%s]\n"
                           "             presentState : [%s]\n"
                           "-------------------------------------------------------\n",
                           pFruEventPayload->fruId,
                           pFruEventPayload->physicalSlot,
                           lookupString(pFruEventPayload->previousState, hotswapStateStrings),
                           lookupString(pFruEventPayload->presentState, hotswapStateStrings));
        } 
        else if(!strncmp(CL_CM_ALARM_EVENT_STR, 
                         (ClCharT*)patternArray.pPatterns[0].pPattern,
                         sizeof(CL_CM_ALARM_EVENT_STR)-1))
        {
            pAlarmPayload = (ClAlarmInfoPtrT)resTest;
            clLogMultiline(CL_LOG_NOTICE, "EVT","HPI",
                           "The component which has raised the alarm [%.*s]\n"
                           "Category :%d\n"
                           "Probable Cause:%d\n"
                           "Alarm State:%d\n"
                           "Payload len:%d\n",
                           pAlarmPayload->compName.length,
                           pAlarmPayload->compName.value,
                           pAlarmPayload->category,
                           pAlarmPayload->probCause,
                           pAlarmPayload->alarmState,
                           pAlarmPayload->len);
            if (pAlarmPayload->len == (ClUint32T)sizeof(ClCmSensorEventInfoT))
            {
                ClCmSensorEventInfoT *pSensorEventInfo = (ClCmSensorEventInfoT*)
                    pAlarmPayload->buff;
                SaHpiSensorEventT *pSensorEvent = &pSensorEventInfo->sensorEvent;
                char tmpBuf[MAXBUF];

                clLogMultiline(CL_LOG_NOTICE, "EVT","HPI",
                               "       SensorNum : %d (0x%x)\n"
                               "       SensorType: %s\n"
                               "       EventCategory: %s\n"
                               "       Assertion : %s\n",
                               pSensorEvent->SensorNum,
                               pSensorEvent->SensorNum,
                               lookupString(pSensorEvent->SensorType, sensorTypeStrings),
                               lookupString(pSensorEvent->EventCategory, eventCategoryStrings),
                               pSensorEvent->Assertion ? "Assert" : "De-assert");
                printEventState(pSensorEvent->EventCategory, pSensorEvent->EventState);
                clLogNotice("EVT", "HPI", 
                            "        OptionalDataPresent(Opt Data Msk) : %d\n",
                            pSensorEvent->OptionalDataPresent);
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_TRIGGER_READING)
                {
                    clLogNotice("EVT", "HPI", 
                                "        Trigger Reading : %s\n",
                                sensorReadingToString(tmpBuf, &pSensorEvent->TriggerReading));
                }
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_TRIGGER_THRESHOLD)
                {
                    clLogNotice("EVT", "HPI", 
                                "        Trigger Threshold : %s\n",
                                sensorReadingToString(tmpBuf, &pSensorEvent->TriggerThreshold));
                }
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_OEM)
                {
                    clLogNotice("EVT", "HPI", 
                                "        OEM : %d\n", pSensorEvent->Oem);
                }
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_PREVIOUS_STATE)
                {
                    clLogNotice("EVT", "HPI", 
                                "        Previous State : %d\n", 
                                pSensorEvent->PreviousState);
                }
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_CURRENT_STATE)
                {
                    clLogNotice("EVT", "HPI", 
                                "        Current State : %d\n", 
                                pSensorEvent->CurrentState);
                }
                if (pSensorEvent->OptionalDataPresent & SAHPI_SOD_SENSOR_SPECIFIC)
                {
                    clLogNotice("EVT", "HPI", 
                                "        Specific : %d\n", 
                                pSensorEvent->SensorSpecific);
                }
            }
        }
    }

    out_free:
    for(i = 0; i < patternArray.patternsNumber; ++i)
    {
        if(patternArray.pPatterns[i].pPattern)
            clHeapFree(patternArray.pPatterns[i].pPattern);
    }
    if(patternArray.pPatterns)
        clHeapFree(patternArray.pPatterns);
    
    if(resTest)
        clHeapFree(resTest);

    clEventFree(eventHandle);
}

static void cmNotificationEventFinalize(ClEventSubscriptionIdT subscriptionId)

{
    ClRcT rc = CL_OK;

    rc = clEventUnsubscribe(gCmEvtChannelHandle, subscriptionId);
    if(rc != CL_OK)
    {
        clLogError("EVT","HPI","Event unsubscribe returned with [%#x]", rc);
    }

    rc = clEventChannelClose(gCmEvtChannelHandle);
    if(rc != CL_OK)
    {
        clLogError("EVT","HPI","Event channel close returned with [%#x]", rc);
    }

    rc = clEventFinalize(gCmEvtHandle);
    if(rc != CL_OK)
    {
        clLogError("EVT","HPI","Event finalize returned with [%#x]", rc);
    }
    
}

static ClRcT
cmNotificationEventInitialize(ClEventSubscriptionIdT subscriptionId,
                              ClPtrT cookie)
{
    ClRcT rc = CL_OK;
    ClVersionT evtVersion = { 'B', 1, 1 };

    rc = clEventInitialize(&gCmEvtHandle, &cmEvtCallbacks, &evtVersion);
    if (rc != CL_OK)
    {
        clLogError("EVT","HPI","Event initialize returned [0x%x]",
                   rc);
        goto out;
    }

    /*Open an event chanel so that we can subscribe to events on that channel*/
    rc = clEventChannelOpen(gCmEvtHandle,
                            &gCmEvtChannelName,
                            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL,
                            (ClTimeT)-1,
                            &gCmEvtChannelHandle);
    if (rc != CL_OK)
    {
        clLogError("EVT","HPI","Event channel open returned with [%#x]",
                   rc);
        goto out_free;
    }

    rc = clEventSubscribe(gCmEvtChannelHandle, NULL, subscriptionId, cookie);
    if(rc != CL_OK)
    {
        clLogError("EVT", "HPI", "Event subscribe returned with [%#x]", rc);
        goto out_free;
    }

    goto out;

    out_free:
    if(gCmEvtChannelHandle)
    {
        clEventChannelClose(gCmEvtChannelHandle);
        gCmEvtChannelHandle = 0;
    }
    if(gCmEvtHandle)
    {
        clEventFinalize(gCmEvtHandle);
        gCmEvtHandle = 0;
    }

    out:
    return rc;
}

ClRcT
cmNotificationInitialize(void)
{
    return cmNotificationEventInitialize(CM_SUBSCRIPTION_ID,
                                         "User (cookie) for CM event delivery callback");
}

void cmNotificationFinalize(void)
{
    cmNotificationEventFinalize(CM_SUBSCRIPTION_ID);
}

