#ifndef UTILS_H
#define UTILS_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <string>

class Utils
{
  public:
    Utils();
    virtual ~Utils();
    static std::string toString(const wxString &);
  protected:
  private:
};

#endif // UTILS_H
