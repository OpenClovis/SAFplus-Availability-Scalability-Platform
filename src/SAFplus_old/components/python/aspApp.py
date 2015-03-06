"""@namespace aspApp

Application bundle manager logic.
This module opens applications bundles, parses the appcfg.xml file and presents all available application bundles as a hierarchial set of Python objects.
"""
import pdb

import os
import shutil
from os.path import join, getsize
import tempfile
from datetime import *

import pexpect
import psh

import xml2dict
from dot import *
from misc import *
from aspLog import log

from clusterinfo import *
import aspAmf



#def Log(x):
#  print x

TEMP_DIR = os.getenv("ASP_DIR",None)
if not TEMP_DIR: 
  TEMP_DIR = "/tmp/openclovis"
else:
  TEMP_DIR = TEMP_DIR + os.sep + "var" + os.sep + "tmp"

#TEMP_DIR = "/tmp/openclovis/"

APP_CFG_FILE = "appcfg.xml"
APP_CFG_FILE_VER = (1,0,0,0)

AppArchiveDir = os.getenv("ASP_DIR",None)
if not AppArchiveDir:
  AppArchiveDir = tempfile.mkdtemp("","",TEMP_DIR)
  if not os.access(AppArchiveDir,os.F_OK):
    os.makedirs(AppArchiveDir)

AppArchiveDir = AppArchiveDir + os.sep + "apps"


class AppFile:
  """Represents a single archive file that is a particular application version.
  """

  # Member variables
  ## @var name
  #  Filename and path of the bundle file

  ## @var appName
  #  Name of the application (as specified in appcfg.xml)

  ## @var version
  #  Version of the application (as specified in appcfg.xml)

  ## @var archive
  #  Filename and path of the bundle file (same as the name member variable)

  ## @var dir
  #  Location of the exploded bundle

  ## @var uploadDate
  #  When was this bundle added?

  ## @var cfg
  #  The default configuration as specified in appcfg.xml

  ## @var sg
  #  List of Service Groups that are using this software

  ## @var app
  #  The bundle is a particular version of the app referenced here.


  def __init__(self,file):
    """Constructor
    @param file The filename of the bundle that this object will represent
    """
    self.name    = file 
    self.appName = None
    self.version = None
    self.archive = file
    self.dir     = None
    self.uploadDate = datetime.now()
    self.cfg     = None
    self.sg      = []
    self.app     = None

  def Delete(self):
    """Remove this application file, and all running instances"""

    # TODO: Shut down and delete all SGs

    # Remove me from disk
    try:
      os.remove(self.archive)
    except OSError:
      pass

    shutil.rmtree(self.dir)

    # Remove me from the DB
    self.app.appDb.Remove(self.name)

    del(self.app.appFiles[self.name])
    if len(self.app.appFiles) == 0:  # If this was the last version, bye bye app!
      self.app.Delete()

  def Nodes(self):
    """Show the nodes running instances of this application"""
    nodes = set()
    for sg in self.sg:
      for su in sg.su:
          if su.node:
            nodes.add(su.node)
    return nodes

  def ParseAppCfg(self, file):
    """Private: Parse and return the application configuration file"""
    dict = xml2dict.decode_file(file)
    self.NormalizeCfg(dict)
    return dict

  def Open(self, tgtdir=None):
      """Detar an application tar file and verify its contents

      Parameters:
        tgtdir:       Where you want the archive opened (if not passed, I will make a temporary)

      Exceptions:
        misc.Error:   Raised when shell commands return error codes

      Returns: (tgtdir, cfg)
        tgtdir:       Where the archive was opened
        cfg:          Application configuration
      """

      # Massage filename to be always right
      filename = os.path.abspath(self.archive)

      if not tgtdir:
        # Make temporary dir for uncompression
        try:
          os.makedirs(TEMP_DIR)
        except OSError,e:
          pass # [Errno 17] File exists: '/tmp/hi' is ok
        tgtdir = TEMP_DIR + os.sep + os.path.basename(self.archive)
        try:
          os.makedirs(tgtdir)
        except OSError,e:
          pass # [Errno 17] File exists: '/tmp/hi' is ok
        
        # tgtdir = tempfile.mkdtemp("","",TEMP_DIR)
      else:
        # Make the requested directory 
        try:
          os.makedirs(tgtdir)
        except OSError,e:
          pass # [Errno 17] File exists: '/tmp/hi' is ok

      self.dir = tgtdir

      # open the app file
      output = system('(cd "%s"; tar xvfz "%s")' % (tgtdir,filename))

      # Parse the app config file
      cfgFile = tgtdir + os.sep + APP_CFG_FILE
      self.cfg = self.ParseAppCfg(cfgFile)

      # Currently only one app per file is allowed
      cfgdict = self.cfg.values()[0]
      self.version = cfgdict.get('version', cfgdict.get('ver', '0.0.0.0'))
      self.appName = self.cfg.keys()[0]

      return(self.dir,self.cfg)


  def NormalizeCfg(self, cfg=None):
    """Internal function: Fills in defaults if some configuration is not included.
       Converts empty strings to None.
    """
    if cfg is None: cfg = self.cfg
    for (app,acfg) in cfg.items():

      # Verify the basic required fields
      assert(acfg.has_key("programNames"))
      assert(type(acfg.programNames) is InstanceType)
      assert(acfg.has_key("redundancyModel"))
      assert(acfg.has_key("totalNodes"))

      # Supply the optional appDir parameter if it is not supplied
      if not acfg.has_key("appDir"):
          acfg['appDir'] = app + "_" + acfg.ver

      # Make sure the modifiers section is as expected
      if acfg.has_key("modifiers"):
        if type(acfg.modifiers) in StringTypes:
              acfg['modifiers']={}
        assert((type(acfg.modifiers) is DictType) or (type(acfg.modifiers) is InstanceType))
      else:
        acfg['modifiers']={}

      # Verify that the work definitions are ok.
      if acfg.has_key("work"): 
        if type(acfg.work) in StringTypes: acfg.work = None
        else:
          for (wname,kvdict) in acfg.work.items():  # If the kvdict is empty (a string) then make it None
            if type(kvdict) in StringTypes:
              acfg.work[wname]={}
                  
      else:
        cfg["work"] = None # No work defs? So set work to None


