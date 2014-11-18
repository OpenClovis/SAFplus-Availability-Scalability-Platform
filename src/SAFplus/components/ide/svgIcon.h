#ifndef SVGICON_H
#define SVGICON_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <vector>
#include <Python.h>
#include <boost/python.hpp>
#include <cairo.h>
#include <librsvg/rsvg.h>

class svgIcon
{
  public:
    svgIcon();
    virtual ~svgIcon();
    boost::python::object m_svgModule;
    boost::python::object componentIcon(const std::string &iconBase, const std::string &componentName, const std::string &commandLine);
    void render(const unsigned char *buff, int bufsize, RsvgHandle **hdl);
  protected:
  private:
};

#endif // SVGICON_H
