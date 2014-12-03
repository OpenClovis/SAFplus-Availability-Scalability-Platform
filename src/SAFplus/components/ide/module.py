import watchdog
import watchdog.observers
import watchdog.events
import yang
import os
import pdb

import common
import entity

class ModuleChangedHandler(watchdog.events.FileSystemEventHandler):
  def __init__(self):
    self.files = {}
  def on_modified(self,event):
    pdb.set_trace()    
    if type(event) == watchdog.events.FileModifiedEvent:
      print "file modified";
      tmp = self.files.get(event.src_path, None)
      if tmp: tmp.onModuleChanged()

class Module:
  def __init__(self, filename):
    global observer
    global handler
    self.filename = common.fileResolver(filename)
    realfile = os.path.realpath(self.filename)  # Chase thru symbolic links
    handler.files[realfile] = self
    self.ytypes, self.yobjects  = yang.go(os.getcwd(),[self.filename])
    observer.schedule(handler, os.path.dirname(realfile), recursive=False)
    self.entityTypes = {}
    for i in self.yobjects.items():
      if i[1].get("ui-entity",False):
        self.entityTypes[i[0]] = entity.EntityType(i[0],i[1])

  def delete(self):
    """Stop using this module"""
    global handler
    realfile = os.path.realpath(self.filename)  # Chase thru symbolic links
    del handler.files[realfile]  # Clean up this reference
    self.entityTypes = None

  def onModuleChanged(self):
    """If the module file changes, we need to reload it."""
    print "module changed"  

        
handler = ModuleChangedHandler()
observer = watchdog.observers.Observer()
observer.start()

def Test():
  import time
  m = Module("SAFplusAmf.yang")
  time.sleep(1000);
