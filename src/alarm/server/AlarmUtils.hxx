// object contains list of alarm infor on storage data

#ifndef ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B
#define ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/functional/hash.hpp>
#include <functional>
#include <boost/unordered_map.hpp>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>
#include <clCustomization.hxx>
#include <clCkptApi.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <AlarmInfo.hxx>
#include <AlarmTimerInfo.hxx>
#include <AlarmProfileData.hxx>
#include <AlarmState.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmCategory.hxx>
#include <AlarmSeverity.hxx>
#include <Summary.hxx>

using namespace SAFplusAlarm;
using namespace SAFplus;

namespace SAFplus
{

struct AlarmKey
{
  AlarmKey(const std::string& fullPath)
  {
    std::vector < std::string > paths;
    boost::algorithm::split(paths, fullPath, boost::is_any_of(":"));
    if (paths.size() != 3) throw std::runtime_error("bad key.It should be 3 keys");
    std::stringstream ss;
    AlarmCategory category;
    AlarmProbableCause probCause;
    std::string resourceId(paths[0]);
    ss << paths[1];
    ss >> category;
    ss << paths[2];
    ss >> probCause;

    AlarmKey(resourceId, category, probCause);
  }
  AlarmKey(std::string resourceId, AlarmCategory category, AlarmProbableCause probCause) :
      tuple(resourceId, category, probCause)
  {
  }
  string toString() const
  {
    std::ostringstream oss;
    oss << "resourceId:[" << tuple.get<0>().c_str() << "] category:[" << tuple.get<1>() << "] probCause:[" << tuple.get<2>() << "]";
    return oss.str();
  }
  boost::tuples::tuple<std::string, AlarmCategory, AlarmProbableCause> tuple;
};
struct AlarmKey2
{
  AlarmKey2(AlarmCategory category, AlarmProbableCause probCause) :
      tuple(category, probCause)
  {
  }
  boost::tuples::tuple<AlarmCategory, AlarmProbableCause> tuple;
};
struct AlarmFilter
{
  std::string resourceId;
  AlarmProbableCause probCause;
  AlarmSpecificProblem specificProblem;
};

struct AlarmComponentResAlarms
{
  /**
   * Resource \e MOID for which the alarm profiles are provided in the \e MoAlarms.
   */
  std::string resourceId;
  /**
   * The frequency at which the resource will be
   * polled for the current status of all of its alarms.
   */
  int pollingTime;
  /**
   * Pointer to AlarmProfile structures of all alarms for the resource.
   */
  AlarmProfile* MoAlarms;
};
inline void copyKey(const AlarmProfileData& profileData, AlarmInfo& alarmInfo)
{
  memset(alarmInfo.resourceId, 0, sizeof(alarmInfo.resourceId));
  strcpy(alarmInfo.resourceId, profileData.resourceId);
  alarmInfo.category = profileData.category;
  alarmInfo.probCause = profileData.probCause;
}
inline void copyKey(const AlarmInfo& alarmInfo, AlarmProfileData& profileData)
{
  memset(profileData.resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  strcpy(profileData.resourceId, alarmInfo.resourceId);
  profileData.category = alarmInfo.category;
  profileData.probCause = alarmInfo.probCause;
}
inline std::string toString(const AlarmKey& key)
{
  std::stringstream stream;
  stream << key.tuple.get<0>() << ":" << key.tuple.get<1>() << ":" << key.tuple.get<2>();
  return stream.str();
}

inline bool operator==(const AlarmKey& key1, const AlarmKey& key2)
{
  return key1.tuple.get<0>() == key2.tuple.get<0>() && key1.tuple.get<1>() == key2.tuple.get<1>() && key1.tuple.get<2>() == key2.tuple.get<2>();
}
inline std::size_t hash_value(const AlarmKey &key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key.tuple.get<0>());
  boost::hash_combine(seed, key.tuple.get<1>());
  boost::hash_combine(seed, key.tuple.get<2>());
  return seed;
}
inline bool operator==(const std::string& key1, const std::string& key2)
{
  return (key1.compare(key2) == 0);
}
inline std::size_t hash_value(const std::string& key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key);
  return seed;
}
inline bool operator==(const AlarmKey2& key1, const AlarmKey2& key2)
{
  return key1.tuple.get<0>() == key2.tuple.get<0>() && key1.tuple.get<1>() == key2.tuple.get<1>();
}
inline std::size_t hash_value(const AlarmKey2 &key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key.tuple.get<0>());
  boost::hash_combine(seed, key.tuple.get<1>());
  return seed;
}
typedef boost::unordered_map<std::size_t, AlarmProfileData> MAPALARMPROFILEINFO;
typedef boost::unordered_map<std::size_t, AlarmInfo> MAPALARMINFO;
typedef boost::unordered_map<std::size_t, AlarmTimerInfo> MAPALARMTIMERINFO;
typedef boost::unordered_map<std::size_t, MAPALARMINFO> MAPMAPALARMINFO;
typedef std::function<void(void*)> EventCallback1;

std::string formResourceId(const std::vector<std::string>& paths, const int& intSize);
std::vector<std::string> convertResourceId2vector(const std::string& resourceId);
std::string formResourceIdforPtree(const std::vector<std::string>& paths, const int& intSize);
std::string convertResourceId2Ptree(const std::string& resourceId);

}

#endif /* ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B */
