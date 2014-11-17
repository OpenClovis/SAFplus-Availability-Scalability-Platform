#include "yangParser.h"

using namespace std;
namespace bpy = boost::python;

YangParser::YangParser()
{
  m_yangParserModule = bpy::import("yang");
}

YangParser::~YangParser()
{
  //dtor
}

boost::python::tuple YangParser::parseFile(const string &path, const vector<string> &yangFiles)
{
  bpy::list listFiles;
  vector<string>::const_iterator it;
  for (it = yangFiles.begin(); it != yangFiles.end(); ++it){
    listFiles.append(*it);
  }
  return bpy::extract<bpy::tuple>(m_yangParserModule.attr("go")(path, listFiles));
}
