#ifndef ALARM_CLOCK_CKPT_H
#define ALARM_CLOCK_CKPT_H

//#include <clCkptUtils.h>
#include <clCkptErrors.h>
#include <clCkptApi.h>
#include <clCkptIpi.h>

extern ClCkptHdlT  alarmClockCkptHdl;
ClRcT
alarmClockCkptInitialize (void);

ClRcT
alarmClockCkptFinalize (void);

ClRcT
alarmClockCkptCreate (
    const ClCharT     *ckpt_name,
    ClInt32T          num_sections,
    ClInt32T          section_size,
    ClCkptHdlT        *ckpt_hdl,
    ClBoolT           *hot_standby );

ClRcT 
alarmClockCkptWrite (
    ClCkptHdlT     ckpt_hdl,
    ClInt32T       section_num,
    void*          data,
    ClInt32T       data_size );

ClRcT 
alarmClockCkptRead (
    ClCkptHdlT     ckpt_hdl,
    ClInt32T       section_num,
    void*          data,
    ClInt32T       data_size );

ClRcT 
alarmClockCkptActivate(ClCkptHdlT ckptHdl, ClUint32T numSections);

#endif
