#ifndef _CL_SNMP_OCTRAIN_ENUMS_H
#define _CL_SNMP_OCTRAIN_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* enums for column nodeCreate */
#define NODECREATE_ACTIVE        1
#define NODECREATE_NOTINSERVICE        2
#define NODECREATE_NOTREADY        3
#define NODECREATE_CREATEANDGO        4
#define NODECREATE_CREATEANDWAIT        5
#define NODECREATE_DESTROY        6


/* enums for scalar clockRunningStatus */
#define CLOCKRUNNINGSTATUS_STOPPED        0
#define CLOCKRUNNINGSTATUS_RUNNING        1

/* enums for scalar clockRedStatus */
#define CLOCKREDSTATUS_NONRED        0
#define CLOCKREDSTATUS_RED        1

#ifdef __cplusplus
}
#endif

#endif /* _CL_SNMP_OCTRAIN_ENUMS_H */
