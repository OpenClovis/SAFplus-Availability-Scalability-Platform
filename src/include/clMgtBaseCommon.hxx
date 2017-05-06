#pragma once
#ifndef CL_MGT_BASE_COMMON_H_
#define CL_MGT_BASE_COMMON_H_
extern "C"
{
#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
} /* end extern 'C' */
#include <sstream>
#include <string>
#include <map>
#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace boost::property_tree;

static std::string xml2json(std::stringstream& strXml)
{
  ptree pt;
  read_xml(strXml,pt);
  std::stringstream ss;
  write_json(ss, pt);
  return ss.str();
}
static std::string json2xmlnoheader(std::stringstream& strJson)
{
  ptree pt;
  read_json(strJson,pt);
  std::stringstream ss;
  write_xml_element(ss,boost::property_tree::ptree::key_type(),pt,-1,boost::property_tree::xml_writer_settings<boost::property_tree::ptree::key_type>());
  return ss.str();
}
static std::string json2xml(std::stringstream& strJson)
{
  ptree pt;
  read_json(strJson,pt);
  std::stringstream ss;
  write_xml(ss, pt);
  return ss.str();
}
static void parse_tree(const ptree& pt,const std::string& key, std::map<std::string, std::string> &bulkMapPath)
{
  std::string nkey;
  //get end first
  ptree::const_iterator end = pt.end();//get end to verify is new node
  ptree::const_iterator it = pt.begin();//get begin
  if (!key.empty())
    {
      // The full-key/value pair for this node is key / pt.data()
      if(!pt.data().empty()||
          (pt.data().empty() && it == end))//case create new node,no value
        {
          //check data is not empty
          bulkMapPath.insert(std::pair<std::string, std::string>(key,pt.data()));
        }
      nkey = key + "/";  // More work is involved if you use a different path separator
    }
  //continue travel
  for (; it != end; ++it)
    {
      parse_tree(it->second, nkey + it->first,bulkMapPath);
    }
}

//class parser to parse value
class xmlParser
{

  private:
  ptree pt;

  public:
  xmlParser(const std::string& strValue )
    {
      std::stringstream sstrContent;
      try
        {
          sstrContent <<strValue;
          pt.clear();
          read_xml(sstrContent,pt);
        }
      catch(...)
        {
        }
    }
  void setValue(const std::string& strValue)
    {
      std::stringstream sstrContent;
      try
        {
          sstrContent <<strValue;
          pt.clear();
          read_xml(sstrContent,pt);
        }
      catch(...)
        {
        }
    }
//this function throw exception if tag not found
  template <class T> T getValue(const std::string& tagName)
    {
      return pt.get<T>(tagName);
    }

//this function throw exception if tag not found
  template <class T> std::vector<T> getValues(const std::string& tagName)
    {
      std::vector<T> retVec;
      for (auto& item : pt.get_child(tagName))
        {
          retVec.push_back(item.second.get_value<T>());
        }
      return retVec;
    }
  ptree getNode(const std::string& tagName)
    {
      return pt.get_child(tagName);
    }

  std::string getXMLChild(const std::string& tagName)
    {
      ptree noderoot = pt.get_child(tagName);
      std::stringstream ss;
      write_xml_element(ss,ptree::key_type(),noderoot,-1,xml_writer_settings<ptree::key_type>());
      return ss.str();
    }
  std::vector<std::string> getChildNames(const std::string& tagName)
    {
      std::vector<std::string> retVec;
      try
        {
          ptree noderoot = pt.get_child(tagName);
          BOOST_FOREACH(ptree::value_type &vs,noderoot)
            {
              retVec.push_back(vs.first);
            }
        }
      catch(...)
        {
          return retVec;
        }
      return retVec;
    }
  std::vector<std::string> getChildNames()
    {
      std::vector<std::string> retVec;
      try
        {
          ptree::const_iterator end = pt.end();  //get end to verify is new node
          ptree::const_iterator it = pt.begin();//get begin
          for (; it != end; ++it)
            {
              retVec.push_back(it->first);
            }
        }
      catch(...)
        {
          return retVec;
        }
      return retVec;
    }
  std::map<std::string, std::string> getChildPathValues(const std::string& tagName,const std::string& basePath = "")
    {
      std::map <std::string, std::string> retMap;
      try
        {
          ptree noderoot = pt.get_child(tagName);
          parse_tree(noderoot, basePath, retMap);
        }
      catch(...)
        {
          return retMap;
        }
      return retMap;
    }
  std::map<std::string, std::string> getChildNameValues(const std::string& tagName)
    {
      std::map <std::string, std::string> retMap;
      try
        {
          ptree noderoot = pt.get_child(tagName);
          BOOST_FOREACH(ptree::value_type &vs,noderoot)
            {
              retMap.insert(std::pair<std::string, std::string>(vs.first,vs.second.data()));
            }
        }
      catch(...)
        {
          return retMap;
        }
      return retMap;
    }
  int getLevel()
    {
      int retLevel = 0;
      getLevel(pt,0, retLevel);
      return retLevel;
    }
  void getLevel(const ptree& ppt,const int& inLevel, int& outLevel)
    {
      if(outLevel < inLevel) outLevel = inLevel;
      ptree::const_iterator end = ppt.end();  //get end to verify is new node
      ptree::const_iterator it = ppt.begin();//get begin
      if (ppt.empty())
        {
          return;
        }
      else
        {
          for (; it != end;++it)
            {
              getLevel(it->second, inLevel + 1,outLevel);
            }
        }  //else
    }
};
#endif
