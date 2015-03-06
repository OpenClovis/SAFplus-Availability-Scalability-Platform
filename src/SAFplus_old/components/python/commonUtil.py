import re

retype = type(re.compile(""))
def Delve(recDict, keylst, default):
  """
     Dives down a recursive dictionary structure and returns the value of the list of keys specified in keylst.
     You may use "*" in the keylst to indicate all keys at a particular level.  Or use a compiled regular expression.
  """
  curItem = recDict
  try:
    i = 0
    for key in keylst:
      i+=1
      if type(key) is retype:
        ret = []
        for (k,v) in recDict.items():
          if key.match(k):
            ret.append(Delve(v,keylst[i:],None))
        ret = filter(lambda x: x!=None, ret)
        return ret

      if key == "*": # Match all, must recurse
        ret = []
        for (k,v) in recDict.items():
          ret.append(Delve(v,keylst[i:],None))
        ret = filter(lambda x: x!=None, ret)
        return ret
      else: # Standard dictionary match
        curItem = curItem[key]
    return curItem
  except KeyError:
    return default
  except TypeError:
    return default

retype = type(re.compile(""))
def DelvePath(recDict, keylst, default):
  """
     Dives down a recursive dictionary structure and returns the value of the list of keys specified in keylst.
     You may use "*" in the keylst to indicate all keys at a particular level.  Or use a compiled regular expression.

     Returns a list of pairs ([path],value) where [path] is a list of all of the matched keys required to
     descend and value is the final value.
  """
  if len(keylst) == 0: return ([],recDict)

  try:

      key = keylst[0]
      rest = keylst[1:]

      if type(key) is retype or key == "*":
        ret = []
        for (k,v) in recDict.items():
          if type(key) is retype and not key.match(k):
            pass
          else:
            rec = DelvePath(v,rest,None)
            if rec:
              if type(rec) is type([]):
                  ret += [ ([k] + r[0], r[1]) for r in rec]
              else:
                ret.append(([k] + rec[0], rec[1]))

        return ret

      else: # Standard dictionary match
        path, result = DelvePath(recDict[key],rest,default)
        return ([key] + path, result)

  except KeyError:
    return default
  except TypeError:
    return default
