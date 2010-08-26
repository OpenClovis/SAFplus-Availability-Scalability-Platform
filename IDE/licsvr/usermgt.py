# Python imports
import pdb
import os
import time
import logging
from hashlib import md5
from datetime import datetime, date

# WebApp imports
from google.appengine.ext.webapp import template
from google.appengine.api import mail

from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app
from google.appengine.api import users
from google.appengine.ext import db

# Local imports
from constants import *
from svrcommon import *

# Matt's local debug flag
# I use this to skip email systems etc... should ALWAYS be false when deployed
mldebug = False

# Data Model
class UserAccount(db.Model):
    email                   = db.StringProperty()
    password                = db.StringProperty()
    phone                   = db.PhoneNumberProperty(required=False, default=None)
    confirmed               = db.BooleanProperty(default=False)
    lastUpdated             = db.DateTimeProperty(auto_now=True)
    dateReg                 = db.DateTimeProperty(auto_now_add=True)
    remainingGenerations    = db.IntegerProperty(default=10)
    codeGensUsed            = db.IntegerProperty(default=0)
    lastLoggedIn            = db.DateTimeProperty()
    timesLoggedIn           = db.IntegerProperty(default=0)
    dateOfLastCodeGen       = db.DateTimeProperty()
    timesDownloadedIDE      = db.IntegerProperty(default=0)
    dateLastDownloadedIDE   = db.DateTimeProperty()
    regIPAddress            = db.StringProperty()
    

#def AAA(ctxt, fn):
#    user = users.get_current_user()
#    if not user:
#        pass
#    ctxt.redirect(users.create_login_url(ctxt.request.uri))
#    if fn == GenerateHandler: return user
#    return user

# Accepts an email and password, and returns true if
# the passed email exists and password is valid for that account
# *** returns False if account not validated ***
def validate_login(email, password):
    
    # make sure email and password non empty
    if not (email and password):
        return False
    
    userobj = UserAccount.all().filter('email =', email).fetch(1)
    
    # if no matching email found
    if len(userobj) == 0:
        return False
    
    # is user validated? (have they clicked the confirmation link)
    if not userobj[0].confirmed:
        return False
    
    # does password match stored digest?
    if md5calc(password) == userobj[0].password:
        return userobj[0]
    
    return False


# --------------------------------------------
# Pages

class UserStats(webapp.RequestHandler):
    def get(self):
        
        reg_count = UserAccount.all().filter('confirmed = ', True).count()
        unreg_count = UserAccount.all().count() - reg_count
        
        template_values = {
            'greeting': 'Openclovis IDE Registration', 
            'regCount': reg_count, 
            'unregCount': unreg_count, 
            'url1': '/login', 
            'url1_linktext': 'Login', 
            'url2': '/create', 
            'url2_linktext': 'Register', 
            'url3': '/forgot', 
            'url3_linktext': 'Forgot Password'}
        
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'home.html')
        self.response.out.write(template.render(path, template_values))


class Login(webapp.RequestHandler):
    def get(self):
    
        if mldebug:
            self.response.out.write('[mldebug = True] usermgt.py Make sure this is FALSE in deployed systems!')

    
        result = (OK, 'Login', 'Please use the panel on the right to login.')
        self.response.out.write(basicTemplate(*result))
    
    def post(self):
        
        email = self.request.get('email').lower().strip()
        password = self.request.get('password').strip()
        
        if validate_login(email, password):
                    
            download_link = '/download?&u=%s&k=%s' % (email, md5calc(email + md5calc(password)))
            
            result = (OK, 'login valid', 'You are logged in.  Please %s the OpenClovis Middleware IDE.' % make_link('Download', download_link))
            
            # set lastLoggedIn
            # inc timesLoggedIn
            user = UserAccount.all().filter('email = ', email).fetch(1)
            
            if len(user) != 0:
                user[0].timesLoggedIn += 1
                user[0].lastLoggedIn = datetime.now()
                user[0].put()
                            
        else:
            result = (INVALID_LOGIN, 'login invalid', 'Login information incorrect or unverified.')
            
        self.response.out.write(basicTemplate(*result))


