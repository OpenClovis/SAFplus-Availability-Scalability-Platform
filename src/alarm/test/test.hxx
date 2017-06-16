#include <AlarmSeverity.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmRuleRelation.hxx>
#define ALARM_CLIENT

AlarmRuleEntry alarmRuleEntry[]={{AlarmProbableCause::LOSS_OF_SIGNAL,0},
									{AlarmProbableCause::LOSS_OF_FRAME,0},
									{AlarmProbableCause::FRAMING_ERROR,0},
									{AlarmProbableCause::LOCAL_NODE_TRANSMISSION_ERROR,0},
									{AlarmProbableCause::INVALID,0}};
AlarmRuleEntry alarmRuleEntry1[]=   {{AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,0},
									{AlarmProbableCause::CALL_ESTABLISHMENT_ERROR,0},
									{AlarmProbableCause::DEGRADED_SIGNAL,0},
									{AlarmProbableCause::SUBSYSTEM_FAILURE,0},
									{AlarmProbableCause::INVALID,0}};
AlarmRuleInfo Rule[]=               {{AlarmRuleRelation::NO_RELATION,alarmRuleEntry},
									{AlarmRuleRelation::NO_RELATION,alarmRuleEntry},
									{AlarmRuleRelation::NO_RELATION,alarmRuleEntry},
									{AlarmRuleRelation::NO_RELATION,alarmRuleEntry},
									{AlarmRuleRelation::INVALID,NULL}};
AlarmRuleInfo Rule1[]=              {{AlarmRuleRelation::NO_RELATION,alarmRuleEntry1},
		                            {AlarmRuleRelation::NO_RELATION,alarmRuleEntry1},
		                            {AlarmRuleRelation::NO_RELATION,alarmRuleEntry1},
		                            {AlarmRuleRelation::NO_RELATION,alarmRuleEntry1},
		                            {AlarmRuleRelation::INVALID,NULL}};

AlarmProfile profile(AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::LOSS_OF_SIGNAL,
        AlarmSeverity::MAJOR,
        false,//send to fault management
        0,
        0,
        Rule,
        Rule1,
        0);

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
        Rule1,
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
		Rule1,
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
		Rule1,
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
        Rule1,
        0
    },
    {
    	AlarmCategory::COMMUNICATIONS,
    	AlarmProbableCause::REMOTE_NODE_TRANSMISSION_ERROR,
    	AlarmSeverity::MAJOR,
        false,
        0,
        0,
        Rule,
        Rule1,
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
        Rule,
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
        Rule,
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
        Rule,
        0
    },

    {AlarmCategory::INVALID,AlarmProbableCause::INVALID,AlarmSeverity::INVALID,false,0,0,NULL,NULL,0}
};

AlarmComponentResAlarms appAlarms [] =
{
    {"resourcetest", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest";

