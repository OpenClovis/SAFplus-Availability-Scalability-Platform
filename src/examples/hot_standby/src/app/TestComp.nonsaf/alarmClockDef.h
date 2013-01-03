#ifndef ALARM_CLOCK_DEF_H
#define ALARM_CLOCK_DEF_H

typedef struct acTime
{
    ClInt32T hour;
    ClInt32T minute;
    ClInt32T second;
} acTimeT;

typedef enum alarmReactionT
{
    ALARM_REACTION_NONE = 0,
    ALARM_REACTION_TRAP,
    ALARM_REACTION_KILL,
    ALARM_REACTION_MAX = 0xFF
} alarmReactionT;

typedef enum alarmSetValueT
{
    ALARM_SETVALUE_NONE = 0,
    ALARM_SETVALUE_ON,
    ALARM_SETVALUE_OFF,
    ALARM_SETVALUE_MAX = 0xFF
} alarmSetValueT;

typedef struct acAlarmT
{
    acTimeT        time;
    alarmReactionT reaction;
    alarmSetValueT setValue;
} acAlarmT;

typedef struct acClock
{
    acTimeT   clockTime;
    acAlarmT  alarm;
    ClInt32T  clockId;
    ClBoolT   clockRunning;
    ClBoolT   alarmSet;
} acClockT;

ClRcT alarmClockInitialize( void );

void alarmClockFinalize( void );

void alarmClockSetAlarm( ClInt32T clockId, acAlarmT *setAlarm );

void alarmClockGet( acClockT *getClock );

void alarmClockCopyHotStandby( acClockT *backup );

void alarmClockCopyAndStart( acClockT *backup );

void alarmClockSetTime( acTimeT *setTime );

void alarmClockRaiseAlarm( void );

void alarmClockStart( void );

void alarmClockStop( void );

#endif
