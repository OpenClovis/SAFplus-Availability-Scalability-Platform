#!/usr/bin/python
import sys, os, os.path, time

#try:
if 1:
  import localaccess as access
  print "using local access"

#except ImportError, e:
#  import netconfaccess as access

SuNameColor = (50,160,80)
SuNameSize = 32

SuListColor = (20,100,60)
SuListSize = 40

NodeColor = (30,30,0xc0)
LeafColor = (0x30,0,0x30)

FullPathColor = (0xa0,0x70,0)

SAFplusNamespace = "http://www.openclovis.org/ns/amf"

import pdb
import xml.etree.ElementTree as ET

try:
  import xmlterm
  windowed=True
except Exception, e:
  windowed=False

def formatTag(tag):
  """Take a tag and format it for display"""
  if tag[0] == "{":  # Get rid of the namespace indicator b/c that's ugly
    tag = tag.split("}")[1]
  return tag
  

def commonSuffix(stringList):
  """Given a list of strings, return the string that is the common ending of all strings in the list"""
  revList = [ s[::-1] for s in stringList]
  revPfx = os.path.commonprefix(revList)
  return revPfx[::-1]

# Handlers take XML ElementTree objects and turn them into displayable objects


def epochTimeHandler(elem, resolver,context):
  """Convert an xml leaf containing seconds since the epoch into a user readable date panel"""
  seconds = int(elem.text)
  s = time.strftime('%Y-%b-%d %H:%M:%S', time.localtime(seconds))
  w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + formatTag(elem.tag) + ": " +  s)
  resolver.add(w)  

def epochMsTimeHandler(elem, resolver,context):
  """Convert an xml leaf containing milli-seconds since the epoch into a user readable date panel"""
  try:
    mseconds = int(elem.text)
    s = time.strftime('%Y-%b-%d %H:%M:%S', time.localtime(mseconds/1000))
  except TypeError:
    if elem.text is None: s = 'unspecified'
    else:  # garbage in the field
      assert(0)  
      s = "!error bad numeric field: ", str(elem.text)

  w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + formatTag(elem.tag) + ": " +  s)
  resolver.add(w)  
  
def elapsedSecondsHandler(elem, resolver,context):
  """Convert an xml leaf containing seconds to a user readable version of elapsed time"""
  if elem.text is None:
    s = ": unspecified"
  else:
    seconds = int(elem.text)
    days, rest = divmod(seconds,60*60*24)
    hours, rest = divmod(rest,60*60)
    minutes, seconds = divmod(rest,60)
    s = ": %d days %d hours %d minutes and %d seconds" % (days,hours,minutes,seconds)
  w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + formatTag(elem.tag) + s)
  resolver.add(w)  
  

def childReorg(elem,path):
  """Reorganize children of this element into the manner preferred by display.  This includes:
     1. Grouping all elements of a list into a common child node
  """
  lists = {}
  #pdb.set_trace()
  if elem.attrib.get("type",None) != "list":  # If I'm already a list, no list item isolation needed
   for child in elem:  # go thru all the children
    lstName = access.isListElem(child,path)  # Ask the access layer if this is a list element
    if lstName:  # if so, add this element to our temporary dictionary and remove it from elem
      if not lists.has_key(lstName):
        lists[lstName] = []
      lists[lstName].append(child)

   for (lstName,children) in lists.items():  # go through the temp dict creating list elements and adding items
    e = ET.SubElement(elem,lstName)
    e.attrib["type"] = "list"  # Mark this new element as a list so no infinite recursion
    for c in children:
      elem.remove(c)
      e.append(c)

