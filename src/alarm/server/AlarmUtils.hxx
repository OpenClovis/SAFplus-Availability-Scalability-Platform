// object contains list of alarm infor on storage data

#ifndef ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B
#define ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/functional/hash.hpp>
#include <functional>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>
#include <exception>
#include <clCustomization.hxx>
#include <clCkptApi.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <AlarmInfo.hxx>
#include <AlarmProfileInfo.hxx>
#include <AlarmState.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmCategory.hxx>
#include <AlarmSeverity.hxx>
#include <Summary.hxx>

using namespace SAFplusAlarm;
using namespace SAFplus;

namespace SAFplus
{

struct AlarmKey{
AlarmKey(const std::string& fullPath)
{
	std::vector<std::string> paths;
	boost::algorithm::split(paths, fullPath,boost::is_any_of(":"));
	if(paths.size() != 3) throw std::runtime_error("bad key.It should be 3 keys");
	std::stringstream ss;
	AlarmCategory category;
	AlarmProbableCause probCause;
	std::string resourceId(paths[0]);
	ss << paths[1];
	ss >> category;
	ss << paths[2];
	ss >> probCause;

	AlarmKey(resourceId,category,probCause);
}
AlarmKey(std::string resourceId, AlarmCategory category, AlarmProbableCause probCause)
:tuple(resourceId,category,probCause){}
 boost::tuples::tuple<std::string,AlarmCategory,AlarmProbableCause> tuple;
};
struct AlarmKey2{
AlarmKey2(AlarmCategory category,AlarmProbableCause probCause)
:tuple(category,probCause){}
 boost::tuples::tuple<AlarmCategory,AlarmProbableCause> tuple;
};
struct AlarmFilter{
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

inline std::string toString(const AlarmKey& key)
{
	std::stringstream stream;
	stream<<key.tuple.get<0>()<<":"<<key.tuple.get<1>()<<":"<<key.tuple.get<2>();
	return stream.str();
}

inline bool operator==(const AlarmKey& key1, const AlarmKey& key2)
{
	  return key1.tuple.get<0>() == key2.tuple.get<0>() &&
			 key1.tuple.get<1>() == key2.tuple.get<1>() &&
			 key1.tuple.get<2>() == key2.tuple.get<2>();
}
inline std::size_t hash_value(const AlarmKey &key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key.tuple.get<0>());
  boost::hash_combine(seed, key.tuple.get<1>());
  boost::hash_combine(seed, key.tuple.get<2>());
  return seed;
}
inline bool operator==(const AlarmKey2& key1, const AlarmKey2& key2)
{
	  return key1.tuple.get<0>() == key2.tuple.get<0>() &&
			 key1.tuple.get<1>() == key2.tuple.get<1>();
}
inline std::size_t hash_value(const AlarmKey2 &key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key.tuple.get<0>());
  boost::hash_combine(seed, key.tuple.get<1>());
  return seed;
}
typedef boost::unordered_map<AlarmKey,AlarmProfileInfo> MAPALARMPROFILEINFO;
typedef boost::unordered_map<AlarmKey,AlarmInfo> MAPALARMINFO;
typedef boost::unordered_map<std::string,MAPALARMINFO> MAPMAPALARMINFO;
typedef boost::unordered_map<AlarmProbableCause, AlarmState> AlarmUnorderedProbableMap;
typedef std::function<void(const AlarmInfo&)> EventCallback ;


AlarmState getAlarmState(const AlarmKey& key, const boost::unordered_map<AlarmKey,AlarmProfileInfo>& mapAlarmProfileInfo);
std::string formResourceId(const std::vector<std::string>& paths,const int& intSize);
std::vector<std::string> convertResourceId2vector(const std::string& resourceId);
std::string formResourceIdforPtree(const std::vector<std::string>& paths,const int& intSize);
std::string convertResourceId2Ptree(const std::string& resourceId);

}

#endif /* ALARMUTILS_HXX_HEADER_INCLUDED_A6DEE43B */
