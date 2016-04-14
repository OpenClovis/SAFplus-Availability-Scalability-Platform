import time
import pdb
import sys
sys.path = ["../../yadog"] + sys.path
sys.path.append("doclayout")
import yadog
import PyHtmlGen.imagehtml


def main(dirs):
  PyHtmlGen.imagehtml.SetImagePathPrefix("./")

  cfg={"project":{'name':'SAFplus Availability Scalability Platform','version':'7.0.0','date':time.strftime("%d %b, %Y"),'author':"OpenClovis, Inc.",'homepage':'www.openclovis.com'},
     "sections":["section","file","class","fn","var"],
     "html":
       {
       "dir":"doclayout",
       "skin":"docskin",
       "sectionPageImplementers": {"class":"jsclasspage","file":"jsfilepage"},
       "sectionIndexImplementers": { "Class":"jsclassindexpage","File":"jsfileindexpage","Section":"jssectionindexpage"},
       "nav": [ ("Home","jshome"),("Examples","jstoc",("Examples",{"tag_":"example"},"example.xml")), ("Search","jssearch"),{"name":"Index","gen":"jsindex","file":"idx"} ],
       # "quicklists": {"History":"jsqhist","Sections":"jsqsec","Classes":"jsqclass","Files":"jsqfile"},
       "misc": { "frame":"jsframe"}
       }
     }
  

  #yadog.genPaths(["../src"],["3rdparty","target"], cfg,"SAFplus/")
  yadog.genPaths(dirs,["3rdparty","target","SAFplus_old", "genshi"], cfg,"SAFplus/")

def Test():
  
  main(["../src", "../examples"])
  #main(["test"])

if __name__ == "__main__":
  yadog.DropToDebugger = False
  main(["../src","../examples"])

