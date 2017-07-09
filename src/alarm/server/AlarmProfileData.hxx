// contain soaking/clearing time on storage data

#ifndef ALARMPROFILEDATA_HXX_HEADER_INCLUDED_A6DEEEFA
#define ALARMPROFILEDATA_HXX_HEADER_INCLUDED_A6DEEEFA
#include <boost/algorithm/string/split.hpp>
#include <boost/unordered_map.hpp>
#include <clCustomization.hxx>
#include <SAFplusAlarmCommon.hxx>
#include <AlarmRuleRelation.hxx>
#include <AlarmCategory.hxx>
#include <AlarmProbableCause.hxx>

using namespace SAFplusAlarm;
using namespace std;
namespace SAFplus
{
// contain soaking/clearing time on storage data
class AlarmProfileData
{
public:
  //constructor
  AlarmProfileData();
  AlarmProfileData(const std::string& inresourceId, const AlarmCategory& incategory, const AlarmProbableCause& inprobCause, const AlarmSpecificProblem& aspecificProblem, const bool& aisSend, const int& aintAssertSoakingTime, const int& aintClearSoakingTime, const AlarmRuleRelation& agenRuleRelation,
      const BITMAP64& agenerationRuleBitmap, const AlarmRuleRelation& asuppRuleRelation, const BITMAP64& asuppressionRuleBitmap, const int& aintIndex, const BITMAP64& aaffectedBitmap,const bool& aisSuppressChild);
  AlarmProfileData operator=(const AlarmProfileData& other);
  bool operator==(const AlarmProfileData& other);
  std::string toString() const;
  char resourceId[SAFplusI::MAX_RESOURCE_NAME_SIZE];
  // alarm cagegory type
  AlarmCategory category;
  // alarm probalbe  cause
  AlarmProbableCause probCause;
  AlarmSpecificProblem specificProblem;
  //send to fault manager or not
  bool isSend;
  // assert soaking time
  int intAssertSoakingTime;
  // clear soaking time
  int intClearSoakingTime;
  //generate rule
  BITMAP64 generationRuleBitmap;
  AlarmRuleRelation genRuleRelation;
  //suppression rule
  BITMAP64 suppressionRuleBitmap;
  AlarmRuleRelation suppRuleRelation;
  int intIndex;
  BITMAP64 affectedBitmap;
  bool isSuppressChild;

};
}
#endif /* ALARMPROFILEDATA_HXX_HEADER_INCLUDED_A6DEEEFA */
