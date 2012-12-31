/******************************************************************************
 * Alarm Clock routines used in the training exercise
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <clCommon.h>
#include <clTimerApi.h>
#include <clTimerErrors.h>
#include <clLogApi.h>
#include <clAlarmApi.h>

#include "alarmClockDef.h"
#include "alarmClockLog.h"
#include "alarmClockCkpt.h"
#include "session.h"

static acClockT         alarmClock;
static ClTimerHandleT   timerHandle;     

/*******************************************************************************
API: alarmClockAdvance

Description : Advance the second, minute and hour hands of the clock

              In addition if an alarm time is reached call the raise alarm API.
Arguments In: 
    none
Arguments Out:
    none
Return Value:
    ClRcT
*******************************************************************************/

ClRcT alarmClockAdvance ( void )
{
    ClBoolT  raiseAlarm = CL_FALSE;
    ClInt32T pid = getpid();

    /* advance the clock by a second
     */
    if (++alarmClock.clockTime.second >= 60)
    {
        alarmClock.clockTime.second = 0;
        if (++alarmClock.clockTime.minute >= 60)
        {
            alarmClock.clockTime.minute = 0;
            if (++alarmClock.clockTime.hour >= 24)
            {
                alarmClock.clockTime.hour = 0;
            }
        }
    }


    /* Turn the alarm off if it is due to go off; or else the standby 
     * will also set the alarm off; 
     */
    if ((alarmClock.alarmSet == CL_TRUE) &&
        (alarmClock.alarm.time.hour == alarmClock.clockTime.hour) &&
        (alarmClock.alarm.time.minute == alarmClock.clockTime.minute))
    {
        alarmClock.alarmSet = CL_FALSE;
        raiseAlarm = CL_TRUE;
    }
    

    /*
     * Checkpoint every 2 seconds
     */
    if (! (alarmClock.clockTime.second & 1) )
    {
        alarmClockCkptLock();
        alarmClockCkptWrite(alarmClockCkptHdl, 1, &alarmClock, sizeof(acClockT));
        alarmClockCkptUnlock();
    }

    /* Log 
     */
    alarmClockLogWrite(CL_LOG_SEV_INFO,
                       "alarm clock (pid=%d): time is %dh:%dm:%ds\n", 
                       pid, 
                       alarmClock.clockTime.hour,
                       alarmClock.clockTime.minute,
                       alarmClock.clockTime.second);

    if (raiseAlarm == CL_TRUE)
    {
        /* raising an alarm can have two consequences
         * calling the API to raise the alarm
         * or killing the process that has triggered
         * the alarm
         */
        alarmClockRaiseAlarm();
    }

    return CL_OK;
}

/*******************************************************************************
API: alarmClockHotStandbyCopy

Description : Copy the clock data structure; except for clock running status
              which on the standby should be "stopped"

Arguments In: 
    acClockT

Arguments Out:
    none
Return Value:
    ClRcT
*******************************************************************************/
void alarmClockCopyHotStandby ( acClockT *backup )
{
    if(sessionBufferMode())
    {
        memcpy(&alarmClock, backup, sizeof(acClockT));
        return;
    }

    if (alarmClock.clockRunning == CL_TRUE)
    {
        alarmClockLogWrite(CL_LOG_SEV_CRITICAL, 
                "alarmClockHotStandbyCopy(pid=%d): clock running; will be stopped\n",
                getpid());
        /* stop the clock if it is running
         */
        alarmClockStop();
    }
  
    /* copy the clock struct
     */
    memcpy( &alarmClock, backup, sizeof(acClockT));

    /* set value of clockRunning to false as it has been stopped
     */
    alarmClock.clockRunning = CL_FALSE;
}