class App:
  """An application may consist of a bunch of application files, each corresponding
  to a different version of the application.
  """

  # Member variables
  ## @var name
  #  Unique identifier for this object (in this case the name of the application from appcfg.xml)

  ## @var sg
  #  List of Service Groups that are using this application

  ## @var appFiles
  #  List of software bundles that contain versions of this application
  
  ## @var version
  #  Dictionary of available versions (@ref AppFile) indexed by version string

  ## @var appDb
  #  Back-pointer to the application database


  def __init__(self,name,appDb):
    self.name = name
    self.sg = []
    self.appFiles = {}
    self.version = {}
    self.appDb = appDb

  def AddAppFile(self,af):
    """Add an application file object to this application
    @param af @ref AppFile object
    """
    self.appFiles[af.archive] = af
    self.version[af.version] = af
    
  def Versions(self):
    """Return a list of all versions of this application that I have"""
    ret = []
    for af in self.appFiles.values():
      ret.append(af.version)
    ret.sort()
    ret.reverse()
    return ret

  def Delete(self):
    """Remove this application, all running instances, and all archive files"""
    # Remove all appFiles
    for af in self.appFiles.values():
      af.Delete()

    self.appDb.Remove(self.name)


  def LatestVersion(self):
    """Return the application file that contains the latest version"""
    best = None
    for af in self.appFiles.values():
      if not best or af.version > best.version:
        best = af

    return best

  def Nodes(self):
    """Return the nodes that are currently running this application"""
    nodes = set()
    for sg in self.sg:
      for su in sg.su:
        if su.node: nodes.add(su.node)
    return nodes


