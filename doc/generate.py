import time
import sys
sys.path.append("../../yadog")
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
       "pageimplementers": {"class":"jsclasspage","file":"jsfilepage"},
       "nav": [ ("home","jshome"),("Examples","jssection",("Examples",)), ("search","jssearch"),("idx","jsindex") ],
       "indeximplementers": { "class":"jsclassindexpage","file":"jsfileindexpage","section":"jssectionindexpage"},
       # "quicklists": {"History":"jsqhist","Sections":"jsqsec","Classes":"jsqclass","Files":"jsqfile"},
       "misc": { "frame":"jsframe"}
       }
     }
  

  #yadog.genPaths(["../src"],["3rdparty","target"], cfg,"SAFplus/")
  yadog.genPaths(dirs,["3rdparty","target","SAFplus_old", "genshi"], cfg,"SAFplus/")

def Test():
  
  main(["../src"])
  # main(["test"])

if __name__ == "__main__":
  main(["../src"])

