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

void svgIcon::genSvgIcon(SvgIconType svgIconType, const bpy::dict &configDict, RsvgHandle **rsvghdl)
{
  int iconType = static_cast<int>(svgIconType);
  std::string svgStr = bpy::extract<std::string>(m_svgModule.attr("loadSvgIcon")(iconType, configDict));

  GError *error;
  if (!rsvg_handle_write(*rsvghdl, (const unsigned char *)svgStr.c_str(), (int)svgStr.length(), &error))
  {
    rsvg_handle_close(*rsvghdl,&error);
    *rsvghdl = NULL;
  }
  else rsvg_handle_close(*rsvghdl,&error);
}