def defaultHandler(elem,resolver,context):
  """Handle normal text and any XML that does not have a specific handler"""
  # Name preference order is: "key" attribute of tag (for lists) > "name" attribute of tag > "name" child of tag > tag 
  childReorg(elem,context.path)
  nameChild = elem.find("name")
  if nameChild is not None: name=nameChild.text
  else: name=elem.tag
  name = elem.attrib.get("name",elem.tag) 
  name = elem.attrib.get("listkey", name)
  name = formatTag(name) # Get rid of the namespace indicator b/c that's ugly
  # ns = 

  fore = None
  #more = elem.find("more")
  #if more is None:
  #  more = elem.find("{%s}more" % SAFplusNamespace)
  #if more is not None:

  if elem._children:  # it will either have child nodes or a "more" child node if it has children
    fore = NodeColor
  else:
    fore = LeafColor

  if elem.text:
    top = name + ":" + elem.text
  else:
    top = name

  w = xmlterm.FancyTextChunked(resolver.parentWin, ("  "*resolver.depth) + top,chunkSize=2048,fore=fore)
  resolver.add(w)  

  resolver.depth += 1
  context.path.append(elem.tag)
  for child in elem:
    resolver.resolve(child,context)
    if child.tail and child.tail.strip():
      w = xmlterm.FancyTextChunked(resolver.parentWin,("  "*resolver.depth) + child.tail,chunkSize=2048)
      resolver.add(w)
  del context.path[-1]   
  resolver.depth -= 1


def topHandler(elem,resolver,context):
  """Make this XML node invisible -- skip down to the children"""
  childReorg(elem,context.path)
  for child in elem:
    fullpath = child.attrib.get("path",None)
    if fullpath: # print out the full path with a : if it is placed into the child's attribute list.  This only happens for top level nodes so the user can see where they were found
      resolver.add(xmlterm.FancyText(resolver.parentWin,fullpath + ":",fore=FullPathColor))      
      resolver.depth+=1
    resolver.resolve(child,context)
    if fullpath:
      resolver.depth-=1

    if child.tail and child.tail.strip():
      w = xmlterm.FancyTextChunked(resolver.parentWin,("  "*resolver.depth) + child.tail,chunkSize=2048)
      resolver.add(w)
    

def childrenOnlyHandler(elem,resolver,context):
  """Make this XML node invisible -- skip down to the children"""
  childReorg(elem,context.path)
  for child in elem:
    resolver.resolve(child,context)
    if child.tail and child.tail.strip():
      w = xmlterm.FancyTextChunked(resolver.parentWin,("  "*resolver.depth) + child.tail,chunkSize=2048)
      resolver.add(w)

def historyHandler(elem,resolver,context):
  """Create a plot for the historical statistics XML nodes"""
  if elem.text:
    data = [float(x) for x in elem.text.split(",")]
    title = elem.attrib.get("title", formatTag(elem.tag))
    xlabel = elem.attrib.get("xlabel", "time")
    ylabel = elem.attrib.get("ylabel", "")  # TODO: grab the parent's tag as the ylabel?
    
    plotAttrib = { "title":title, "xlabel":xlabel, "ylabel":ylabel}
    if len(resolver.windows)>1:  # If there is lots of other data in this command, show a small graph
      plotAttrib["size"]="(200,100)"
    plot = ET.Element("plot",plotAttrib)
    series = ET.Element("series", {"label":""},text=elem.text)
    series.text = elem.text
    plot.append(series)
    resolver.add(xmlterm.FancyText(resolver.parentWin,formatTag(elem.tag)))
    w = xmlterm.PlotPanel(resolver.parentWin,plot)
    resolver.add(w)
  else:
    resolver.defaultHandler(elem,resolver,context)


def serviceUnitHandler(elem,resolver,context):
  """Custom representation of a service unit"""
  name = elem.attrib.get("listkey",formatTag(elem.tag))
  w = xmlterm.FancyText(resolver.parentWin,("  "*resolver.depth) + name,fore=SuNameColor,size=SuNameSize)
  resolver.add(w)
  try:
    w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + "  Admin: %s Operational: %s %s " % (elem.find("adminState").text,"OK" if elem.find("operState").text == "1" else "FAULTED", elem.find("haState").text))
    resolver.add(w)
    w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + "  Active Work: %s Standby Work: %s" % (elem.find("numActiveServiceInstances").find("current").text,elem.find("numStandbyServiceInstances").find("current").text))
    resolver.add(w)
    w = xmlterm.FancyText(resolver.parentWin, ("  "*resolver.depth) + "  On Node: %s  In Service Group: %s" % (elem.find("node").text.split("/")[-1],elem.find("serviceGroup").text.split("/")[-1]))
    resolver.add(w)
  except AttributeError, e: # object could have no children because recursion depth exceeded
    pass 

