#pragma once

#ifndef CL_MGT_BASE_COMMON_H_
#define CL_MGT_BASE_COMMON_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#ifdef __cplusplus
}
#endif
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace boost::property_tree;

enum NCOPERATION {Create = 0,Merge,Replace,Delete};
static std::string xml2json(std::stringstream& strXml);
static std::string json2xmlnoheader(std::stringstream& strJson);
static std::string json2xml(std::stringstream& strJson);
static void parse_tree(const ptree& pt,const std::string& key, std::map<std::string, std::string> &bulkMapPath);
static std::string encodexml(const std::string& strValue)
{
  std::stringstream ss;
  for(int i = 0;i < strValue.length();i++)
  {
    if('<' == strValue[i]) ss <<"&lt;";
    else if('>' == strValue[i]) ss <<"&gt;";
    else if('&' == strValue[i]) ss <<"&amp;";
    else if('"' == strValue[i]) ss <<"&quot;";
    else if('\'' == strValue[i]) ss <<"&apos;";
    else ss << strValue[i];
  }
  return ss.str();
}
static std::string decodexml(const std::string& strValue)
{
  std::string strTemp = strValue;
  boost::replace_all(strTemp, "&lt;", "<");
  boost::replace_all(strTemp, "&gt;", ">");
  boost::replace_all(strTemp, "&amp;", "&");
  boost::replace_all(strTemp, "&quot;", "\"");
  boost::replace_all(strTemp, "&apos;", "\'");
  return strTemp;
}
//class parser to parse value
class xmlParser
{

private:
  ptree pt;

public:
  xmlParser();
  xmlParser(const std::string& strValue );
  void setContent(const std::string& strValue);
  ptree getNode(const std::string& tagName);
  std::string getXMLChild(const std::string& tagName);
  std::vector<std::string> getChildNames(const std::string& tagName);
  std::vector<std::string> getChildNames();
  std::map<std::string, std::string> getChildPathValues(const std::string& tagName,const std::string& basePath = "");
  std::map<std::string, std::string> getChildNameValues(const std::string& tagName);
  int getLevel();
  void getLevel(const ptree& ppt,const int& inLevel, int& outLevel);
  template <class T> T getValue(const std::string& tagName) const
  {
    return pt.get <T> (tagName);
  }
  //this function throw exception if tag not found
  template<typename T> std::vector<T> getValues(const std::string& tagName)
  {
    std::vector<T> retVec;
    for (auto& item : pt.get_child(tagName))
    {
      retVec.push_back(item.second.get_value<T>());
    }
    return retVec;
  }
};
template<typename T> static std::string path2xml(const std::string& strPath,const T& Value,const std::string& strNameSpace,const NCOPERATION& oper = NCOPERATION::Create, const bool& isEncode = true)
{
  std::vector<std::string> results;
  std::stringstream ss;
  boost::split(results, strPath, [](char c)
      { return c == '\\';});
  for (auto it = results.cbegin(); it != results.cend(); ++it)
  {
    if(std::next(it) == results.cend())
    {
      if(NCOPERATION::Create == oper)
      {
        ss<<"<"<<*it<<" operation=\"create\""<<">";
      } else
      {
        ss<<"<"<<*it<<" operation=\"delete\""<<">";
      }
    }else if(it == results.cbegin())
    {
        ss<<"<"<<*it<<" "<<strNameSpace<<">";
    }
    else
    {
      ss<<"<"<<*it<<">";
    }

  }
  if(isEncode && (strcmp(typeid(T).name(),"string") == 0))
  {
    std::stringstream ssvalue;
    ssvalue<<Value;
    ss<<encodexml(ssvalue.str());//put value
  } else
  {
    ss <<Value;
  }
  for (auto it = results.rbegin(); it != results.rend();it++ )
  {
    ss<<"</"<<*it<<">";
  }
  return ss.str();
}
template<typename T> static std::string path2xml(const std::string& strPath,const T& Value, const bool& isEncode = true)
{
  std::vector<std::string> results;
  std::stringstream ss;
  boost::split(results, strPath, [](char c)
      { return c == '\\';});
  for (auto it = results.cbegin(); it != results.cend(); ++it)
  {
    ss<<"<"<<*it<<">";
  }
  if(isEncode && (strcmp(typeid(T).name(),"string") == 0))
  {
    std::stringstream ssvalue;
    ssvalue<<Value;
    ss<<encodexml(ssvalue.str());//put value
  } else
  {
    ss <<Value;
  }
  for (auto it = results.rbegin(); it != results.rend();it++ )
  {
    ss<<"</"<<*it<<">";
  }
  return ss.str();
}
static std::string path2xml(const std::string& strPath)
{
  std::vector<std::string> results;
  std::stringstream ss;
  boost::split(results, strPath, [](char c)
      { return c == '\\';});
  for (auto it = results.cbegin(); it != results.cend(); ++it)
  {
    if(std::next(it) == results.cend())
    {
      ss<<"<"<<*it<<"/>";
    } else
    {
      ss<<"<"<<*it<<">";
    }
  }
  for (auto it = results.rbegin(); it != results.rend();it++ )
  {
    if(it != results.rbegin())
    {
      ss<<"</"<<*it<<">";
    }
  }
  return ss.str();
}

#endif
