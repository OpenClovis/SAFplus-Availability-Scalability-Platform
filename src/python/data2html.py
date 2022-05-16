#from types import *

#import asp

def noop(x): return x

def htmlspan(contents,attr = None,style=None):
  return htmltag("span",contents,attr,style)

def htmltag(tag, contents,attr = None,style=None):
  ret = ["<%s " % tag]
  if attr is not None:
    for (k,v) in attr.items():
      ret.append("%s='%s'" % (k,v))

  if style is not None:
    ret.append("style='")
    for (k,v) in style.items():
      ret.append("%s:%s; " % (k,v))
    ret.append("'")
  
  ret.append(">")
  ret.append(contents)
  ret.append("</%s>" % tag)

  return "".join(ret)


def htmlAppFileVersion(appFile):
  col = 'black'
  if appFile:
    if appFile.app:
      if appFile.app.LatestVersion() == appFile:
        col = 'green'
      else:
        col = 'yellow'
    return htmlspan(appFile.version, None,{ "color": col})
  return ""

def prettyTimeMs(amt):
  amt = int(amt)
  if (amt >= 60*60000) and ((amt%(60*10000))==0):
    return "%.1f hr" % (float(amt)/(60*60000.0))
  if (amt >= 60000) and ((amt%10000)==0):
    return "%.1f min" % (float(amt)/60000.0)
  if (amt >= 1000) and ((amt%100)==0):
    return "%.1f sec" % (float(amt)/1000.0)
  return str(amt) + " ms"

redModel2Str = ["", "No redundancy", "Active standby (2N)", "N Active M Standby (N+M)","N Way", "N Way Active","Custom"] 
#def redModel(rm):
#  rm = int(rm)
#  if rm>asp.CL_AMS_SG_REDUNDANCY_MODEL_NONE and rm<asp.CL_AMS_SG_REDUNDANCY_MODEL_MAX:
#    return redModel2Str[rm]
#  return "Invalid redundancy model: %d" % rm

def availableRedModels():
  """Returns a list of available redundancy model strings.  The index of the string in the list is the ordinality of the redundancy model enum."""
  return redModel2Str[0:3]

loadingStrategy2Str = ["","Fewest work assignments per node","Fewest assigned nodes", "Load balanced", "Configured preference", "User defined"]
#def loadingStrategy(ls):
#  ls = int(ls)
#  if ls>asp.CL_AMS_SG_LOADING_STRATEGY_NONE and ls<=asp.CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED:
#    return loadingStrategy2Str[ls]
#  return "Invalid loading strategy: %d" % ls

def yesNo(bool):
  if isinstancef(bool, basestring):
    lwr = bool.lower()
    if lwr == "y" or lwr == "yes": return "Yes"
    if lwr == "n" or lwr == "no": return "No"
 
  if bool: return "Yes"
  return "No"

def jsonAppFileVersion(appFile):
  col = 'black'
  if appFile:
    if appFile.app:
      if appFile.app.LatestVersion() == appFile:
        col = 'green'
      else:
        col = 'yellow'
    return {"tag":"SPAN","attrs":{"style":"color:%s" % col},"children": appFile.version} 
  return ""

def htmlAppVersions(app):
  ver = app.Versions()
  # GAS TODO: Grey if no one is using the version
  if not ver: return "0.0.0.0"
  return ", ".join(ver)

def htmlAppStatus(app):
  # "Needs Upgrade", "Not running"
  running = 0
  idle    = 0
  stopped = 0
  if len(app.sg) == 0: return "Not deployed"
  for sg in app.sg:
    if sg.isRunning(): running +=1
    elif sg.isIdle(): idle += 1
    else: stopped += 1

  if running: return htmlspan("Running", None,{ "color": "green"})
  if idle: return htmlspan("Idle", None,{ "color": "yellow"})
  return htmlspan("Stopped", None,{ "color": "red"})

def htmlAppVerStatus(appFile):
  app = appFile # Logic is the same as for an app
  running = 0
  idle    = 0
  stopped = 0
  if len(app.sg) == 0: return "Not deployed"
  for sg in app.sg:
    if sg.isRunning(): running +=1
    elif sg.isIdle(): idle += 1
    else: stopped += 1

  if running: return htmlspan("Running", None,{ "color": "green"})
  if idle: return htmlspan("Idle", None,{ "color": "yellow"})
  return htmlspan("Stopped", None,{ "color": "red"})


def htmlSgStatus(sgEntity):
  if not sgEntity.isRunning():
    return(htmlspan("down",  None,{ "color": "red"}))

  if sgEntity.isIdle():
    return(htmlspan("idle",None, {"color":"yellow"}))

  #if sgEntity.isProtected()==False:
  #  return(htmlspan("unprotected", None,{ "color": "orange"}))

  return(htmlspan("up",  None,{ "color": "green"}))

def jsonSgStatus(sgEntity,T=noop):
  data = ("","black")

  if not sgEntity.isRunning():
    data = ("down","red")
  elif sgEntity.isIdle():
    data = ("idle","yellow")
  #elif sgEntity.isProtected()==False:
  #  data = ("unprotected","orange")
  else:
    data = ("up","green")

  return({"tag":"span","attrs":{"style":"color:%s" % data[1]},"children": T(data[0])})