def serviceUnitListHandler(elem,resolver,context):
  """Create a graphical representation of the XML 'text' tag"""
  if elem.attrib.has_key("listkey"): # Its an instance of the list
    serviceUnitHandler(elem,resolver,context)
    return
  path = elem.attrib.get("path",None)
  if path == "/SAFplusAmf/ServiceUnit" or len(elem)>0:  # serviceunit tag can be a list, a list entry, or a su ref.  SU ref has no children
    resolver.add(xmlterm.FancyText(resolver.parentWin,("  "*resolver.depth) + "ServiceUnit",fore=SuListColor,size=SuListSize))
    resolver.depth += 1
    for su in elem:
      serviceUnitHandler(su,resolver,context)
    resolver.depth -= 1
  else:  # Could be ServiceUnit has no children because recursion depth exceeded
    w = xmlterm.FancyText(resolver.parentWin,("  "*resolver.depth) + formatTag(elem.tag))  # so normal display
    resolver.add(w)
    
  
  if elem.text:  # There should not be any text in the service unit list but display it if there is
    size = elem.attrib.get("size",None)
    if size: size = int(size)
    fore = elem.attrib.get("fore",None)
    if fore: fore = color.rgb(fore) # [ int(x) for x in fore.split(",")]
    back = elem.attrib.get("back",None)
    if back: back = color.rgb(back) # [ int(x) for x in back.split(",")]
    w = xmlterm.FancyText(resolver.parentWin,elem.text,fore=fore, back=back,size=size)
    resolver.add(w)

seriesTags = ["history10sec","history1min","history10min","history1hour","history1day","history1week","history4weeks"]

def findSeries(et,prefix=None):
  """Pull all plottable elements out of the element tree and return them with a series label.  A series is a list of numbers"""
  if prefix is None: 
    prefix = ""
  if et.attrib.has_key("path"):  # override the prefix with a supplied path if it exists, since that is guaranteed to be complete
    prefix = et.attrib["path"]
  else:
    prefix = prefix + "/" + et.tag  # Build the prefix from the et structure
  if et.tag in seriesTags:
      return [(prefix, et.text)]
  ret = []
  for e in et:
    t = findSeries(e,prefix)
    if t: ret += t
  return ret

def findMetrics(et,prefix=None):
  """Pull all plottable elements out of the element tree and return them with a metric label.  A metric is just a single number, while a series is a list of numbers"""
  if prefix is None: 
    prefix = ""
  if et.attrib.has_key("path"):  # override the prefix with a supplied path if it exists, since that is guaranteed to be complete
    prefix = et.attrib["path"]
  else:
    prefix = prefix + "/" + et.tag  # Build the prefix from the et structure
  ret = []
  try:  # If we can convert into a number, report it as a metric
    test = float(et.text)
    ret.append((prefix, et.text))
  except Exception, e:
    pass
  for e in et:
    t = findMetrics(e,prefix)
    if t: ret += t
  return ret

def uniquePortion(seriesNames):
    """Determine the common prefix and suffix of a list of names, and return (x,y) such that name[x:y] removes the common parts"""
    commonPfx = os.path.commonprefix(seriesNames)
    t = commonPfx.rfind("/")  # we want to split across / boundaries, not within tags. that is: commonprefix([a/bc, a/bd] -> a/b  so convert that to "a/"
    if t>=0: commonPfx = commonPfx[0:t+1]
    pfxLen=len(commonPfx)
 
    commonSfx = commonSuffix(seriesNames)
    t = commonSfx.find("/")
    if t>=0: commonSfx = commonSfx[t:]
    sfxLen=-1 * len(commonSfx)
    if sfxLen==0: sfxLen = None
    return (pfxLen,sfxLen)


