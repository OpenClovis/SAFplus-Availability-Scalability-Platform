#ifndef TEST_HXX
#define TEST_HXX
#include <clAlarmApi.hxx>

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
AlarmRuleInfo Rule[]=
                  {{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry},
									{AlarmRuleRelation::INVALID,NULL}};
AlarmRuleInfo Rule1[]=
                  {{AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry1},
                  {AlarmRuleRelation::LOGICAL_AND,alarmRuleEntry1},
                  {AlarmRuleRelation::INVALID,NULL}};


AlarmProfile ManagedResource0AlmProfile [] =
{
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::LOSS_OF_SIGNAL,
        AlarmSeverity::MAJOR,
        false,//send to fault management
        0,
        0,
        Rule,
        NULL,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::LOSS_OF_FRAME,
    	AlarmSeverity::MAJOR,
        true,
        0,
        0,
		Rule,
		NULL,
		0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::FRAMING_ERROR,
    	AlarmSeverity::MAJOR,
        true,
        0,
        0,
        Rule,
        NULL,
		0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::LOCAL_NODE_TRANSMISSION_ERROR,
    	AlarmSeverity::MAJOR,
        false,
        0,
        0,
        Rule,
        NULL,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,
    	AlarmSeverity::MAJOR,
        false,
        0,
        0,
        Rule1,
        NULL,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::CALL_ESTABLISHMENT_ERROR,
    	AlarmSeverity::MAJOR,
        true,
        0,
        0,
        Rule1,
        NULL,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::DEGRADED_SIGNAL,
    	AlarmSeverity::MAJOR,
        false,
        0,
        0,
        Rule1,
        NULL,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::SUBSYSTEM_FAILURE,
    	AlarmSeverity::MAJOR,
        true,
        0,
        0,
        Rule1,
        NULL,
        0
    },

    {AlarmCategory::INVALID,AlarmProbableCause::INVALID,AlarmSeverity::INVALID,false,0,0,NULL,NULL,0}
};

AlarmComponentResAlarms appAlarms [] =
{
    {"resourcetest/child", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest/child";
#endif
