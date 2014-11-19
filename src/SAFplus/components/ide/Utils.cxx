#include "utils.h"

Utils::Utils()
{
  //ctor
}

Utils::~Utils()
{
  //dtor
}

std::string Utils::toString(const wxString &wxStr)
{
  return std::string(wxStr.mb_str());
}
