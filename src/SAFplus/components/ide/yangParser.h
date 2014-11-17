#ifndef YANGPARSER_H
#define YANGPARSER_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <vector>
#include <Python.h>
#include <boost/python.hpp>

class YangParser
{
  public:
    YangParser();
    virtual ~YangParser();
    boost::python::object m_yangParserModule;
    boost::python::tuple parseFile(const std::string &path, const std::vector<std::string> &yangFiles);
  private:
};

#endif // YANGPARSER_H
