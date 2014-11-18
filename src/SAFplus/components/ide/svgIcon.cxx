#include "svgIcon.h"

using namespace std;
namespace bpy = boost::python;

svgIcon::svgIcon()
{
  //ctor
  m_svgModule = bpy::import("svg");
}

svgIcon::~svgIcon()
{
  //dtor
}

boost::python::object svgIcon::componentIcon(const std::string &iconBase, const std::string &componentName, const std::string &commandLine)
{
  return m_svgModule.attr("componentIcon")(iconBase, componentName, commandLine);
}

void svgIcon::render(const unsigned char *buf, int bufsize, RsvgHandle **rsvghdl)
{
  GError *error;
  if (!rsvg_handle_write(*rsvghdl, buf, bufsize, &error))
  {
    rsvg_handle_close(*rsvghdl,&error);
    *rsvghdl = NULL;
  }
  else rsvg_handle_close(*rsvghdl,&error);
}
