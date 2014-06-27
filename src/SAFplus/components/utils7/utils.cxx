#include <clThreadApi.hxx>
#include <cltypes.h>
#include <clCksmApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>  // for va_start, etc
#include <memory>    // for std::unique_ptr

#include <clLogApi.hxx>
#include <clCommon.hxx>
#include <clGlobals.hxx>
#include <clTransaction.hxx>

#include <clHandleApi.hxx>

namespace SAFplus
  {

/** Name of the node.  Loaded from the same-named environment variable.  */
  ClCharT ASP_NODENAME[CL_MAX_NAME_LENGTH]="";
/** Name of the component.  Loaded from the same-named environment variable.  */
  ClCharT ASP_COMPNAME[CL_MAX_NAME_LENGTH]="";
/** Address of the node.  This is the slot number in chassis-based system.  This is loaded from the same-named environment variable, and defined in asp.conf.  On chassis-based systems it is expected that a script determine the proper slot number and set this environment variable accordingly (removing it from asp.conf).   */
  ClWordT ASP_NODEADDR = ~((ClWordT) 0);

/** Working dir where programs are run. Loaded from the same-named environment variable.  */
  ClCharT ASP_RUNDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where logs are stored. Loaded from the same-named environment variable.  */
  ClCharT ASP_LOGDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where ASP binaries are located. Loaded from the same-named environment variable.  */
  ClCharT ASP_BINDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where xml config are located. Loaded from the same-named environment variable.  */
  ClCharT ASP_CONFIG[CL_MAX_NAME_LENGTH]="";
/** Dir where db files are to be stored. Loaded from the same-named environment variable.  */
  ClCharT ASP_DBDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].  Deprecated.  Use ASP_APP_BINDIR */
  ClCharT CL_APP_BINDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0]. */
  ClCharT ASP_APP_BINDIR[CL_MAX_NAME_LENGTH]="";

/** Variable to check if the current node is a system controller node.  Loaded from the same-named environment variable.  */
  ClBoolT SYSTEM_CONTROLLER = CL_FALSE; 
/** Variable to check if the current node is a SC capable node.  Loaded from the same-named environment variable.  */
  ClBoolT ASP_SC_PROMOTE = CL_FALSE;

  SaVersionT safVersion = { 'B',4,1 };

  pid_t pid = 0;
  /** True if this component is not under AMF control (will not receive CSI callbacks) */
  bool clWithoutAmf;

  uint64_t curHandleIdx = (1<<SUB_HDL_SHIFT);

  int utilsInitCount=0;

  LogSeverity logSeverityGet(const ClCharT  *pSevName);

  
  //  char* ASP_NODENAME=NULL;
  //char* ASP_COMPNAME=NULL;

  static const ClUint32T crctab[] = {
    0x0,
    0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
    0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
    0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
    0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
    0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
    0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
    0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
    0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
    0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
    0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
    0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
    0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
    0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
    0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
    0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
    0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
    0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
    0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
    0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
    0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
    0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
    0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
    0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
    0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
    0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
    0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
    0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
    0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
    0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
    0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
    0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
    0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
    };
  
  static uint32_t crc_total = ~0;		/* The crc over a number of files. */