class AppDb:
  """All applications and application versions and archives that are currently on the system.  Note that you should only create 1 of these per bundle repository directory or you will have 2 entities managing the same data.
  """

  # Member variables

  ## @var apps
  #  List of applications in the application database

  ## @var appdir
  #  Location where the application bundle files are stored by default
  
  ## @var entities
  #  A dictionary of all subobject, indexable by name (either the filename in the case of appFile objects, or the application name for App objects.

  def __init__(self,dbdir=AppArchiveDir):
    ## @var apps A dictionary of available applications
    self.apps = {}
    self.appdir = dbdir
    ## @var entities A dictionary of all available app versions
    self.entities = {}
    try:
      os.makedirs(dbdir)
    except OSError,e:
      if e.errno != 17: raise  # 17 is dir already exists
    self.ReloadApps()

  def Remove(self,name):
    """Remove an entity from the appDb, but do not delete it.
    To delete it, use the entities' Delete member.
    @param name String name of application to remove
    """
    if self.entities.has_key(name):
      del self.entities[name]
    if self.apps.has_key(name):
      del self.apps[name]

  def AppList(self):
    """Return a list of all application objects"""
    return self.apps.values()

  def __iter__(self):
    """Iterate through all application objects"""
    return self.apps.values().__iter__()

  def NewAppFileFromFd(self,filename, fs):
    """This is a fairly special-purpose function used when you have a file stream, but not an actual file.  This happens through a web uploads"""
    # Copy the <file> to a real name so I can detar it using shell calls
    fp = open(AppArchiveDir + os.sep + filename,"wb")

    done = False
    while not done:
      buf = fs.read(65536)
      fp.write(buf)
      if len(buf) != 65536: done = True
    fp.close()

    af = self.NewAppFile(AppArchiveDir + os.sep + filename)
    return af


  def NewAppFile(self,filename):
      """Tell the database about a new application file.  The file should be in the AppArchiveDir, or it will not be found during a reload.
      @exception misc.Error  The file does not exist (error code is 2)
      @return application file object (@ref AppFile)
      """
      af = AppFile(filename)
      af.Open()
      app = self.apps.get(af.appName,None)
      if not app: 
        app = App(af.appName,self)
        self.apps[app.name] = app
        self.entities[app.name] = app
      app.AddAppFile(af)
      af.app = app

      self.entities[af.name] = af

      return af

  def ConnectToServiceGroups(self,cinfo = None):
    """ Iterate through the clusterinfo service groups and connect them up to apps and appfiles
    """

    if cinfo is None: cinfo = ci
  
    # Clear out the old connections
    for e in self.entities.values():
      e.sg = []

    log.info("Connecting App Database to existing Service Groups")
    for sg in cinfo.sgList:
      if sg.associatedData:
        log.info("Service Group %s, data: %s" % (sg.name, sg.associatedData))
        try:
          [appName, appVer] = sg.associatedData.split()
        except ValueError:  # The data isn't in the right format, so it is probably not for me
          log.info("Improperly formatted entity associated data [%s].  Expecting 'appName version'." % (sg.associatedData))
          continue
          #appName = None # skip the rest of the logic for this SG
      
        app = self.entities.get(appName,None)
        if app:
          log.info("App %s found" % (app.name))
          app.sg.append(sg)
          appFile = app.version.get(appVer,None)
          sg.app = app
          if appFile:
            log.info("AppFile %s found" % (appFile.name))
            appFile.sg.append(sg)
            sg.appVer = appFile
      else:
        log.info("Service Group %s, no app data" % (sg.name))



  def ReloadApps(self):
    """Reload the entire DB from archive files in the AppArchiveDir"""
    self.apps = {}  # Clear the old objects
    
    # Find all .tgz files in the app dir and process them
    for base, dirs, files in os.walk(self.appdir):

      # Blow away all the temporary directories
      for d in dirs:
        os.removedirs(base + os.sep + d)

      for f in files:
        (noext, ext) = os.path.splitext(f)
        if ext == ".tgz" or ext == ".tar.gz":
          try:
            self.NewAppFile(base + os.sep + f)
          except Error, e:  # Bad file
            log.info("Bad archive [%s] error: [%s]" % (base + os.sep + f, str(e)))
            os.remove(base + os.sep + f)
          except xml2dict.BadConfigFile, e:
            log.info("Bad archive [%s] error: [%s]" % (base + os.sep + f, str(e)))

      break # Actually I dont want a recursive walk.

    self.ConnectToServiceGroups()


def RemoveDefaultTemporaryBundleDir():
  """To be called when you are certain that no appDb instances exist to clean up from prior runs.  Or just rm -rf /tmp/openclovis/"""
  try:
    shutil.rmtree(TEMP_DIR)
  except OSError:
    pass
