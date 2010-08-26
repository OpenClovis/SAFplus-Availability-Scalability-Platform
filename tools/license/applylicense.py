import sys
import os
import os.path
import licenses


SkipDirs = [ "3rdparty", ".svn","dep","obj","test_case_files",".tmp_versions" ]

skipExt  = [ ".gcov", ".gif", ".png", ".jpg", ".bmp", ".o", ".so", ".o.cmd",".jar", ".pdf",".cap", ".swp", ".ko.cmd", ".css", ".mib",".dat", "README", "configure", ".out", ".out.po", ".pyc", "libs.mk" ]


def Replace(lst,srch,rep):
  numFiles=0
  for file in lst:
    f = open(file,"r")
    contents = f.read()
    f.close()
    if srch in contents:
      numFiles+=1
      newtext = contents.replace(srch,rep,1)
      f2 = open(file,"w")
      f2.write(newtext)
      f2.close()
      print "Processed %s" % file
    else:
      # If it looks like some kind of clovis copyright is in there, but it's not the one we are going to then oops!
      if ("OpenClovis" in contents or "Clovis Solutions" in contents) and "Copyright" in contents and not rep in contents:
        print "ERROR: Processing %s Header not found, but suspicious strings found!" % file
      else:
        pass # print "INFO:  Processing %s Header not found" % file

  return numFiles


# NOTE, .css files don't have license headers
# .mib file version headers or OLD

def EndsWith(lst, str):
  for l in lst:
    if len(str) >= len(l) and str[-1*len(l):] == l:
      return 1

  return 0  

def GetAllFiles(basedir):
  dirLst = os.walk(basedir)

  cfiles     = []
  otherfiles = []
  shellfiles = []
  xmlfiles   = []

  for (dirpath,dirname,filenames) in dirLst:
    for skip in SkipDirs:
      if skip in dirname:
        dirname.remove(skip)

    for file in filenames:
      fullname = os.path.join(dirpath, file)
      if EndsWith(skipExt,file):  # known skips
        pass 
      elif file[-2:] in [".c",".C",".h",".H",".h.in"]:
        cfiles.append(fullname)
      elif EndsWith(["Makefile", ".mk", ".mak", ".sh",".py",".properties",".ini",".in",".ac",".m4",".conf",".conf.distributed",".conf.single"],fullname):
        shellfiles.append(fullname)
      elif EndsWith([".xml",".xmi",".html",".htm",".ecore",".xml.distributed",".xml.single"],file):
        xmlfiles.append(fullname)
      else:
        print "Unknown file %s, dir %s" % (fullname, dirpath)
        otherfiles.append(fullname)

  return (cfiles,shellfiles,xmlfiles,otherfiles)

def Test(pkgdir):

  (c, sh, xml, other) = GetAllFiles(pkgdir)

  num = Replace(c, licenses.cOrig, licenses.c)
  num += Replace(sh, licenses.shellOrig, licenses.shell)
  num += Replace(xml, licenses.xmlOrig, licenses.xml)
  #ProcessOther(other)

  print "%d files changed" % num

Test(sys.argv[1])
