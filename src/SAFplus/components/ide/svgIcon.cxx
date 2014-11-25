#include "svgIcon.h"

using namespace std;
namespace bpy = boost::python;

SvgIcon::SvgIcon()
{
  //ctor
  m_svgModule = bpy::import("svg");
  bitmap=NULL;
}

SvgIcon::~SvgIcon()
{
  //dtor
}


    // The apply function applies the SVG template to the passed dictionary, resulting in a finished SVG file.
void SvgIcon::apply(bpy::dict& replacements)
{
  
}

void SvgIcon::regenerate()
  {
  bitmap = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,250, 250);
  cairo_t* cr = cairo_create(bitmap);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba               (cr,0,1,0,0);
  cairo_paint(cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);  // blending
  //rsvg_handle_set_dpi(im,1000);  // no effect that I can determine

  rsvg_handle_render_cairo(svgHandle,cr);

  cairo_destroy(cr);
  }

#if 0
void SvgIcon::genSvgIcon(SvgIconType svgIconType, const bpy::dict &configDict, RsvgHandle **rsvghdl)
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
#endif
