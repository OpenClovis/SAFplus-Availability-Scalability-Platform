#ifndef UTILS_H
#define UTILS_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <string>
#include <boost/python.hpp>

extern std::string parse_python_exception();

class Utils
{
  public:
    Utils();
    virtual ~Utils();
    static std::string toString(const wxString &);
  protected:
  private:
};

extern wxWindow* createPythonControlledWindow(const char* module, wxWindow* parent,wxMenuBar* menubar, wxToolBar* toolbar, wxStatusBar* statusbar,boost::python::object& obj, bool isInstance);

boost::python::object loadModel(const char* modelName);


#endif // UTILS_H