class TermController(xmlterm.XmlResolver):
  """This class customizes the XML terminal"""
  def __init__(self):
    xmlterm.XmlResolver.__init__(self)
    self.tags.update(xmlterm.GetDefaultResolverMapping())
    self.curdir = "/"
    self.cmds.append(self)  # I provide some default commands
    self.xmlterm = None

  def newContext(self):
    path = self.curdir.split("/")
    return xmlterm.ParsingContext(path)

  def start(self,xt):
    """Called when the XML terminal is just getting started"""
    self.xmlterm = xt
    xt.frame.SetTitle("SAFplus CLI")
    
  def prompt(self):
    """Returns the terminal prompt"""
    return self.curdir + "> "

  def new(self):
    """Clone this controller for sub-documents"""
    return TermController()

  def completion(self,s):
    """Return the best command line completion candidate, given that the user already typed the string s"""
    if not s:
      return ""
    cmds=["!time ", "!echo ", "alias ", "!alias ", "!name","cd","get" ]
    for c in cmds:
      if c.startswith(s):
        print "complete", c
        return c[len(s):]
    return ""

  def bar(self,xml,xt):
    """Implement the bar graph command, which draws a bar graph"""
    et = ET.fromstring("<top>" + "".join(xml) + "</top>")
    metrics = findMetrics(et)
    if not metrics: return
    metricNames = [x[0] for x in metrics]
    (pfxLen,sfxLen) = uniquePortion(metricNames)
    st = ['<barGraph>']
    for s in metrics:
      if s[1]:  # If the series has some data then add it to the plot
        lbl = s[0][pfxLen:sfxLen]
        st.append('<metric label="%s">' % lbl)
        st.append(s[1])
        st.append('</metric>')
    st.append("</barGraph>")
    xt.doc.append("".join(st))  # I have to wrap in an xml tag in case I get 2 top levels from mgtGet
  

  def plot(self,xml,xt):
    """Implement the plot command -- draws a plot (line graph)"""
    et = ET.fromstring("<top>" + "".join(xml) + "</top>")
    series = findSeries(et)
    seriesNames = [x[0] for x in series]

    sz = xt.GetSize()
    sizeString = "(%s,%s)" % (sz[0]-30,sz[1]/2)
    (pfxLen,sfxLen) = uniquePortion(seriesNames)

    st = ['<plot size="%s">' % sizeString]
    for s in series:
      if s[1]:  # If the series has some data then add it to the plot
        lbl = s[0][pfxLen:sfxLen]
        st.append('<series label="%s">' % lbl)
        st.append(s[1])
        st.append('</series>')
    st.append("</plot>")
    # print "".join(st)
    xt.doc.append("".join(st))  # I have to wrap in an xml tag in case I get 2 top levels from mgtGet
 
  def do_help(self,*command):
    """? display this help, or detailed help about a particular command"""
    if command:
      command = [" ".join(command)]
    else:
      command = None
    return "<top>" + self.getHelp(command) + "</top>"

  def do_ls(self,*sp):
    """Displays the object tree at the current or specified location.
Arguments: [-N] location
By default the specified location and children are shown.  Use -N to specify how many children to show.  For example -2 will show this location, children and grandchildren.
    """
    depth=1
    rest = ""
    if len(sp):  
      if sp[0][0] == "-":  # Its a flag
        depth = int(sp[0][1:])  # depth modifier flag
        if len(sp)>2:
          rest = sp[1]
      else:             
        rest = sp[0]
    t = os.path.normpath(os.path.join(self.curdir,rest))
    gs = "{d=%s}%s" % (depth,str(t))
    print "getting ", gs
    xml = access.mgtGet(gs)
    print xml
    return "<top>" + xml + "</top>"  # I have to wrap in an xml tag in case I get 2 top levels from mgtGet

  def do_cd(self,location):
    """Change the current directory (object tree context)"""
    self.curdir = os.path.normpath(os.path.join(self.curdir,location))
    self.xmlterm.cmdLine.setPrompt()
    return ""

  def do_pwd(self,*sp):
    """Print working directory - shows the current directory"""
    return self.curdir

  def do_raw(self,*sp):
    """Equivalent to 'ls' but displays raw XML"""
    depth=1
    prefix = "{d=%s}"
    rest = ""
    flags = sp
    for flag in flags:  
      if flag[0] == "-":  # Its a flag
        if flag[1] == 'b':
          prefix = "{b,d=%s}"
        elif flag[1] == 'p':
          prefix = "{p,d=%s}"
        else:
          try:  
            depth = int(flag[1:])  # depth modifier flag
          except ValueError, e:
            xt.doc.append("bad flag: " + flag)
            pass
      else:             
        rest = flag
    t = os.path.normpath(os.path.join(self.curdir,rest))
    print (prefix % depth) + str(t)
    xml = access.mgtGet((prefix % depth) + str(t))
    txt = "<text>" + xmlterm.escape(xmlterm.indent("<top>" + xml + "</top>")) + "</text>"
    return txt # I have to wrap in an xml tag in case I get 2 top levels from mgtGet

  def do_time(self,*sp):
    """Show the time"""
    return "<time/>"

  def do_echo(self,*sp):
    """Print the args back at the user"""
    return " ".join(sp[1:])

  def do_title(self,title):
    """Change this window's title"""
    self.xmlterm.frame.SetTitle(title)
    return ""

  def do_alias(self,alias,*val):
    """Make one command be replaced by another string"""
    self.xmlterm.aliases[alias] = " ".join(val)
    return ""

  def do_exit(self):
    """Exit this program"""
    self.xmlterm.frame.Close()    

  def do_plot(self,*sp):
          """? Draw a line graph
    Arguments: locations that contain plottable data (comma separated list of numbers)
    Example:  Plot memory utilization on all components in 10 second and 1 minute time frames. 
       cd /*/safplusAmf/Component/*/procStats/memUtilization
       plot history10sec history1min         
"""
          depth=1
          rest = ""
          if len(sp):  
            if sp[0][0] == "-":  # Its a flag
              depth = int(sp[0][1:])  # depth modifier flag
              if len(sp)>1:
                rest = sp[1:]
            else:             
              rest = sp
          xml = []
          for arg in rest:
            t = os.path.normpath(os.path.join(self.curdir,arg))
            xml.append(access.mgtGet("{d=%s}%s" % (depth,str(t))))
          self.plot(xml,self.xmlterm)
          return ""

  def do_bar(self,*sp):
          """? Draw a bar graph.  A bar graph compares single values of multiple entities.  If locations do not resolve to single-number entities, the bar graph will not be shown.
    Arguments: locations (must contain a number)
    Example:  Compare number of threads in all components on all network elements
       cd /*/safplusAmf/Component/*/procStats
       bar numThreads/current
          """
          depth=1
          rest = ""
          if len(sp):  
            if sp[0][0] == "-":  # Its a flag
              depth = int(sp[0][1:])  # depth modifier flag
              if len(sp)>1:
                rest = sp[1:]
            else:             
              rest = sp
          xml = []
          for arg in rest:
            t = os.path.normpath(os.path.join(self.curdir,arg))
            xml.append(access.mgtGet("{d=%s}%s" % (depth,str(t))))
          self.bar(xml,self.xmlterm)
          return ""

  def xxexecute(self,textLine,xt):
    """Execute the passed string"""
    cmdList = textLine.split(";")
    depth = 1
    while cmdList:
      text = cmdList.pop(0) 
      sp = text.split()
      if sp:
        alias = xt.aliases.get(sp[0],None)
        if alias: # Rewrite text with the alias and resplit
          sp[0] = alias
          text = " ".join(sp)
          sp = text.split()
        # Now process the command
        if sp[0]=="ls" or sp[0] == "dir":
          rest = ""
          if len(sp)>1:  
            if sp[1][0] == "-":  # Its a flag
              depth = int(sp[1][1:])  # depth modifier flag
              if len(sp)>2:
                rest = sp[2]
            else:             
              rest = sp[1]
          t = os.path.normpath(os.path.join(self.curdir,rest))
          gs = "{d=%s}%s" % (depth,str(t))
          print "getting ", gs
          xml = access.mgtGet(gs)
          print xml
          xt.doc.append("<top>" + xml + "</top>")  # I have to wrap in an xml tag in case I get 2 top levels from mgtGet
        elif sp[0]=="plot":
          rest = ""
          if len(sp)>1:  
            if sp[1][0] == "-":  # Its a flag
              depth = int(sp[1][1:])  # depth modifier flag
              if len(sp)>2:
                rest = sp[2:]
            else:             
              rest = sp[1:]
          xml = []
          for arg in rest:
            t = os.path.normpath(os.path.join(self.curdir,arg))
            xml.append(access.mgtGet("{d=%s}%s" % (depth,str(t))))
          self.plot(xml,xt)
        elif sp[0]=="bar":
          rest = ""
          if len(sp)>1:  
            if sp[1][0] == "-":  # Its a flag
              depth = int(sp[1][1:])  # depth modifier flag
              if len(sp)>2:
                rest = sp[2:]
            else:             
              rest = sp[1:]
          xml = []
          for arg in rest:
            t = os.path.normpath(os.path.join(self.curdir,arg))
            xml.append(access.mgtGet("{d=%s}%s" % (depth,str(t))))
          self.bar(xml,xt)
        
        elif sp[0]=="cd":
          self.curdir = os.path.normpath(os.path.join(self.curdir,sp[1]))
          xt.cmdLine.setPrompt()
        elif sp[0]=='pwd':
          xt.doc.append(self.curdir)
        elif sp[0]=='raw':
          prefix = "{d=%s}"
          rest = ""
          flags = sp[1:]
          for flag in flags:  
            if flag[0] == "-":  # Its a flag
              if flag[1] == 'b':
                prefix = "{b,d=%s}"
              elif flag[1] == 'p':
                prefix = "{p,d=%s}"
              else:
                try:  
                  depth = int(flag[1:])  # depth modifier flag
                except ValueError, e:
                  xt.doc.append("bad flag: " + flag)
                  pass
            else:             
              rest = flag
          t = os.path.normpath(os.path.join(self.curdir,rest))
          print (prefix % depth) + str(t)
          xml = access.mgtGet((prefix % depth) + str(t))
          txt = "<text>" + xmlterm.escape(xmlterm.indent("<top>" + xml + "</top>")) + "</text>"
          xt.doc.append(txt)  # I have to wrap in an xml tag in case I get 2 top levels from mgtGet

        elif sp[0]=="!time": # Show the time (for fun)
          xt.doc.append("<time/>")
        elif sp[0] == '!echo':  # Display something on the terminal
          xt.doc.append(" ".join(sp[1:]))
          if 0:
           try:  # If its good XML append it, otherwise excape it and dump as text
            rest = " ".join(sp[1:])
            testtree = ET.fromstring(rest)
            xt.doc.append(rest)
           except ET.ParseError:
            xt.doc.append(xmlterm.escape(rest))
        elif sp[0] == '!name':  # Change the terminal's title
          xt.frame.SetTitle(sp[1])
        elif sp[0] == 'alias' or sp[0] == '!alias':  # Make one command become another
          xt.aliases[sp[1]] = " ".join(sp[2:])
        elif sp[0] == '!exit' or sp[0] == 'exit':  # goodbye
          self.parentWin.GetParent().frame.Close()
        else:
          pdb.set_trace()
          # TODO look for registered RPC call
          t = xmlterm.escape(" ".join(sp))
          xt.doc.append('<process>%s</process>' % t)  



