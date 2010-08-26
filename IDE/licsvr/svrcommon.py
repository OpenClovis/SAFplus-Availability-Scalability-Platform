# Python imports
import os
import os.path
import logging
from hashlib import md5
from datetime import datetime, date

# WebApp imports
from google.appengine.ext.webapp import template
from google.appengine.api import mail


# Global Customization
DefaultTitle     = "OpenClovis SAF IDE"
TemplateDir      = "templates" + os.sep
SiteMaintainer   = "licsvr@openclovis.com"


# General utility functions

# Generate the basic web page filled in with data.  In particular, this page provides a <div> for computer 2 computer web access
# with succinct success/failure information
def basicTemplate(err,automatedResponse,webBody,title=DefaultTitle):
    path = os.path.join(os.path.dirname(__file__), TemplateDir + 'frame.html')
    return template.render(path,{'hash':0,'errorcode':err,'automatedResponse':automatedResponse, 'content':webBody,'title':title})

# Creates a basic HTML link
def make_link(text, destination):
    return '<a href="%s" >%s</a>' % (destination, text)

# Calculate MD5 digest (with salt) for a passed string
def md5calc(instr):
    m = md5()
    m.update(instr)
    m.update('this is my salt, it helps protect against rainbow table attacks')
    return m.hexdigest()

# Send an email message
def send_email(recv, subj, frm, body):
    message = mail.EmailMessage(sender=frm, 
                                subject=subj)
    message.to = recv
    message.body = body

    logging.info("Email: To: %s Contents: %s" % (recv,body))
    message.send()
