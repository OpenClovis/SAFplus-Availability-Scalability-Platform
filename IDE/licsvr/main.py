#!/usr/bin/env python

# Test:
# python2.5 /code/google_appengine/dev_appserver.py .

# Upload:
# /code/google_appengine/appcfg.py update .
# ../appcfg.py update .


# Python imports
import os
import time
from hashlib import md5
import tarfile
from StringIO import StringIO
from datetime import datetime, date


# WebApp imports
from google.appengine.ext.webapp import template
from google.appengine.api import mail

from google.appengine.ext import webapp
from google.appengine.ext.webapp import util
from google.appengine.ext.webapp.util import run_wsgi_app
from google.appengine.api import users
from google.appengine.ext import db

# application imports
from usermgt import *
from adminmgt import *
from svrcommon import *
from constants import *
import codegen.openSAF 

class GeneratedCode(db.Model):
    email = db.StringProperty()    
    tgzfile = db.BlobProperty()
    created = db.DateTimeProperty(auto_now_add=True)


# --------------------------------------------
# Pages

class MainHandler(webapp.RequestHandler):
    def get(self):
        #self.response.headers['Content-Type'] = "text/html"
        f = open("templates/index.html","r")
        page = f.read()
        self.response.out.write(page)
        #self.redirect("/templates/index.html")

class GetFileHandler(webapp.RequestHandler):
    def get(self,blob):
        #self.response.out.write(blob)
        k = db.Key(blob)
        fil = db.get(k)
        self.response.headers['Content-disposition'] = "attachment; filename=generatedModel.tgz"
        self.response.headers['Content-Type'] = "application/x-tar"
        self.response.out.write(fil.tgzfile)



class GenerateHandler(webapp.RequestHandler):    
    def get(self):
        #user = AAA(self,GenerateHandler)
        self.response.headers['Content-Type'] = "text/html"
        self.redirect("/static/generate.html")
        pass
    def post(self):
        #user = AAA(self,GenerateHandler)
        username = self.request.get("username")
        pw = self.request.get("password")
        userRecord = validate_login(username,pw)

        # Make sure the user is known
        if not userRecord:
            self.response.out.write(basicTemplate(INVALID_LOGIN,"login invalid","Login information incorrect, or unverified."))
            return

        # Limit the number of generations
        if userRecord.remainingGenerations < 1:
            self.response.out.write(basicTemplate(TRIAL_EXPIRED,"trial expired",'Your trial has expired.  Please contact Openclovis.'))
            
            # send email to site maintainer
            email_subj = "A user's trial has expired"
            email_body = "Username: " + username
            
            send_email(SiteMaintainer, email_subj, SiteMaintainer, email_body)
            
            return
        
        mdlFileStr = self.request.get("model")
        #logging.info(self.request.arguments())
        #print mdlFileStr
        if not mdlFileStr:
            self.response.out.write(basicTemplate(INVALID_FIELDS,'No model file supplied','You forgot to fill in the model file upload dialog box.  Please go back and try again.'))
            return
        mdlFileName = self.request.POST[u"model"].filename        
        #logging.info("Model filename: %s" % str(mdlFileName))
        mdlName = os.path.basename(mdlFileName).split(".")[0]
        logging.info("Model name: %s" % mdlName)
        mdlFileObj = StringIO(mdlFileStr)
        output = StringIO()
        test = codegen.openSAF.cvtModel(mdlFileObj,output,mdlName)
        blob = output.getvalue()

        userRecord.remainingGenerations -= 1
        userRecord.codeGensUsed += 1
        userRecord.dateOfLastCodeGen = datetime.now()
        userRecord.put()

        gc = GeneratedCode()
        gc.email = userRecord.email
        gc.tgzfile = blob
        gc.put()
        k = gc.key()
        self.response.out.write(basicTemplate(OK,"<a href='/getfile/%s'>download</a>" % str(k),"Success! Download your OpenSAF generated code <a href='/getfile/%s'>here</a>" % str(k)))
        #self.response.out.write("Success! Download <a href='/getfile/%s'>here</a>" % str(k))
        #self.response.out.write('User: %s Pass: %s Processing files %s' % (str(username),str(pw),str(test)))


class TestHandler(webapp.RequestHandler):
    def get(self):
        #user = AAA(self,GenerateHandler)
        #self.response.headers['Content-Type'] = "text/html"
        self.redirect("/static/test.html")
        self.response.out.write('<h1>Test!</h1>')
        pass
    def post(self):
        #user = AAA(self,GenerateHandler)
        mdlfile = self.request.get("model")
        self.response.out.write('Processing file')
        pass

class AdminHandler(webapp.RequestHandler):
    def get(self):
        pass
    def post(self):
        pass
        
class DownloadHandler(webapp.RequestHandler):
    def get(self):
   
        # accept a unique link to download the ide
        # the key is defined as md5calc(email + password)
        
        email = self.request.get('u').lower()
        key = self.request.get('k')
          
        if not (email and key):
            result = (INVALID_FIELDS, 'Error', 'Error: Missing Information.  Please make sure the entire URL was used.')
            self.response.out.write(basicTemplate(*result))
            return

        # lookup the passed email
        find_user = UserAccount.all().filter('email = ', email).fetch(1)
        
        if len(find_user) == 0:
            result = (UNKNOWN_ERROR, 'Error', 'Error: An error occured. Please recreate your account.')
            self.response.out.write(basicTemplate(*result))
            return

        # is code valid
        if md5calc(email + find_user[0].password) != key:
            result = (UNKNOWN_ERROR, 'Error', 'Error: An error occured.  Please make sure the entire URL was used.')
            self.response.out.write(basicTemplate(*result))
            return

        if not find_user[0].confirmed:
            result = (INVALID_LOGIN, 'Error', 'Error: An error occured.  Account has not yet been confirmed.')
            self.response.out.write(basicTemplate(*result))
            return
                
            
        find_user[0].timesDownloadedIDE += 1
        find_user[0].dateLastDownloadedIDE = datetime.now()
        find_user[0].put()

        time.sleep(0.5)
        download_ide_location = 'http://www.openclovis.org/files/images/openclovis_logo_sm.gif'
        self.redirect(download_ide_location)
            

            
    def post(self):    
        pass
        
def main():
    application = webapp.WSGIApplication([('/', MainHandler),
                                          ('/generate',GenerateHandler),
                                          ('/test',TestHandler),
                                          ('/admin',AdminHandler),
                                          ('/download',DownloadHandler),
                                          ('/getfile/(.*)',GetFileHandler)
                                          ]
                                         + UserMgtPages + AdminMgtPages,
                                         debug=True)
    util.run_wsgi_app(application)


if __name__ == '__main__':
    main()
