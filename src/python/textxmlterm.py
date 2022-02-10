import pdb, sys, traceback,re
import types
from xml.sax.saxutils import escape,unescape
import xml.etree.ElementTree as ET
try:
  import readline
except:
  pass

DropToDebugger=False

def FancyText(unused,string,size=0,fore=0, back=0):
  return string

class CommandBadArgException(Exception):
  pass


class CmdLine:
  def __init__(self,resolver):
    self.resolver = resolver
    self.prompt = "> "
  def setPrompt(self):
    self.resolver.path = self.resolver.curdir.split("/")
    
  def input(self):
    self.prompt = self.resolver.curdir + "> "
    readline.parse_and_bind("tab:complete")
    readline.set_completer_delims("")
    readline.set_completer(self.resolver.completion)
    return input(self.prompt)

class Doc:
  def __init__(self,resolver):
    self.resolver = resolver
  def append(self,data):
    if data is None: return
    try:  # If its good XML append it, otherwise escape it and dump as text
      tree = ET.fromstring(data)
    except ET.ParseError:
      tree = escape(data)
    self.resolver.resolve(tree,self.resolver)

class XmlResolver:
  def __init__(self):
    self.tags = {}
    self.cmds = []
    self.doc=Doc(self)
    self.aliases = {}
    self.cmdLine = CmdLine(self)
    self.parentWin = None
  
    self.frame = self   

    # Context objects
    self.path = [""]
    self.depth = 0
    #self.xmlterm = self # just dumping everything in one class

  def Close(self):
    exit(0)

  def resolve(self,tree,context):
    """Figure out the appropriate handler for this element, and call it"""
    tag = tree.tag
    lookupDict = self.tags
    # tag has a namespace.  Support a heirarchial dictionary by looking up the namespace first to see if it is in there with a subdictionary.  If its not I just look the tag up in the main dictionary
    if tree.tag[0] == "{": 
      namespace, tag = tree.tag[1:].split("}")
      lookupDict = lookupDict.get(namespace,lookupDict) # Replace the dictionary with the namespace-specific one, if such a dictionary is installed
    handler = lookupDict.get(tag, self.defaultHandler)
    handler(tree,self,context)

  def add(self,string):
    print (string)

  def addCmds(self,cmds):
    if cmds:
      cmds.setContext(self)
      self.cmds.append(cmds)

  def bindCmd(self,cmds,lst):
    """This function takes a nested dictionary which maps command to implementation function, and a list of user input tokens.  It attempts to bind the user input to a command in the dictionary and returns:
    ((function, completion_function), [args],{kwargs}) if it succeeds or
    (None, None, None) if there is no binding
    """
    idx = 0
    out = cmds.get(lst[0],None)
    if out is not None:
      if type(out) is types.DictType:  # Recurse into the next level of keyword
        ret = self.bindCmd(out,lst[1:])
        return ret
      else:
        if out[0].__name__ == "handleRpc":
          return (out,lst,None)  # TODO keyword args
        else:
          #print"bbb"
          return (out,lst[1:],None)  # TODO keyword args

    default = cmds.get(None,None)  # If there's a dictionary entry whose key is None, then this is used if nothing else matches
    if default:
      return (default,lst, None) # TODO keyword args
    return (None, None,None)

  def execute(self,textLine,xmlterm):
    """Execute the passed string"""
#   try:
    cmdList = textLine.split(";")
    while cmdList:
      text = cmdList.pop(0)
      pattern = re.compile(r'''((?:[^\s"']|'[^']*')+)''')
      sp = pattern.split(text)[1::2]
      if sp:
        alias = xmlterm.aliases.get(sp[0],None)
        if alias: # Rewrite text with the alias and resplit
          sp[0] = alias
          text = " ".join(sp)
          sp = pattern.split(text)[1::2]
        output = self.executeOne(sp,xmlterm)
        if isinstance(output, int):
          return output
        

  def executeOne(self,sp,xmlterm):
        fn_name = "do_" + sp[0]
        for cmdClass in self.cmds:
          if hasattr(cmdClass,"commands"):
            (fns,args,kwargs) = self.bindCmd(cmdClass.commands,sp)
            if fns: 
              (binding,completion) = fns
              if binding:
                if kwargs:
                  output = binding(*args,**kwargs)
                else:
                  output = binding(*args)
                if output is not None: # None means to continue looking for another command
                  if output:  # Commands will return "" if they have no output
                    xmlterm.doc.append(output)
                  return
              if fn_name=="do_connect": # it has been handled at handleRpc function of Command object
                return

          if hasattr(cmdClass,fn_name):
            try:
              # print "executing:", str(cmdClass.__class__), fn_name
              output = getattr(cmdClass,fn_name)(*sp[1:])
              if output is not None:  # None means keep looking -- I did not execute a command
                if output:   # "" means command worked but nothing output
                  if isinstance(output, int):
                    return output
                  xmlterm.doc.append(output)
                return
            except TypeError as e:  # command had incorrect arguments or something
              if DropToDebugger:
                type, value, tb = sys.exc_info()
                traceback.print_exc()
                last_frame = lambda tb=tb: last_frame(tb.tb_next) if tb.tb_next else tb
                frame = last_frame().tb_frame
                pdb.post_mortem()
              # TODO: print the command's help and try to hint at the problem
              print ('Error:', e)
              #xmlterm.doc.append("<error>" + str(e) + "</error>")
              return

        if sp[0] == '!exit' or sp[0] == 'exit':  # goodbye
          exit(0)
        elif sp[0] == 'alias' or sp[0] == '!alias':  # Make one command become another
          xmlterm.aliases[sp[1]] = " ".join(sp[2:])
        elif sp[0] == 'rpc' or sp[0] == '!rpc':
          return
        else:
          print ("Unknown command [%s]" % sp)

def indent(elem,depth=0):
  if type(elem) in types.StringTypes:
    try:
      elem = ET.fromstring(elem)
    except ET.ParseError as e: # Its bad XML so just do something simple that breaks up lines
      return elem.replace(">",">\n")
  ret = ["  "*depth + "<" + elem.tag]
  for a in elem.attrib.items():
    ret.append(" " + a[0] + "=" + '"' + a[1] + '"')
  ret.append(">")
  if elem.text: ret.append(elem.text)
  if len(elem):
    ret.append("\n")
    for c in elem:
      ret.append(indent(c,depth+1))
  if elem.tail: ret.append(elem.tail)
  ret.append("</" + elem.tag + ">")
  ret.append("\n")
  return "".join(ret)
  

def GetDefaultResolverMapping():
  return {}