def htmlSiStatus(entity):
  return htmlEntityStatus(entity)

def htmlNodeStatus(n):
  if not n.isPresent():
    return(htmlspan("not present",  None,{ "color": "grey"}))

  if not n.isRunning():
    return(htmlspan("down",  None,{ "color": "red"}))

  #if not n.isProtected():
  #  return(htmlspan("unprotected", None,{ "color": "orange"}))

  if n.isIdle():
    return(htmlspan("idle", None,{ "color": "yellow"}))

  return(htmlspan("up",  None,{ "color": "green"}))

def jsonNodeStatus(n,T=noop):
  if not n.isPresent():
    return({"tag":"SPAN","attrs":{"style":"color:grey"},"children": T("not present")})

  if not n.isRunning():
    return({"tag":"DIV","attrs":{"style":"color:red"},"children": T("down")})

  #if not n.isProtected(): not applied in 7.0
  #  return({"tag":"DIV","attrs":{"style":"color:orange"},"children": T("unprotected")})

  if n.isIdle():
    return({"tag":"DIV","attrs":{"style":"color:yellow"},"children": T("idle")})

  return({"tag":"DIV","attrs":{"style":"color:green"},"children": T("up")})



def htmlCompStatus(n):
  if not n.isRunning():
    return(htmlspan("down",  None,{ "color": "red"}))

  if n.isIdle():
    return(htmlspan("idle", None,{ "color": "yellow"}))

  return(htmlspan("up",  None,{ "color": "green"}))

def jsonCompStatus(n):
  data = ("","black")

  if not n.isRunning():
    data = ("down","red")

  elif n.isIdle():
    data = ("idle","yellow")
  else:
    data = ("up","green")

  return({"tag":"span","attrs":{"style":"color:%s" % data[1]},"children": data[0]})


def htmlEntityName(entity,sep = ""):
  """Returns the entity name(s) and colors them relative to its status"""
  if not isinstance(entity,list):
    entity = [entity]

  ret = []
  for e in entity:
    if not e is None:
      if not e.isRunning():
        ret.append(htmlspan(e.name,  None,{ "color": "grey"}))
      elif e.isIdle()==True:
        ret.append(htmlspan(e.name,  None,{ "color": "yellow"}))
      #elif e.isProtected()==False:
      #  ret.append(htmlspan(e.name,  None,{ "color": "orange"}))
      else: 
        ret.append(htmlspan(e.name,  None,{ "color": "green"}))

  return sep.join(ret)

def jsonEntityName(entity,sep = None):
  """Returns the entity name(s) and colors them relative to its status"""
  if not isinstance(entity, list):
    entity = [entity]

  ret = []
  for e in entity:
    if not e is None:
      if not e.isRunning():
        ret.append({"tag":"span","attrs":{"style":"color:grey"},"children": e.name})
      elif e.isIdle()==True:
        ret.append({"tag":"span","attrs":{"style":"color:yellow"},"children": e.name})
      #elif e.isProtected()==False:
      #  ret.append({"tag":"span","attrs":{"style":"color:orange"},"children": e.name})
      else: 
        ret.append({"tag":"span","attrs":{"style":"color:green"},"children": e.name})
      if sep: ret.append(sep)

  return ret

def jsonEntityStatus(entity,sep = None):
  """Returns the entity name(s) and colors them relative to its status"""
  if entity is None: return ""

  if isinstance(entity,list):
    entity = [entity]

  ret = []
  for e in entity:
    if not e is None:
      if not e.isRunning():
        ret.append({"tag":"span","attrs":{"style":"color:grey"},"children": "down"})
      elif e.isIdle()==True:
        ret.append({"tag":"span","attrs":{"style":"color:yellow"},"children": "idle"})
      #elif e.isProtected()==False:
      #  ret.append({"tag":"span","attrs":{"style":"color:orange"},"children": "unprotected"})
      else: 
        ret.append({"tag":"span","attrs":{"style":"color:green"},"children": "ok"})
      if sep: ret.append(sep)

  return ret
  

def json2html(json):

  if isinstance(json,str): return json
  if json is None: return ""
  if isinstance(json, dict): json = [json]

  ret = []
  for j in json:
    ret.append(htmltag(j["tag"],json2html(j.get("children",None)),j.get("attrs",None),None))

  return ret


def htmlEntityStatus(entity,sep = ""):
  """Return status of an entity"""

  json = jsonEntityStatus(entity,"")
  ret = json2html(json) 
  return sep.join(ret)

def adminState(s,T=noop, json=False):
    if s == '':
      ret = {"tag":"span","attrs":{"style":"color:grey"},"children": "not applicable"}
    elif s == 'on':
      ret = {"tag":"span","attrs":{"style":"color:green"},"children": T("on")}
    elif s == 'idle':
      ret = {"tag":"span","attrs":{"style":"color:yellow"},"children": T("idle")}
    elif s == 'off':
      ret = {"tag":"span","attrs":{"style":"color:red"},"children": T("off")}
    else:  # Catch all should never be executed (unless the state is out of range)
      ret = {"tag":"span","attrs":{"style":"color:red"},"children": str(s)}

    if not json: return "".join(json2html(ret))
    return ret


def Test():
  print (htmlspan("down", style={ "color": "red"}))

  
