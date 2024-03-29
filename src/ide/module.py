import watchdog
import watchdog.observers
import watchdog.events
import yang
import os
import pdb
from types import *

import common
import entity

def cmp(a, b):
    return bool(a > b) - bool(a < b) 

def DataTypeSortOrder(a,b):
  if not type(a[1]) is dict:
    return 1
  if not type(b[1]) is dict:
    return -1
  return cmp(a[1].get("order",10000),b[1].get("order",10001))

class ModuleChangedHandler(watchdog.events.FileSystemEventHandler):
  def __init__(self):
    self.files = {}
  # def on_modified(self,event):
  #   if type(event) == watchdog.events.FileModifiedEvent:
  #     print "file modified";
  #     tmp = self.files.get(event.src_path, None)
  #     if tmp: tmp.onModuleChanged()

class Module:
  def __init__(self, filename):
    global observer
    global handler
    print(filename)
    self.filename = common.fileResolver(filename)
    realfile = os.path.realpath(self.filename)  # Chase thru symbolic links
    handler.files[realfile] = self
    # Parse the yang file into a pythonic dictionary structure
    self.ytypes, self.yobjects  = yang.go(os.getcwd(),[self.filename])
    # Now let's watch this file for changes
    observer.schedule(handler, os.path.dirname(realfile), recursive=False)
    # Parse the yang module into entity types
    self.entityTypes = {}
    for i in list(self.yobjects.items()): # Load the top level defined entities (like node, sg)
      if i[1].get("ui-entity",False):
        if i[0] == "Component":
          i[1]['csiTypes'] = set()
        self.entityTypes[i[0]] = entity.EntityType(i[0],i[1])  # create a new entity type, with the member fields (located in i[1])

  def delete(self):
    """Stop using this module"""
    global handler
    realfile = os.path.realpath(self.filename)  # Chase thru symbolic links
    del handler.files[realfile]  # Clean up this reference
    self.entityTypes = None

  def onModuleChanged(self):
    """If the module file changes, we need to reload it."""
    print("module changed")  

        
handler = ModuleChangedHandler()
observer = watchdog.observers.Observer()
observer.start()

def Test():
  import time
  m = Module("SAFplusAmf.yang")
  time.sleep(1000);
