#ifndef _CL_SNMP_OCTRAIN_COLUMNS_H
#define _CL_SNMP_OCTRAIN_COLUMNS_H

#ifdef __cplusplus
extern "C" {
#endif

/* column number definitions for table clockTable */
#define COLUMN_CLOCKROW        1
#define COLUMN_CLOCKID        2
#define COLUMN_CLOCKHOUR        3
#define COLUMN_CLOCKMINUTE        4
#define COLUMN_CLOCKSECOND        5
#define COLUMN_ALARMHOUR        6
#define COLUMN_ALARMMINUTE        7
#define COLUMN_ALARMREACTION        8
#define COLUMN_ALARMSET        9

/* column number definitions for table timeSetTable */
#define COLUMN_TIMESETROW        1
#define COLUMN_TIMESETHOUR        2
#define COLUMN_TIMESETMINUTE        3
#define COLUMN_TIMESETSECOND        4

/* column number definitions for table nameTable */
#define COLUMN_NODEADD        1
#define COLUMN_NODENAME        2
#define COLUMN_NODEIP        3
#define COLUMN_NODECREATE        4

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_OCTRAIN_COLUMNS_H */
