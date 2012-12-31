#ifndef ALARM_CLOCK_CKPT_H
#define ALARM_CLOCK_CKPT_H

#include <clCkptApi.h>
#include <clCkptIpi.h>
#include <clCkptErrors.h>
#include <saCkpt.h>
#include <clOsalApi.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SaCkptHandleT  alarmClockCkptHdl;

SaAisErrorT    alarmClockCkptInitialize (void);
SaAisErrorT    alarmClockCkptFinalize (void);

SaAisErrorT
alarmClockCkptCreate (
    const  SaStringT             ckpt_name,
    SaUint32T                    num_sections,
    SaSizeT                      section_size,
    SaCkptCheckpointHandleT      *ckpt_hdl );

SaAisErrorT 
alarmClockCkptWrite (
    SaCkptCheckpointHandleT      ckpt_hdl,
    SaUint32T                    section_num,
    void*                        data,
    SaUint32T                    data_size );

SaAisErrorT 
alarmClockCkptRead (
    SaCkptCheckpointHandleT      ckpt_hdl,
    SaUint32T                    section_num,
    void*                        data,
    SaUint32T                    data_size );

SaAisErrorT 
alarmClockCkptActivate(SaCkptCheckpointHandleT ckptHdl, 
                       ClUint32T section_num);

ClRcT alarmClockCkptLock(void);

ClRcT alarmClockCkptUnlock(void);

SaAisErrorT alarmClockCkptSessionDelete(SaCkptCheckpointHandleT ckpt_hdl, 
                                        ClUint32T section_num,
                                        ClUint32T id);

SaAisErrorT alarmClockCkptSessionFinalize(SaCkptCheckpointHandleT ckpt_hdl, 
                                          ClUint32T section_num);

SaAisErrorT alarmClockCkptHotStandbyRegister(SaCkptCheckpointHandleT ckpt_hdl);

SaAisErrorT alarmClockCkptHotStandbyDeregister(SaCkptCheckpointHandleT ckpt_hdl);

#ifdef __cplusplus
}
#endif

#endif
