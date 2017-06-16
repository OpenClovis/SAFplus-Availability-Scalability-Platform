#include "AlarmUtils.hxx"
#include <boost/algorithm/string.hpp>

using namespace std;

namespace SAFplus
{
AlarmState getAlarmState(const AlarmKey& key, const boost::unordered_map<AlarmKey,AlarmProfileInfo>& mapAlarmProfileInfo)
{
	/*std::unordered_map<AlarmKey,AlarmProfileInfo>::const_iterator it = mapAlarmProfileInfo.find(key);
	if(it != mapAlarmProfileInfo.end())
	{
		return it->second.state;
	}*/
	return AlarmState::INVALID;
}
std::string formResourceId(const std::vector<std::string>& paths,const int& intSize)
{
	std::string path;
	for(int i = 0; i < paths.size(), i < intSize;i++)
	{
		path += "\\"+ paths[i];
	}
	return path;
}
std::vector<std::string> convertResourceId2vector(const std::string& resourceId)
{
	std::vector<std::string> paths;
	boost::algorithm::split(paths, resourceId,boost::is_any_of("/"));
	return paths;
}
std::string formResourceIdforPtree(const std::vector<std::string>& paths,const int& intSize)
{
	std::string path;
	for(int i = 0; i < paths.size(), i < intSize;i++)
	{
		if(i > 0) path +="."+paths[i];
		else path += paths[i];
	}
	return path;
}
std::string convertResourceId2Ptree(const std::string& resourceId)
{
	std::vector<std::string> vect = convertResourceId2vector(resourceId);
	return formResourceIdforPtree(vect,vect.size());
}

}