def main(args):
  cmds,handlers = access.Initialize()

  if windowed:
    os.environ["TERM"] = "XT1" # Set the term in the environment so child programs know xmlterm is running
    resolver = TermController()
    resolver.addCmds(cmds)
    resolver.tags["ServiceUnit"] = serviceUnitListHandler
    resolver.tags["top"] = topHandler
    resolver.tags["more"] = childrenOnlyHandler  # don't show this indicator that the node has children
    resolver.tags["bootTime"] = epochTimeHandler
    resolver.tags["lastInstantiation"] = epochMsTimeHandler
    resolver.tags["upTime"] = elapsedSecondsHandler
    resolver.tags["pendingOperationExpiration"] = epochMsTimeHandler

    resolver.tags["history10sec"] = historyHandler
    resolver.tags["history1min"] = historyHandler
    resolver.tags["history10min"] = historyHandler
    resolver.tags["history1hour"] = historyHandler
    resolver.tags["history1day"] = historyHandler
    resolver.tags["history1week"] = historyHandler
    resolver.tags["history4weeks"] = historyHandler

    resolver.tags.update(handlers)

    resolver.depth = 0
    resolver.defaultHandler = defaultHandler
    doc = []
    app = xmlterm.App(lambda parent,doc=doc,termController=resolver: xmlterm.XmlTerm(parent,doc,termController=resolver),redirect=False,size=(600,900))
    app.MainLoop()

if __name__ == '__main__':
    main(sys.argv)


def Test():
  main([])
