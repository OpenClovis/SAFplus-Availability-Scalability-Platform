#ifndef TEST_HXX
#define TEST_HXX
#include <clAlarmApi.hxx>

AlarmRuleEntry alarmRuleEntry2[]=
                  {{AlarmProbableCause::STORAGE_CAPACITY_PROBLEM,0},
									{AlarmProbableCause::VERSION_MISMATCH,0},
									{AlarmProbableCause::CORRUPT_DATA,0},
									{AlarmProbableCause::CPU_CYCLES_LIMIT_EXCEEDED,0},
									{AlarmProbableCause::INVALID,0}};
AlarmRuleEntry alarmRuleEntry[]=
                  {{AlarmProbableCause::LOSS_OF_SIGNAL,0},
                  {AlarmProbableCause::LOSS_OF_FRAME,0},
                  {AlarmProbableCause::FRAMING_ERROR,0},
                  {AlarmProbableCause::LOCAL_NODE_TRANSMISSION_ERROR,0},
                  {AlarmProbableCause::INVALID,0}};

AlarmRuleEntry alarmRuleEntry1[]=
                  {{AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,0},
									{AlarmProbableCause::CALL_ESTABLISHMENT_ERROR,0},
									{AlarmProbableCause::DEGRADED_SIGNAL,0},
									{AlarmProbableCause::SUBSYSTEM_FAILURE,0},
									{AlarmProbableCause::INVALID,0}};

AlarmRuleEntry suppAlarmRuleEntry[]=
                  {{AlarmProbableCause::LOSS_OF_SIGNAL,0},
                  {AlarmProbableCause::LOSS_OF_FRAME,0},
                  {AlarmProbableCause::FRAMING_ERROR,0},
                  {AlarmProbableCause::LOCAL_NODE_TRANSMISSION_ERROR,0},
                  {AlarmProbableCause::INVALID,0}};
AlarmRuleEntry suppAlarmRuleEntry1[]=
                  {{AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,0},
                  {AlarmProbableCause::CALL_ESTABLISHMENT_ERROR,0},
                  {AlarmProbableCause::DEGRADED_SIGNAL,0},
                  {AlarmProbableCause::SUBSYSTEM_FAILURE,0},
                  {AlarmProbableCause::INVALID,0}};

AlarmRuleInfo Rule[]=
                  {{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::INVALID,NULL}};
AlarmRuleInfo Rule2[]=
                  {{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry2},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry2},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry2},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry2},
                  {AlarmRuleRelation::INVALID,NULL}};
AlarmRuleInfo Rule1[]=
                  {{AlarmRuleRelation::LOGICAL_OR,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,alarmRuleEntry1},
                  {AlarmRuleRelation::INVALID,NULL}};

AlarmRuleInfo SuppRule[]=
                  {{AlarmRuleRelation::LOGICAL_AND,suppAlarmRuleEntry},
                  {AlarmRuleRelation::LOGICAL_AND,suppAlarmRuleEntry},
                  {AlarmRuleRelation::LOGICAL_AND,suppAlarmRuleEntry},
                  {AlarmRuleRelation::LOGICAL_AND,suppAlarmRuleEntry},
                  {AlarmRuleRelation::INVALID,NULL}};
AlarmRuleInfo SuppRule1[]=
                  {{AlarmRuleRelation::LOGICAL_OR,suppAlarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,suppAlarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,suppAlarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_OR,suppAlarmRuleEntry1},
                  {AlarmRuleRelation::INVALID,NULL}};


AlarmProfile ManagedResource0AlmProfile [] =
{
    {
    	AlarmCategory::COMMUNICATIONS,//0
    	AlarmProbableCause::LOSS_OF_SIGNAL,
      AlarmSeverity::MAJOR,
      true,//send to fault management
      0,
      0,
      Rule,
      NULL,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//1
    	AlarmProbableCause::LOSS_OF_FRAME,
    	AlarmSeverity::MAJOR,
      false,
      10000,
      0,
      Rule,
      NULL,
      0,
      false
    },
    {
    	AlarmCategory::COMMUNICATIONS,//2
    	AlarmProbableCause::FRAMING_ERROR,
    	AlarmSeverity::MAJOR,
      true,
      0,
      10000,
      Rule,
      NULL,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//3
    	AlarmProbableCause::LOCAL_NODE_TRANSMISSION_ERROR,
    	AlarmSeverity::MAJOR,
      false,
      10000,
      10000,
      Rule,
      NULL,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//4
    	AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,
    	AlarmSeverity::MAJOR,
      false,
      5000,
      10000,
      Rule,
      NULL,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//5
    	AlarmProbableCause::CALL_ESTABLISHMENT_ERROR,
    	AlarmSeverity::MAJOR,
      true,
      10000,
      5000,
      Rule1,
      NULL,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//6
    	AlarmProbableCause::DEGRADED_SIGNAL,
    	AlarmSeverity::MAJOR,
      false,
      5000,
      10000,
      Rule1,
      SuppRule,
      0,
      true
    },
    {
    	AlarmCategory::COMMUNICATIONS,//7
    	AlarmProbableCause::SUBSYSTEM_FAILURE,
    	AlarmSeverity::MAJOR,
      true,
      5000,
      10000,
      Rule1,
      SuppRule1,
      0,
      true
    },
    {
      AlarmCategory::COMMUNICATIONS,//8
      AlarmProbableCause::STORAGE_CAPACITY_PROBLEM,
      AlarmSeverity::MAJOR,
      false,
      40000,
      0,
      Rule2,
      NULL,
      0,
      false
    },
    {
      AlarmCategory::COMMUNICATIONS,//9
      AlarmProbableCause::VERSION_MISMATCH,
      AlarmSeverity::MAJOR,
      true,
      0,
      40000,
      Rule2,
      NULL,
      0,
      true
    },
    {AlarmCategory::INVALID,AlarmProbableCause::INVALID,AlarmSeverity::INVALID,false,0,0,NULL,NULL,0,false}
};

const int RETRY = 5;
#endif