#define	COMPUTE(var, ch)	(var) = (var) << 8 ^ crctab[(var) >> 24 ^ (ch)]

  uint32_t computeCrc32(uint8_t *buf, register ClInt32T nr)
    {
    register uint8_t *p;
    register uint32_t crc, len;


    crc = len = 0;
    crc_total = ~crc_total;
    for (len += nr, p = buf; nr--; ++p) 
      {
      COMPUTE(crc, *p);
      COMPUTE(crc_total, *p);
      }

    /* Include the length of the file. */
    for (; len != 0; len >>= 8) {
      COMPUTE(crc, len & 0xff);
      COMPUTE(crc_total, len & 0xff);
      }

    crc_total = ~crc_total;
    return ~crc;
    }
  
  void loadEnvVars()
    {
    ClCharT missing[512];
    ClCharT * temp=NULL;
    ClInt32T i = 0; 
    missing[0] = 0;
    
    if (1) /* Required environment variables */
      {       
      const ClCharT* envvars[] = { "ASP_NODENAME", "ASP_COMPNAME", "ASP_RUNDIR", "ASP_LOGDIR", "ASP_BINDIR", "ASP_CONFIG", "ASP_DBDIR", 0 };
      ClCharT* storage[] = { ASP_NODENAME ,  ASP_COMPNAME ,  ASP_RUNDIR ,  ASP_LOGDIR ,  ASP_BINDIR ,  ASP_CONFIG , ASP_DBDIR, 0 };
    
      for (i=0; envvars[i] != 0; i++)
        {
        temp = getenv(envvars[i]);
        if (temp) strncpy(storage[i],temp,CL_MAX_NAME_LENGTH-1);
        else 
          {
          strcat(missing,envvars[i]);
          strcat(missing," ");
          }
        }
      }
    if (1)  /* Optional environment variables */
      {       
      const ClCharT* envvars[] = { "ASP_APP_BINDIR", 0 };  /* This won't be defined if the AMF is run */
      ClCharT* storage[] = { ASP_APP_BINDIR, 0 };
    
      for (i=0; envvars[i] != 0; i++)
        {
        temp = getenv(envvars[i]);
        if (temp) strncpy(storage[i],temp,CL_MAX_NAME_LENGTH-1);
        }
      }
    
    
    strcpy(CL_APP_BINDIR,ASP_APP_BINDIR);
    
    temp = getenv("ASP_NODEADDR");
    if (temp) ASP_NODEADDR = atoi(temp);
    else strcat(missing,"ASP_NODEADDR ");

    SYSTEM_CONTROLLER = clParseEnvBoolean("SYSTEM_CONTROLLER");
    ASP_SC_PROMOTE = clParseEnvBoolean("ASP_SC_PROMOTE");
    clWithoutAmf = clParseEnvBoolean("ASP_WITHOUT_CPM");

    if (1)
      {
      ClCharT  *pEnvVar = NULL;

      /* Default severity level while booting up the system */
      if( NULL == (pEnvVar = getenv("CL_LOG_SEVERITY")) )
        {
        SAFplus::logSeverity = LOG_SEV_NOTICE;
        }
      else
        {
        SAFplus::logSeverity = logSeverityGet(pEnvVar);
        }
      }
    
    if (missing[0])
      {
      logCritical("ENV", "INI", "The following required environment variables are not set: %s.", missing);
      // should raise
      }
    }

  LogSeverity logSeverityGet(const ClCharT  *pSevName)
    {
    if( NULL == pSevName )
      {
      return LOG_SEV_NOTICE;
      }
    if( !strcmp(pSevName, "EMERGENCY") )
      {
      return LOG_SEV_EMERGENCY;
      }
    else if( !strcmp(pSevName, "ALERT") )
      {
      return LOG_SEV_ALERT;
      }
    else if( !strcmp(pSevName, "CRITICAL") )
      {
      return LOG_SEV_CRITICAL;
      }
    else if( !strcmp(pSevName, "ERROR") )
      {
      return LOG_SEV_ERROR;
      }
    else if( !strcmp(pSevName, "WARN") )
      {
      return LOG_SEV_WARNING;
      }
    else if( !strcmp(pSevName, "NOTICE") )
      {
      return LOG_SEV_NOTICE;
      }
    else if( !strcmp(pSevName, "INFO") )
      {
      return LOG_SEV_INFO;
      }
    else if( !strcmp(pSevName, "DEBUG") )
      {
      return LOG_SEV_DEBUG;
      }
    else if( !strcmp(pSevName, "TRACE") )
      {
      return LOG_SEV_TRACE;
      }
    return LOG_SEV_NOTICE;
    }


  /** \brief  Load the str from a SaNameT structure.
      \param  name The string you want to read.
      \param  str  The destination char* array
      \param  maxLen The length of the available memory buffer.  String will be a max length of maxLen-1 to account for the null terminator.
      If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
  */
void saNameGet(char* str,const SaNameT* name, uint_t maxLen)
  {
  int len = name->length;
  CL_ASSERT(len < maxLen-1);
  if (len > maxLen-1) len = maxLen-1;
  memcpy(str,name->value,len);
  // Add the null terminator if it does not exist in name
  if (str[len-1] != '0') str[len] = 0;
  }
  
/** \brief  Load the SaNameT structure.
    \param  name The structure you want to load
    \param  str  The value to be put into the SaNameT structure

    If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
*/
  void saNameSet(SaNameT* name, const char* str)
    {
    int sz = strlen(str);
    /* Make sure that the name is not too big when in debugging mode */
    CL_ASSERT(sz < CL_MAX_NAME_LENGTH);
    strncpy((ClCharT *)name->value,str,CL_MAX_NAME_LENGTH-1);
    /* If the name is too big, then crop it */
    if (sz >= CL_MAX_NAME_LENGTH)
      {
      name->value[CL_MAX_NAME_LENGTH-1] = 0;
      name->length = CL_MAX_NAME_LENGTH-1;
      }
    else name->length = sz;
    }
