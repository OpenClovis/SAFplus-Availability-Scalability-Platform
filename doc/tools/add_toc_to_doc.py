#!/usr/bin/python

import sys
#import sre
import re
import pdb

def create_toc(doc, docid):

   toc = []
      
   for i, docline in enumerate(doc):
      if (('\page' in docline) or ('\section' in  docline) or \
         ('\subsection' in docline) or ('\subsubsection' in  docline) or \
         ('\paragraph' in docline)): 
          mo_doc = re.search("sec_[0-9]+_*[0-9]*_*[0-9]*_*[0-9]*_*[0-9]*", docline)
          if mo_doc != None:
              reference = docline[mo_doc.start():mo_doc.end()]
              numbering = re.sub('sec_','',reference)
              reo_doc = re.split('_', numbering)

              if (len(reo_doc) == 1):
                  if re.search("Preface", docline):
                     tocline = '\n\\ref %s_%s\n' % (docid, reference)
                  elif re.search("Appendix", docline):
                     tocline = '\n\\ref %s_%s\n' % (docid, reference)
                  else:
                     tocline = '\n<b>Chapter %s</b> \\ref %s_%s\n' % (reo_doc[0], docid, reference)
              elif (len(reo_doc) == 2): 
                  tocline = '- \\ref %s_%s\n' % (docid, reference)      
              elif (len(reo_doc) == 3): 
                  tocline = '   - \\ref %s_%s\n' % (docid, reference)      
              elif (len(reo_doc) == 4): 
                  tocline = '      - \\ref %s_%s\n' % (docid, reference)      
              elif (len(reo_doc) == 5): 
                  tocline = '         - \\ref %s_%s\n' % (docid, reference)      
              toc.append(tocline)
              # print docline
              # print tocline
   return toc
              
doctitle= sys.argv[1] 
format  = sys.argv[2] 

in_doc  = open(sys.argv[3], 'r')
doc     = in_doc.readlines()
docid   = re.sub('\.txt','',sys.argv[3])
toc     = create_toc(doc, docid)

out_doc = open(sys.argv[4], 'w')

out_doc.write('/**\n')
out_doc.write('\\anchor %s\n' % docid)
out_doc.write('\\mainpage %s\n\n' % doctitle)

if (docid == 'tutorial' or docid == 'installguide' or
    docid == 'relnotes' or docid == 'safcompliance' or
    docid == 'ideguide' or docid == 'logtoolguide' or
    docid == 'sdkguide' or docid == 'evalguide') : 
    out_doc.write('Back to <a href=\"../index.html\">OpenClovis SDK Documentation Main Page</a>\n\n')

out_doc.write('<b>Table of Contents: </b>\n\n')
out_doc.writelines(toc)

if format == 'multipage1' or format == 'multipage2' or format == 'multipage3':
   out_doc.write('\n*/\n');
else:
   out_doc.write('\n\n\n');
   
for i, docline in enumerate(doc):
   out_doc.write(docline)
   
   if '\page' in docline: 
       mo_doc = re.search("sec_[0-9]+_*[0-9]*_*[0-9]*_*[0-9]*", docline)
       if mo_doc != None :
           numbering = docline[mo_doc.start():mo_doc.end()]
           # numbering = re.sub('sec_','',numbering)
           numbering = numbering + '_'
           # print 'Pattern: %s' % numbering
          
           first_in_toc = 1
           for j, tocline in enumerate(toc):
              mo_toc = re.search(numbering, tocline)
              if mo_toc != None :
                  if first_in_toc:
                     out_doc.writelines('<b>Table of Contents: </b>\n\n') 
                     first_in_toc = 0                 
                  out_doc.writelines(tocline)
           if (first_in_toc == 0):
              # out_doc.writelines('\n \\htmlonly <hr color="#84B0C7" style="color:#84B0C7;"> \\endhtmlonly \n')  
              # does not work to color line, therefore I will use this:   
              out_doc.writelines('\n \\htmlonly <h2></h2>\\endhtmlonly \n')   
 
out_doc.close()
in_doc.close()