/*******************************************************************************
API: alarmClockCopyAndStart

Description : Copy the incoming clock structure; called when a checkpoint
              is read and the standby is updating its state

              Clock will be started at end of copy

Arguments In: 
    acClockT

Arguments Out:
    none
Return Value:
    ClRcT
*******************************************************************************/
void alarmClockCopyAndStart ( acClockT *backup )
{
    /* stop the clock if it is running
     */
    alarmClockStop();
  
    /* copy the clock struct
     */
    memcpy( &alarmClock, backup, sizeof(acClockT));

    /* set value of clockRunning to false as it has been stopped
     */
    alarmClock.clockRunning = CL_FALSE;

    /* start the clock 
     */
    alarmClockStart();
}

/*******************************************************************************
API: alarmClockInitialze

Description : Set the alarm clock to all zeroes, and the boolean variable to 
              indicate non-running clock

Arguments In: 
    none
Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
ClRcT alarmClockInitialize ( void )
{
    ClRcT                rc = CL_OK;
    ClTimerTimeOutT      timeOut;

    alarmClock.clockTime.hour   = 0;
    alarmClock.clockTime.minute = 0;
    alarmClock.clockTime.second = 0;

    alarmClock.alarm.time.hour   = 0;
    alarmClock.alarm.time.minute = 0;
    alarmClock.alarm.time.second = 0;
    alarmClock.alarm.reaction    = ALARM_REACTION_NONE;

    alarmClock.clockId          = 0;;

    alarmClock.clockRunning = CL_FALSE;
    alarmClock.alarmSet     = CL_FALSE;

    /* one second in the clock is 50 millisecond of real time
     * as an enhancement this can be made provisionable as well
     */
    timeOut.tsSec = 1;
    timeOut.tsMilliSec = 0;

/*
    rc = clTimerInitialize(NULL);
    if (rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_CRITICAL,
                    "alarmClockInitialize(pid=%d): Timer initialize failed:0x%x\n", 
                    getpid(), rc);
        return rc;
    }
*/

    rc = clTimerCreate(timeOut, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT,
                       (ClTimerCallBackT)&alarmClockAdvance, NULL, &timerHandle);
    if (rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_CRITICAL,
                    "alarmClockInitialize(pid=%d): Timer start failed: 0x%x\n", 
                    getpid(), rc);
    }

    alarmClockLogWrite(CL_LOG_DEBUG,
            "alarmClockInitialize(pid=%d): Clock Initialize successful\n", 
            getpid());

    return rc;
}

/*******************************************************************************
API: alarmClockFinalize

Description : Stops the timer driving the clock and delete the timer

Arguments In: 
    none
Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockFinalize ( void )
{
    ClRcT                rc = CL_OK;

    /* Stop the clock
     */
    alarmClockStop();

    rc =  clTimerDelete(timerHandle);
    if (rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_ERROR,
                    "alarmClockFinalize(pid=%d): Timer delete failed: 0x%x\n", 
                    getpid(), rc);
    }

    rc = clTimerFinalize();
    if (rc != CL_OK)
    {
        alarmClockLogWrite(CL_LOG_SEV_CRITICAL,
                    "alarmClockFinalize(pid=%d): Timer finalize failed:0x%x\n", 
                    getpid(), rc);
        return;
    }

}

/*******************************************************************************
API: alarmClockGet

Description : Get the current clock struct

Arguments In/Out: 
    1. acTimeT * : time structure

Return Value:
    none
*******************************************************************************/
void alarmClockGet ( acClockT *retClock )
{
    memcpy(retClock, &alarmClock, sizeof(acClockT));
}

/*******************************************************************************
API: alarmClockSetAlarm

Description : Set the alarm time with no validation, as that is assumed done prior
              to calling this API. 

              Set the ClockId if nonzero

Arguments In: 
    1. clockId   : Clock identifier (used to generate traps)
    2. acTimeT * : validated time structure

Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockSetAlarm ( ClInt32T clockId, acAlarmT *alarm )
{
    /* if alarm is already set, set it to false
     * so that we do not accidentally raise an
     * alarm
     */
    if (alarm->setValue == ALARM_SETVALUE_ON)
    {
        alarmClock.alarmSet = CL_TRUE;
    }
    else if (alarm->setValue == ALARM_SETVALUE_OFF)
    {
        alarmClock.alarmSet = CL_FALSE;
    }

    alarmClock.alarm.time.hour   = alarm->time.hour;
    alarmClock.alarm.time.minute = alarm->time.minute;

    if (alarm->reaction != ALARM_REACTION_NONE)
    {
        alarmClock.alarm.reaction = alarm->reaction;
    }

    if ( clockId != 0 )
    {
        alarmClock.clockId = clockId;
    }
}