class Forgot(webapp.RequestHandler):
    def get(self):
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'forgot.html')
        page = basicTemplate(OK, 'Forgot', template.render(path, None))
        self.response.out.write(page)
    
    
    def post(self):
        # user submitted the form
        # check to see if we have a matching email
        # if we do, send to that email a link which will give
        # the user the option to reset their password
        
        recv = self.request.get('email').lower()
        
        errorsmsg = ''
        
        if not recv:
            result = (INVALID_FIELDS, 'Error', 'Error: No email supplied. Please go %s and try again.' % make_link('back', '/forgot'))
            self.response.out.write(basicTemplate(*result))
            return
        
        find_email = UserAccount.all().filter('email = ', recv).fetch(1)
        
        if len(find_email) == 0:
            # no email found
            result = (INVALID_LOGIN, 'Error', 'Error: No account found with email: <b>%s</b>.<br />Please go %s and try again.' % (recv, make_link('back', '/forgot')))
            self.response.out.write(basicTemplate(*result))
            return
        
        lastupdated = str(find_email[0].lastUpdated)
        
        # email has been validated after this point
        
        base_url = self.request.headers.get('host', 'no host') + '/reset'
        reset_link = 'http://' + base_url + '?&u=%s&k=%s' % (recv, md5calc(recv+lastupdated)) # u=email k=key
        subj = 'Clovis SDK account password reset request'
        body = 'Please use this link to reset your password:\n%s ' % reset_link
        frm = SiteMaintainer
        #logging.info(body)
        
        try:
            send_email(recv, subj, frm, body)
            result = (OK, 'Success', '<h3>Password reset message sent successfully.</h3>Check your email to reset your password')
            self.response.out.write(basicTemplate(*result))
        
        except Exception, e:
            result = (UNKNOWN_ERROR, 'Error', 'Email could not be sent: Error: %s' % str(e))
            self.response.out.write(basicTemplate(*result))


class Create(webapp.RequestHandler):
    def get(self):
        
        template_values = {
			'greeting': 'Create New Account'
            }
        
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'create.html')
        page = basicTemplate(OK, 'CreateAccount', template.render(path, template_values))
        self.response.out.write(page)
    
    def post(self):
        # they submitted an account to be created
        
        errors = 0
        email = self.request.get('email').lower()
        phone_num = self.request.get('phoneNum')
        if len(phone_num) <= 5:
            phone_num = '000-000-0000'
        # make sure input not empty
        
        errorsmsg = ''
        
        if not (email and self.request.get('password')):
            errorsmsg += 'Error: All or part of input is empty<br />'
            errors += 1
        
		# make sure passwords matched
        if (self.request.get('password') != self.request.get('password2')):
            errorsmsg += 'Error: Passwords do not match<br />'
            errors += 1
        
        # make sure password is valid length
        if (len(self.request.get('password')) <= 5):
            errorsmsg += 'Error: Password must be 6 or more characters<br />'
            errors += 1
        
		# make sure email is valid (simple checks only)
        if email.count('@') != 1 or email.count(' ') > 0 or email.count('.') == 0:
            errorsmsg += 'Error: Email %s is invalid<br />' % email
            errors += 1
        
		# make sure user account doesn't already exist
        find_user = UserAccount.all().filter('email = ', email).fetch(1)
        if len(find_user) != 0:
            errorsmsg += 'Error: Email %s is already in use. \
			Please choose a different email or click %s if you have \
			forgotten your password<br />' % ( email, make_link('here', 'forgot') ) 
            errors += 1
        
        if errors > 0:
            # errors occured, stop creating account
            result = (OK, 'Error', errorsmsg + '<h3>Please go %s and try again</h3>' % make_link('back', '/create'))
            self.response.out.write(basicTemplate(*result))
            return
        
        hashed_password = md5calc(self.request.get('password'))
        
        # create account in database
        new_accnt = UserAccount()
        new_accnt.email = email
        new_accnt.phone = phone_num
        new_accnt.password = hashed_password
        new_accnt.regIPAddress = self.request.remote_addr
        
        # send email to validate account
        # validation link key relates to email + MD5(hashed(password))
        
        base_url = self.request.headers.get('host', 'no host') + '/confirm'
        validation_link = 'http://' + base_url + '?&u=%s&k=%s' % (email, md5calc(hashed_password)) # u=email k=key
        
        if mldebug:
            new_accnt.put()
            
            msg = 'User account created <br />'
            msg += 'mldebug set, skipping email, <br />'
            msg += make_link('Click here to validate', validation_link) + '<br />'
            
            result = (OK, 'Created', msg)
            self.response.out.write(basicTemplate(*result))
            return
        
        frm = SiteMaintainer
        subj = 'OpenClovis IDE Account Activation Link'
        body = 'Use this link to validate your account:\n%s' % validation_link
        
        try:
            send_email(email, subj, frm, body)
            new_accnt.put()
            result = (OK, 'Created', 'User account created, please check <b>%s</b> to validate account before login.' % email)
            self.response.out.write(basicTemplate(*result))
        except Exception, e:
            result = (UNKNOWN_ERROR, 'Error', 'Email could not be sent: Error: %s' % str(e))
            self.response.out.write(basicTemplate(*result))

