"""
@namespace aspMisc

This module provides common functionalities required by ASP applications and other modules
"""

import asp
import types

AspError = {
  asp.SA_AIS_OK: "SA_AIS_OK",
  asp.SA_AIS_ERR_LIBRARY: "SA_AIS_ERR_LIBRARY",
  asp.SA_AIS_ERR_VERSION: "SA_AIS_ERR_VERSION",
  asp.SA_AIS_ERR_INIT: "SA_AIS_ERR_INIT",
  asp.SA_AIS_ERR_TIMEOUT: "SA_AIS_ERR_TIMEOUT",
  asp.SA_AIS_ERR_TRY_AGAIN: "SA_AIS_ERR_TRY_AGAIN",
  asp.SA_AIS_ERR_INVALID_PARAM: "SA_AIS_ERR_INVALID_PARAM",
  asp.SA_AIS_ERR_NO_MEMORY: "SA_AIS_ERR_NO_MEMORY",
  asp.SA_AIS_ERR_BAD_HANDLE: "SA_AIS_ERR_BAD_HANDLE",
  asp.SA_AIS_ERR_BUSY: "SA_AIS_ERR_BUSY",
  asp.SA_AIS_ERR_ACCESS: "SA_AIS_ERR_ACCESS",
  asp.SA_AIS_ERR_NOT_EXIST: "SA_AIS_ERR_NOT_EXIST",
  asp.SA_AIS_ERR_NAME_TOO_LONG: "SA_AIS_ERR_NAME_TOO_LONG",
  asp.SA_AIS_ERR_EXIST: "SA_AIS_ERR_EXIST",
  asp.SA_AIS_ERR_NO_SPACE: "SA_AIS_ERR_NO_SPACE",
  asp.SA_AIS_ERR_INTERRUPT: "SA_AIS_ERR_INTERRUPT",
  asp.SA_AIS_ERR_NAME_NOT_FOUND: "SA_AIS_ERR_NAME_NOT_FOUND",
  asp.SA_AIS_ERR_NO_RESOURCES: "SA_AIS_ERR_NO_RESOURCES",
  asp.SA_AIS_ERR_NOT_SUPPORTED: "SA_AIS_ERR_NOT_SUPPORTED",
  asp.SA_AIS_ERR_BAD_OPERATION: "SA_AIS_ERR_BAD_OPERATION",
  asp.SA_AIS_ERR_FAILED_OPERATION: "SA_AIS_ERR_FAILED_OPERATION",
  asp.SA_AIS_ERR_MESSAGE_ERROR: "SA_AIS_ERR_MESSAGE_ERROR",
  asp.SA_AIS_ERR_QUEUE_FULL: "SA_AIS_ERR_QUEUE_FULL",
  asp.SA_AIS_ERR_QUEUE_NOT_AVAILABLE: "SA_AIS_ERR_QUEUE_NOT_AVAILABLE",
  asp.SA_AIS_ERR_BAD_FLAGS: "SA_AIS_ERR_BAD_FLAGS",
  asp.SA_AIS_ERR_TOO_BIG: "SA_AIS_ERR_TOO_BIG",
  asp.SA_AIS_ERR_NO_SECTIONS: "SA_AIS_ERR_NO_SECTIONS"
}

def getErrorString(errCode):
    if type(errCode) == str:
        return errCode
    try:
        retStr = AspError[errCode]
    except KeyError:
        retStr = "UNKNOWN_ERROR_CODE"
    
    return retStr


def setSaVersionT(saVersionT,releaseCode,major,minor):
  """Set the SAF SaVersionT C object
  @param saVersionT  The object (of type @ref asp.SaVersionT) to be modified
  @param releaseCode A single character string indicating the SAF release code
  @param major       SAF major version (integer)
  @param minor       SAF minor version (integer)
  """
  saVersionT.releaseCode = ord(releaseCode)
  saVersionT.majorVersion = int(major)
  saVersionT.minorVersion = int(minor)
  return saVersionT

def SaVersionT(releaseCode,major,minor):
  """Create a new initialized SAF SaVersionT C object
  @param releaseCode A single character string indicating the SAF release code
  @param major       SAF major version (integer)
  @param minor       SAF minor version (integer)
  """
  savt = asp.SaVersionT()
  return setSaVersionT(savt,releaseCode,major,minor)

def getCNameT(clNameT):
  """Convert a clNameT object into a string
  Its probably a bug somewhere, but sometimes the clNameT is not correctly ended by SWIG, so this function applies fixups.
  @param   clNameT  ASP clNameT C object
  @returns string of the name
  """
  try:
    if ord(clNameT.value[clNameT.length-1]) == 0: # Amf fixup
      return clNameT.value[0:clNameT.length-1]
    else:
      return clNameT.value[0:clNameT.length]
  except IndexError:
    return clNameT.value

def setCNameT(clNameT, s):
  """Put a string into an ASP clNameT C object
  @param clNameT the destination C code
  @param str     The source string
  @returns clNameT
  """
  try: # strip unicode, if val is a string
    s = s.encode('utf8')
  except:
    pass
  s = str(s)

  clNameT.value  = s
  clNameT.length = len(s) + 1
  return clNameT
