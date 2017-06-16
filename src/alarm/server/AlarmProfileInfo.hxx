// contain soaking/clearing time on storage data

#ifndef ALARMPROFILEINFO_HXX_HEADER_INCLUDED_A6DEEEFA
#define ALARMPROFILEINFO_HXX_HEADER_INCLUDED_A6DEEEFA
#include <boost/algorithm/string/split.hpp>
#include <boost/unordered_map.hpp>
#include <clCustomization.hxx>
#include <SAFplusAlarmCommon.hxx>
//#include <AlarmUtils.hxx>
#include <AlarmRuleRelation.hxx>
using namespace SAFplusAlarm;
using namespace std;
namespace SAFplus
{
// contain soaking/clearing time on storage data
class AlarmProfileInfo
{
  public:
	//constructor
	AlarmProfileInfo();
	AlarmProfileInfo(const AlarmSpecificProblem& aspecificProblem,const bool& aisSend, const int& aintAssertSoakingTime,const int& aintClearSoakingTime,
			const AlarmRuleRelation& agenRuleRelation,const BITMAP64& agenerationRuleBitmap,const AlarmRuleRelation& asuppRuleRelation,const BITMAP64 asuppressionRuleBitmap,
			const int& aintIndex,const BITMAP64 aaffectedBitmap);
	AlarmProfileInfo operator=(const AlarmProfileInfo& other);
	bool operator==(const AlarmProfileInfo& other);
	std::string toString() const;
	//send to fault manager or not
	bool isSend;
	AlarmSpecificProblem specificProblem;
    // assert soaking time
    int intAssertSoakingTime;
    // clear soaking time
    int intClearSoakingTime;
    // poll time milliseconds
    //int intPollTime;
    //generate rule
    BITMAP64 generationRuleBitmap;
	AlarmRuleRelation genRuleRelation;
	//suppression rule
	BITMAP64 suppressionRuleBitmap;
	AlarmRuleRelation suppRuleRelation;
	int intIndex;
    BITMAP64 affectedBitmap;
};
}
#endif /* ALARMPROFILEINFO_HXX_HEADER_INCLUDED_A6DEEEFA */
