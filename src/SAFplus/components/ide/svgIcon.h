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

class svgIcon
{
  public:
    svgIcon();
    virtual ~svgIcon();
    boost::python::object m_svgModule;
    void genSvgIcon(SvgIconType iconType, const boost::python::dict &, RsvgHandle **rsvghdl);
  protected:
  private:
};

#endif // SVGICON_H
