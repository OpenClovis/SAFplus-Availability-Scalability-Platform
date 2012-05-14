#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <saAis.h>


namespace SAFplus
{

typedef enum
{
  NoExtendedError      = 0,
  NoActiveReplica,
  SectionDoesNotExist,
  OutOfMemory,
  MsgServerFailure,
  NonExistentDestination,
} ExtendedErrorNumber;

typedef enum
  {
    SafError        = 0,
    ClError         = 1,
    SystemError     = 2
  } ErrorFamily;
  
  
class Error
{
  public:
  Error(ErrorFamily family, unsigned int returnCode, ExtendedErrorNumber extErrp=NoExtendedError, const char* notep = NULL, const char* filep = __FILE__, int linep = __LINE__)
  {
   file     = filep;
   line     = linep;
   extErr   = extErrp;
   rc       = returnCode;
   rcFamily = family;
   if (notep)
     {
       strncpy(note,notep,CL_LOG_MAX_MSG_LEN);
       note[CL_LOG_MAX_MSG_LEN] = 0;
     }
   else note[0] = 0;
  }

   ErrorFamily  rcFamily;
   unsigned int rc;
   ExtendedErrorNumber extErr;
   char         note[CL_LOG_MAX_MSG_LEN+1];
   const char*  file;
   int          line;
};

};