/** \brief  Load the SaNameT structure.
    \param  name The structure you want to load
    \param  str  The structure to be put into the SaNameT structure

    If str is too long, then this function will ASSERT in debug mode, and crop in production mode 
*/
  void saNameCopy(SaNameT* nameOut, const SaNameT *nameIn)
    {
    /* Make sure that the name is not too big when in debugging mode */
    CL_ASSERT(nameIn->length < CL_MAX_NAME_LENGTH);

    nameOut->length = nameIn->length;
    /* If the name is too big, then crop it */
    if( nameIn->length >= CL_MAX_NAME_LENGTH )
      {
      nameOut->length = CL_MAX_NAME_LENGTH - 1;
      }
    memcpy(nameOut->value, nameIn->value, nameOut->length);
    nameOut->value[nameOut->length] = 0;
    }

  void saNameConcat(SaNameT* nameOut, const SaNameT *prefix, const char* separator, const SaNameT *suffix)
    {
    unsigned int curpos = 0;

    if (prefix)
      {
      memcpy(nameOut->value, prefix->value, prefix->length);
      curpos += prefix->length;
      }
    if (separator)
      {
      int amt2Copy = CL_MIN((CL_MAX_NAME_LENGTH-1) - curpos,strlen(separator));
      if (amt2Copy) memcpy(&nameOut->value[curpos], separator, amt2Copy);
      curpos += amt2Copy;
      }
    if (suffix)
      {
      int amt2Copy = CL_MIN((CL_MAX_NAME_LENGTH-1) - curpos,suffix->length);
      if (amt2Copy) memcpy(&nameOut->value[curpos], suffix->value, amt2Copy);
      curpos += amt2Copy;
      }
    assert(curpos < CL_MAX_NAME_LENGTH);
    nameOut->value[curpos]=0;
    nameOut->length=curpos;
    }


  ClBoolT clParseEnvBoolean(const ClCharT* envvar)
    {
    ClCharT* val = NULL;
    ClBoolT rc = CL_FALSE;

    val = getenv(envvar);
    if (val != NULL)
      {
      /* If the env var is defined as "". */
      if (strlen(val) == 0)
        {
        rc = CL_TRUE;
        }
      else if (!strncmp(val, "1", 1) ||
        !strncasecmp(val, "yes", 3) || 
        !strncasecmp(val, "y", 1) ||
        !strncasecmp(val, "true", 4) ||
        !strncasecmp(val, "t", 1) )
        {
        rc = CL_TRUE;
        }
      else if (!strncmp(val, "0", 1) ||
        !strncasecmp(val, "no", 2) || 
        !strncasecmp(val, "n", 1) ||
        !strncasecmp(val, "false", 5) ||
        !strncasecmp(val, "f", 1) )
        {
        rc = CL_FALSE;
        }
      else 
        {
        logWarning("UTL", "ENV", "Environment variable [%s] value is not understood.  Please use 'YES' or 'NO'. Assuming 'NO'", envvar);
        rc = CL_FALSE;
        }
      }
    else
      {
      rc = CL_FALSE;
      }

    return rc;
    }

  ClCharT *clParseEnvStr(const ClCharT *envvar)
    {
    ClCharT *val = NULL;
    if(!envvar || !(val = getenv(envvar)))
      return NULL;
    while(*val && isspace(*val)) ++val;
    if(!*val) return NULL;
    return val;
    }

  bool clVersionVerify (ClVersionDatabaseT *versionDatabase, ClVersionT *version)
    {
    int i;
    bool rc = false;

    assert(version);
    /*
     * Look for a release code that we support.  If we find it then
     * make sure that the supported major version is >= to the required one.
     * In any case we return what we support in the version structure.
     */
    for (i = 0; i < versionDatabase->versionCount; i++)
      {

      /*
       * Check if caller requires and old release code that we don't support.
       */
      if (version->releaseCode < versionDatabase->versionsSupported[i].releaseCode)
        {
        break;
        }

      /*
       * Check if we can support this release code.
       */
      if (version->releaseCode == versionDatabase->versionsSupported[i].releaseCode)
        {
        /*
         * Check if we can support the major version requested.
         */
        if (versionDatabase->versionsSupported[i].majorVersion >= version->majorVersion)
          {
          rc = true;
          break;
          } 
        }
      }

    /*
     * If we fall out of the if loop, the caller requires a release code
     * beyond what we support.
     */
    if (i == versionDatabase->versionCount) {
      i = versionDatabase->versionCount - 1; /* Latest version supported */
      }

    /*
     * Tell the caller what we support
     */
    memcpy(version, &versionDatabase->versionsSupported[i], sizeof(*version));
    return rc;
    }
  

  void utilsInitialize()
    {
    if (utilsInitCount==0)
      {
      utilsInitCount++;
      pid = getpid();
      loadEnvVars(); 
      }
    }

  
  Handle Handle::create(void)
    {
    // TODO: mutex lock around this
    Handle hdl(PersistentHandle,curHandleIdx,pid,ASP_NODEADDR); // TODO node and clusterId
    curHandleIdx += (1<<SUB_HDL_SHIFT);
    return hdl;
    }


  std::string strprintf(const std::string fmt_str, ...) 
    {
    int final_n, n = ((int)fmt_str.size()) * 2; /* reserve 2 times as much as the length of the fmt_str */
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
      formatted.reset(new char[n]); /* wrap the plain char array into the unique_ptr */
      strcpy(&formatted[0], fmt_str.c_str());
      va_start(ap, fmt_str);
      final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
      va_end(ap);
      if (final_n < 0 || final_n >= n)
        n += abs(final_n - n + 1);
      else
        break;
      }
    return std::string(formatted.get());
    }

  };
