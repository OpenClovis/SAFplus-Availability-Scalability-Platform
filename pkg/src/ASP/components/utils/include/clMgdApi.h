#ifndef CL_MGD_API
#define CL_MGD_API

#include "clCommon.h"
#include "clQueueApi.h"

struct ClMgdCommon
{
  ClNameT      name;
  struct ClMgdCommon* parent;
};

typedef struct ClMgdCommon ClMgdCommonT;

typedef struct
{
  ClMgdCommonT cmn;
  ClQueueT*    children;
} ClMgdNodeT;


typedef struct
{
  ClMgdCommonT cmn;
} ClMgdLeafT;


void clMgdNodeInit(ClMgdNodeT* mgd, const char* name);
void clMgdLeafInit(ClMgdLeafT* mgd, const char* name);

void clMgdLeafDestroy(ClMgdLeafT* mgd);
void clMgdNodeDestroy(ClMgdLeafT* mgd);


typedef struct
{
  ClMgdLeafT mgd;
  int       admin;
  int       oper;
  const char** names;
} ClMgdStateT;

/**
 @param stateNames This parameter is an array of strings that correspond to the strings
 in the enum's states.  It must be a constant (i.e. it is NOT copied by this routine, but used after the routine ends -- until clMgdStateDestroy is called).
 */
void clMgdStateInit(ClMgdStateT* ths, ClMgdNodeT* parent, char* name, int initialState, const char* stateNames[]);

void clMgdAdminStateSet(ClMgdStateT* ths, int state);

void clMgdOperStateSet(ClMgdStateT* ths, int state);


  /** \brief  Add a managed item into a node's list
      \param  ths   The node
      \param  item  The item to be added
      \param  name  The name to use (pass NULL to use the managed item's default name)

      If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
   */
void clMgdAdd(ClMgdNodeT* ths, ClMgdCommonT* item, const char* name);


#endif
