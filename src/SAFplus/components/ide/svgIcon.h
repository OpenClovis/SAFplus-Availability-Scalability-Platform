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

#if 0  // NO objects can be defined in code -- all is in XML
enum SvgIconType
{
    SVG_ICON_NODE         = 1, /**< A Cluster Node (system,computer) */
    SVG_ICON_APP          = 2, /**< A SAF application */
    SVG_ICON_SG           = 3, /**< A SAF service group */
    SVG_ICON_SU           = 4, /**< A SAF service unit */
    SVG_ICON_SI           = 5, /**< A SAF service instance (work assignment) */
    SVG_ICON_COMP         = 6, /**< A SAF component (program) */
    SVG_ICON_CSI          = 7, /**< A SAF component service instance (work assigned to a particular program) */
    SVG_ICON_CLUSTER      = 8, /**< A cluster */
};
#endif



class SvgIcon
{
  public:
    SvgIcon();
    virtual ~SvgIcon();

    SvgIcon(const char* file)
      { init(file); }
    SvgIcon(const char* data, int length)
      { init(data,length); }

    void init(const char* file);
    void init(const char* data, int length);

    // The apply function applies the SVG template to the passed dictionary, resulting in a finished SVG file.
    void apply(boost::python::dict& replacements);

    // Draw this icon into the destination.  Note that x and y are convenience.  You can also use cairo transformations to do full rotation, scaling, and translations
    void draw(cairo_t* destination, int x=0, int y=0);

    boost::python::object m_svgModule;
    //void genSvgIcon(SvgIconType iconType, const boost::python::dict &, RsvgHandle **rsvghdl)

    // Recreate the bitmap from the SVG description
    void regenerate();
    
    
  protected:
    cairo_surface_t* bitmap;
    RsvgHandle*      svgHandle;
  private:
};

#endif // SVGICON_H
