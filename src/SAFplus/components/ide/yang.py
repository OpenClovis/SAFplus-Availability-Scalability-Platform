import sys
import os
import re
import pdb

# export PYTHONPATH=/code/git/mgt/3rdparty/pyang
import sys
sys.path.append("/code/git/mgt/3rdparty/pyang")
import pyang
from pyang import plugin
from pyang import error
from pyang import util
from pyang import hello
from pyang import statements

def go(path,filenames):
  repos = pyang.FileRepository(path)
  ctx = pyang.Context(repos)

  ctx.canonical = None
  ctx.max_line_len = None
  ctx.max_identifier_len = None
  ctx.trim_yin = None
  ctx.lax_xpath_checks = None
  ctx.deviation_modules = []
  
  # make a map of features to support, per module
  ctx.features = {}

  ctx.validate()

  modules = []
  if 1:
        r = re.compile(r"^(.*?)(\@(\d{4}-\d{2}-\d{2}))?\.(yang|yin)$")
        for filename in filenames:
            try:
                fd = open(filename)
                text = fd.read()
            except IOError as ex:
                sys.stderr.write("error %s: %s\n" % (filename, str(ex)))
                sys.exit(1)  # TODO
            m = r.search(filename)
            if m is not None:
                (name, _dummy, rev, format) = m.groups()
                name = os.path.basename(name)
                module = ctx.add_module(filename, text, format, name, rev, expect_failure_error=False)
            else:
                module = ctx.add_module(filename, text)
            if module is None:
                exit_code = 1  # TODO
            else:
                modules.append(module)

  
  modulenames = []
  for m in modules:
        modulenames.append(m.arg)
        for s in m.search('include'):
            modulenames.append(s.arg)
  
  ctx.validate()
  #dumpContext(ctx)

  ytypes = {}
  yobjects = {}
  dictify(ctx,ytypes, yobjects)
  #print str(ytypes)
  #print str(yobjects)
  #pdb.set_trace()
  return(ytypes,yobjects)

def getArg(stmts,keyword,default=None):
  """ Given a keyword and a list of statements or a pyang statement object, returns the arg"""
  if type(stmts) is pyang.statements.Statement: stmts = stmts.substmts
  for s in stmts:
    if s.keyword == keyword:
      return s.arg
  return default

def getChildren(stmts, keyword ):
  """ Given a keyword and a list of statements or a pyang statement object, returns a list of matching substatements"""
  if type(stmts) is pyang.statements.Statement: stmts = stmts.substmts
  for s in stmts:
    if s.keyword == keyword:
      yield s
  

def getChild(stmts,keyword):
  for s in getChildren(stmts,keyword): return s

def intOrNone(x): 
  if x is None: 
    return None 
  else: return int(x)

def createLeaf(s,count, result=None):
  if result is None: result = {}
  result[s.arg] = { "order":count, "type": getArg(s,"type"),"help" : getArg(s,"description",None), "alias": getArg(s,"safplus:alias",None), "prompt":getArg(s,"safplus:ui-prompt",None) }
  return result
  

def createObject(s,result=None):
  if result is None: result = {}
  count = 0
  for c in s.substmts:
    count+=1
    if getArg(c,"config", True):  # If the config field does not exist or is true, this is configuration
      if c.keyword == "leaf": createLeaf(c, count, result)
      elif c.keyword == "leaf-list":
        pass  # TODO
      elif c.keyword == "list":
        pass  # TODO
      elif c.keyword == "container":
        pass  # TODO
      elif c.keyword == "uses":
        pass  # TODO
      else:
        pass # TODO 
  return result

def dictifyStatements(stmts,ts,objs,indent=0):
    for s in stmts:
      #print " "*indent, "keyword: ", s.keyword, " Arg: ", s.arg

      # Handle Typedef types
      if s.keyword == "typedef":  # Add typedefs to the types dictionary (ts)
        typ = getArg(s,"type")
        ts[s.arg] = { "type": typ, "help": getArg(s,"description"), "source": (s.pos.ref,s.pos.line) }

        # Its an enum, so fill the children with the enum choices
        if typ == "enumeration":  
          es = getChild(s,"type") # there will only be one
          d = {}
          count = 0  # I need to store an ordering so it can be displayed in the same ordering as was in the file.  Presumably that is the preferred order.
          for c in getChildren(es,"enum"):
            d[c.arg] = { "order": count, "value" : intOrNone(getArg(c,"value",None)), "help" : getArg(c,"description",None), "alias": getArg(c,"safplus:alias",None)}
            count += 1

          ts[s.arg]["values"] = d
          ts[s.arg]["default"] = getArg(es,"default")

        # Its a uint32 number
        elif typ == "uint32":
          pass  # Nothing to do now but what about constraints?
        # TODO all other types

      # Handle grouping types... this should essentially create a dictionary containing the subobjects
      elif s.keyword == "grouping":
        pass
      # Handle Lists -- YANG lists are essentially arrays of the same type object; these objects are modeling candidates
      elif s.keyword == "list":
        d = {}
        objs[s.arg] = createObject(s,d)

      if s.substmts:
        dumpStatements(s.substmts, indent+2)

def dictify(ctx,ts,os):
  #print "Modules:"
  for m in ctx.modules.items():
    print m[0]

  #print "Objects:"
  for module in ctx.modules.items():
    m = module[1]
    dictifyStatements(m.substmts,ts,os,0)


def dumpStatements(stmts,indent=0):
    for s in stmts:
      print " "*indent, "keyword: ", s.keyword, " Arg: ", s.arg
      if s.substmts:
        dumpStatements(s.substmts, indent+2)

def dumpContext(ctx):
  print "Modules:"
  for m in ctx.modules.items():
    print m[0]

  print "Objects:"
  for module in ctx.modules.items():
    m = module[1]
    dumpStatements(m.substmts,0)

def Test():
  import os
  go(os.getcwd(), [ "SAFplusAmf.yang"])