class Done(Exception):
    pass

class Confirm(webapp.RequestHandler):
    def get(self):       
        try:
            #self.response.out.write(template.render(path, template_values))
            
            email = self.request.get('u').lower()
            key = self.request.get('k')
            
            if not (email and key):
                raise Done('Error: An error occured.  Please make sure the entire URL was used.')
            
            # lookup the passed email
            find_user = UserAccount.all().filter('email = ', email).fetch(1)
            
            if len(find_user) == 0:
                raise Done('Error: An error occured. Please recreate your account.')
            
            # is code valid
            if md5calc(find_user[0].password) == key:
                
                # if not previously confirmed
                if not find_user[0].confirmed:
                    find_user[0].confirmed = True
                    find_user[0].put()
                
                download_link = '/download?&u=%s&k=%s' % (email, md5calc(email + find_user[0].password))
                result = '<h3>Account confirmed</h3>You are logged in. Please %s the OpenClovis Middleware IDE.' % make_link('Download', download_link)
           
                raise Done(result)
                
            else:
                raise Done('Error: An error occured.  Please make sure the entire confirmation URL was used.')
        
        except Done, e:
            page = basicTemplate(OK, 'Confirm', str(e))
            self.response.out.write(page)

class Reset(webapp.RequestHandler):
    def get(self):
        try:
        
            email = self.request.get('u').lower()
            key = self.request.get('k')
            
            if not (email and key):
                raise Done('Error: An error occured')

            # lookup the passed email
            find_user = UserAccount.all().filter('email = ', email).fetch(1)
            
            if len(find_user) == 0:
                raise Done('Error: An error occured')
                return
            
            if md5calc(find_user[0].email+str(find_user[0].lastUpdated)) != key:
                raise Done('Error: An error occured')
                return
            
            template_values = {
                'account': email, 
                'u': email, 
                'k': key
            }
            
            path = os.path.join(os.path.dirname(__file__), TemplateDir + 'doreset.html')
            page = basicTemplate(OK, 'Reset Password', template.render(path, template_values))
            self.response.out.write(page)
            
        except Done, e:
            page = basicTemplate(INVALID_FIELDS, 'Reset', str(e))
            self.response.out.write(page)

class DoReset(webapp.RequestHandler):
    # they submitted a form to reset password
    def post(self):
        email = self.request.get('u').lower()
        key = self.request.get('k')
        
        errors = 0
        
        errorsmsg = ''
        
        # make sure passwords matched
        if (self.request.get('password') != self.request.get('password2')):
            errorsmsg += 'Error: Passwords do not match<br />'
            errors += 1
        
        password = self.request.get('password')
        
        # make sure password is valid length
        if len(password) <= 5:
            errorsmsg += 'Error: Password must be 6 or more characters'
            errors += 1
        
        if not (email and key and password):
            errorsmsg += 'Error: All or part of input is empty'
            errors += 1
        
        
        # lookup the passed email
        find_user = UserAccount.all().filter('email = ', email).fetch(1)
        
        if md5calc(find_user[0].email+str(find_user[0].lastUpdated)) != key:
            errorsmsg += 'Error: An error occured'
            errors += 1
        
        # errors occured, stop updating account
        if errors > 0:
            result = (INVALID_FIELDS, 'Error', errorsmsg)
            self.response.out.write(basicTemplate(*result))
            return
        
        hashed_password = md5calc(self.request.get('password'))
        
        # create account in database
        find_user[0].password = hashed_password
        find_user[0].put()

        result = (OK, 'Success', 'Password successfuly updated. Login using the panel on the right')
        self.response.out.write(basicTemplate(*result))


# -----------------------

UserMgtPages = [('/userstats', UserStats), 
                ('/login', Login), 
                ('/forgot', Forgot), 
                ('/create', Create), 
                ('/reset', Reset), 
                ('/doreset', DoReset), 
                ('/confirm', Confirm)]