/*******************************************************************************
API: alarmClockSetTime

Description : Set the clock time with no validation, as that is assumed done prior
              to calling this API. 


Arguments In: 
    1. acTimeT * : validated time structure

Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockSetTime ( acTimeT *setTime )
{
    /* stop the clock 
     */
    alarmClockStop();

    alarmClock.clockTime.hour   = setTime->hour;
    alarmClock.clockTime.minute = setTime->minute;
    alarmClock.clockTime.second = setTime->second;

    /*  start the clock
     */
    alarmClockStart();
}

/*******************************************************************************
API: alarmClockRaiseAlarm

Description : Does the prescribed behavior on reaching an alarm state; i.e.
              raise an alarm or kill the process

Arguments In: 
    None

Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockRaiseAlarm ( void )
{
    if (alarmClock.alarm.reaction == ALARM_REACTION_KILL)
    {
         alarmClockLogWrite(CL_LOG_SEV_NOTICE,
                 "alarmClockRaiseAlarm(pid=%d): killing the clock\n", getpid());

        /* kill the process running the clock
        * to force a failover
        */
        exit(alarmClock.clockId);
    }
    else if (alarmClock.alarm.reaction == ALARM_REACTION_TRAP)
    {
         alarmClockLogWrite(CL_LOG_SEV_NOTICE,
                 "alarmClockRaiseAlarm(pid=%d): raising an alarm not supported yet\n", getpid());
    }
    else
    {
         alarmClockLogWrite(CL_LOG_SEV_NOTICE,
                 "alarmClockRaiseAlarm(pid=%d): reaction value not set; not raising alarm\n",
                 getpid());

    }
}


/*******************************************************************************
API: alarmClockStart

Description : Starts the clock (i.e the timer that drives the clock)

Arguments In: 
    None

Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockStart( void )
{
    ClRcT rc = CL_OK;

    if (alarmClock.clockRunning == CL_FALSE)
    {
        rc = clTimerStart(timerHandle);
        if (rc != CL_OK)
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR,
                    "alarmClockStart(pid=%d): Timer start failed: 0x%x\n", 
                    getpid(), rc);
        }
        else
        {
            alarmClock.clockRunning = CL_TRUE;

            alarmClockLogWrite(CL_LOG_DEBUG,
                "alarmClockStart(pid=%d): Clock Start successful\n", 
                getpid());
        }
    }
    else
    {
        alarmClockLogWrite(CL_LOG_DEBUG,
            "alarmClockStart(pid=%d): Clock already started\n", 
            getpid());
    }
}

/*******************************************************************************
API: alarmClockStop

Description : Stops the clock (i.e the timer that drives the clock)

Arguments In: 
    None

Arguments Out:
    none
Return Value:
    none
*******************************************************************************/
void alarmClockStop( void )
{
    ClRcT rc = CL_OK;
    if (alarmClock.clockRunning == CL_TRUE)
    {
        rc = clTimerStop(timerHandle);
        if (rc != CL_OK)
        {
            alarmClockLogWrite(CL_LOG_SEV_ERROR,
                    "alarmClockStop(pid=%d): Timer stop failed: 0x%x\n", 
                    getpid(), rc);
        }
        else
        {
            alarmClock.clockRunning = CL_FALSE;

            alarmClockLogWrite(CL_LOG_DEBUG,
                "alarmClockStop(pid=%d): Clock Stop successful\n", 
                getpid());
        }
    }
    else
    {
        alarmClockLogWrite(CL_LOG_DEBUG,
            "alarmClockStop(pid=%d): Clock already stopped\n", 
            getpid());
    }
}
