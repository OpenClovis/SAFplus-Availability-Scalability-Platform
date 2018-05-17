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
#include <iostream>
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
/*convert xml format to json format
 *strXml: string xml format
 *return : string json format
 */
static std::string xml2json(std::stringstream& strXml);
/*convert json format to xml format with no header
 *strJson: string json format
 *return : string xml format with no header
 */
static std::string json2xmlnoheader(std::stringstream& strJson);
/*convert json format to xml format with header
 *strJson: string json format
 *return : string xml format with header
 */
static std::string json2xml(std::stringstream& strJson);
/*parser from p_tree to map of paths
 *pt: input ptree node
 *bulkMapPath : output map of paths
 */
static void parse_tree(const ptree& pt,const std::string& key, std::map<std::string, std::string> &bulkMapPath);
/*encode special character of string to be compatible with xml format
 *strValue: input string value
 *return : string xml format is encoded
 */
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
/*decodexml special character from xml format to normal string
 *strValue: input string value
 *return : string xml format is decoded
 */
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
  /*constructor
   *strValue: input string xml format
   */
  xmlParser(const std::string& strValue );
  /*set content for parser
   *strValue: input string xml format
   */
  void setContent(const std::string& strValue);
  /*getNode
   *tagName: input string path with format tag.tag1.tag2
   *return: a node of input path
   */
  ptree getNode(const std::string& tagName);
  /*get all children and values
   *tagName: input string path with format tag.tag1.tag2
   *return: get all children of input path
   */
  std::string getXMLChild(const std::string& tagName);
  /*get all children name
   *tagName: input string path with format tag.tag1.tag2
   *return: get all children name
   */
  std::vector<std::string> getChildNames(const std::string& tagName);
  /*get all children name of content in xml
   *return: get all children name
   */
  std::vector<std::string> getChildNames();
  /*get all children values
   *tagName: input string path with format tag.tag1.tag2
   *basePath: get the values from this path
   *return: get all children values
   */
  std::map<std::string, std::string> getChildPathValues(const std::string& tagName,const std::string& basePath = "");
  /*get all children values
   *tagName: input string path with format tag.tag1.tag2
   *return: get all children values of current content
   */
  std::map<std::string, std::string> getChildNameValues(const std::string& tagName);
  /*get log level of current xml content
   *return: log level
   */
  int getLevel();
  /*get log level
   *ppt: p_tree node
   *inLevel: level input
   *outLevel: level output
   */
  void getLevel(const ptree& ppt,const int& inLevel, int& outLevel);
  /*get value
   *ppt: p_tree node
   *tagName: input string path with format tag.tag1.tag2
   *return: current value at that path
   */
  template <class T> T getValue(const std::string& tagName) const
  {
    return pt.get <T> (tagName);
  }
  /*get values this function throw exception if tag not found
   *tagName: input string path with format tag.tag1.tag2
   *return: vector of values
   */
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
/*convert value to xml
 *strPath: input string path with format [name='test']
 *return: string with format <name>test</name>
 */
static std::string convertval2xml(const std::string& strPath)
{
  std::vector<std::string> results;
  std::stringstream ss;
  size_t pos;
  std::string strLocalPath = strPath;
  pos = strLocalPath.find("[");
  if(pos != std::string::npos)
  {
    boost::split(results, strLocalPath, [](char c)
        { return c == '[';});
    ss<<"<"<<results[0]<<">";
    pos = results[1].find("[");
    if(pos != std::string::npos)
    {
      results[1].erase(pos,1);
    }
    pos = results[1].find("]");
    if(pos != std::string::npos)
    {
      results[1].erase(pos,1);
    }
    pos = results[1].find("\'");
    while(pos != std::string::npos)
    {
      results[1].erase(pos,1);
      pos = results[1].find("\'");
    }
    std::vector<std::string> resultsval;
    if(results[1].find("=") != std::string::npos)
    {
      boost::split(resultsval, results[1], [](char c)
          { return c == '=';});
      if(resultsval.size() == 2)
      {
        ss<<"<"<<resultsval[0]<<">";
        ss<<resultsval[1];
        ss<<"</"<<resultsval[0]<<">";
      }
      ss<<"</"<<results[0]<<">";
    }
  } else
  {
    ss<<"<"<<strLocalPath<<"/>";
  }
  return ss.str();
}
/*convert path to xml
 *strPath: input string path with format tag/tag1/tag2
 *strNameSpace: string of namespace
 *oper: operation create/delete
 *isEncode: need encode
 *return: xml format
 */
template<typename T> static std::string path2xml(const std::string& strPath,const T& Value,const std::string& strNameSpace,const NCOPERATION& oper = NCOPERATION::Create, const bool& isEncode = true)
{
  std::vector<std::string> results;
  std::stringstream ss;
  if(strPath.length() > 0)
  {
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
      } else if(it == results.cbegin())
      {
        ss<<"<"<<*it<<" "<<strNameSpace<<">";
      }
      else
      {
        ss<<"<"<<*it<<">";
      }
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
/*convert path to xml
 *strPath: input string path with format tag/tag1/tag2
 *Value: input value
 *oper: operation create/delete*isEncode :needencode
 *return: xml format
 */
template<typename T> static std::string path2xml(const std::string& strPath, const T& Value, const bool& isEncode = true)
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
  for (auto it = results.rbegin(); it != results.rend(); it++)
  {
     ss<<"</"<<*it<<">";
  }
  return ss.str();
}
/*convert path to xml
 *strPath: input string path with format tag/tag1/tag2[name='value']
 *return: xml format
 */
static std::string path2xml(const std::string& strPath)
{
  std::vector<std::string> results;
  std::stringstream ss;
  std::string localPath = strPath;
  size_t pos = localPath.find("\\");
  if (pos == 0)
  {
    localPath.erase(0, 1);
  }
  boost::split(results, localPath, [](char c)
  { return c == '\\';});
  for (auto it = results.cbegin(); it != results.cend();++it)
  {
    if(std::next(it) == results.cend())
    {
      ss<<convertval2xml(*it);
    } else
    {
      ss<<"<"<<*it<<">";
    }
  }
  for(auto it = results.rbegin(); it != results.rend();it++)
  {
    if (it != results.rbegin())
    {
      ss<<"</"<<*it<<">";
    }
  }
  return ss.str();
}

#endif
