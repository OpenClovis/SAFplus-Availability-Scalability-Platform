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
from usermgt import *


# returns a google user account logout link string
def logout_link(who, user=""):
    return make_link("Logout " + user, users.create_logout_url(who.request.uri))


# returns true or false based on succes or failure of 
# a user being validated as an admin based on their status
# of their google account (NOT LICSVR ACCOUNT)
# returns an error message passable to self.response.out.write(basicTemplate(*result))
#   for invalid accounts
# returns the users email addr
#   for valid accounts
def validate_admin(who):

    # get current google account user
    user = users.get_current_user()
    
    if not user:
        msg  = '<h2>Not logged in as a Google User.</h2>'
        msg += '<ul><li>Please use a <b>non</b> openclovis.com account</li>'
        msg += '<li>You must use a <b>*@gmail.com</b> email</li></ul>'
        msg += '<h3>Click %s to login</h3>' % make_link('here', users.create_login_url(who.request.uri))
        return False, (INVALID_LOGIN, 'Error', msg)
  
    if not (user.email() in DASHBOARD_ADMINS):
        msg  = 'Your Google user account <b>%s</b> has not been enabled as an admin.<br /><br />' % user.email()
        msg += 'Please contact a site maintainer:<br /><br />'
        
        for maintainer in SITE_MAINTAINERS:
            msg += make_link(maintainer, 'mailto:%s?subject=Licsvr Dashboard Admin Request?body=Please enable: %s' % (maintainer, user.email()))
            msg += '<br />'
      
        msg += '<br /><br />Click %s to logout and try a different account<br />' % make_link('here', users.create_logout_url(who.request.uri))
    
        return False, (INVALID_LOGIN, 'Error', msg)
    
    return True, user.email()



class AdminEdit(webapp.RequestHandler):
    def get(self):
        
        is_admin, result = validate_admin(self)
        
        if not is_admin:
            self.response.out.write(basicTemplate(*result))
            return
        
        # 'Edit' link from Dashboard clicked for a certain user
        email = self.request.get('u').lower()
        key = self.request.get('k')
        
        errormsg = ''
        
        if not (email and key):
            errormsg += 'Error: An error occured'
            return
        
        # lookup the passed email
        find_user = UserAccount.all().filter('email = ', email).fetch(1)
        
        if len(find_user) == 0:
            self.response.out.write('Error: An error occured')
            return
        
        if md5calc(find_user[0].password+str(find_user[0].lastUpdated)) != key:
            self.response.out.write('Error: An error occured')
            return
        
        confirm_checked = ''
        
        if find_user[0].confirmed:
            confirm_checked = 'CHECKED'
        
        template_values = {
            'email_lbl': 'Email:', 
            'pass_lbl': 'Password:', 
            'phone_lbl': 'Phone #:', 
            'confirm_lbl': 'Confirmed:', 
            'lastup_lbl': 'Last Modified:', 
            'datereg_lbl': 'Date Registered:', 
            'genremain_lbl': 'Generates Remaining:', 
            
            'genused_lbl': 'Generates Used:',
            'dateoflastcodegen_lbl': 'Date of last Generate:',
            'timesdownloadedide_lbl': 'Times Downloaded IDE:',
            'datelastdownloadedide_lbl': 'Date Downloaded IDE:',
            'timesloggedin_lbl': 'Times Logged In:',
            'lastloggedin_lbl': 'Last Logged In:',
            'ip_lbl': 'IP Address: ' + make_link('Whois', 'http://www.ip-adress.com/ip_tracer/%s' % find_user[0].regIPAddress),

            
            
            'email_val': email, 
            'password_val': find_user[0].password, 
            'phone_val': find_user[0].phone, 
            'confirm_val': confirm_checked, 
            'lastup_val': find_user[0].lastUpdated, 
            'datereg_val': find_user[0].dateReg, 
            'genremain_val': find_user[0].remainingGenerations,
            
            'genused_val': find_user[0].codeGensUsed,
            'dateoflastcodegen_val': find_user[0].dateOfLastCodeGen,
            'timesdownloadedide_val': find_user[0].timesDownloadedIDE,
            'datelastdownloadedide_val': find_user[0].dateLastDownloadedIDE,
            'timesloggedin_val': find_user[0].timesLoggedIn,
            'lastloggedin_val': find_user[0].lastLoggedIn,
            'ip_val': find_user[0].regIPAddress,
            
            'u': email, 
            'k': key,
            
            'logout_link': logout_link(self, result)
        }
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'adminedit.html')
        page = basicTemplate(OK, 'Admin - Edit', template.render(path, template_values))
        self.response.out.write(page)
    
    def post(self):
        
        is_admin, result = validate_admin(self)
        
        if not is_admin:
            self.response.out.write(basicTemplate(*result))
            return
        
        
        # admin submitted data to change for a user
        
        # this is presumably an admin page so errors can be a little more verbose
        
        emailchk = self.request.get('u').lower()
        key = self.request.get('k')
        
        newemail = self.request.get('email').lower().strip()
        phone = self.request.get('phone')
        genremain = self.request.get('genremain')
        validated = False
        
        if self.request.get('validated'):
            validated = True
        
        if not phone:
            phone = 'None'
        
        
        if not (emailchk and newemail and key and genremain):
            self.response.out.write('<h3>Error: Missing Input</h3>')
            
            if mldebug:
                self.response.out.write('emailchk: %s<br />' % str(emailchk))
                self.response.out.write('newemail: %s<br />' % str(newemail))
                self.response.out.write('key: %s<br />' % str(key))
                self.response.out.write('phone: %s<br />' % str(phone))
                self.response.out.write('validated: %s<br />' % str(validated))
                self.response.out.write('genremain: %s<br />' % str(genremain))
                
                return
            
            return
        
        # lookup the passed email
        find_user = UserAccount.all().filter('email = ', emailchk).fetch(1)
        
        if len(find_user) == 0:
            self.response.out.write('Error: User not found')
            return
        
        if md5calc(find_user[0].password+str(find_user[0].lastUpdated)) != key:
            self.response.out.write('Error: Bad Key')
            return
        
        find_user[0].email = newemail
        find_user[0].phone = phone
        find_user[0].remainingGenerations = int(genremain)
        find_user[0].confirmed = bool(validated)
        
        # did we ask validation status to change?
        #if validated != find_user[0].confirmed:
        #        if validated:
        #            # they are becoming validated
        #            find_user[0].confirmed = True
        #            
        #        else:
        #            # they are losing validation
        #            find_user[0].confirmed = False
        #            find_user[0].dateReg = ''
        
        
        find_user[0].put()
        
        
        # find him again to get new lastUpdated
        #find_user = UserAccount.all().filter('email = ', email).fetch(1)
        
        # redirect!
        #self.redirect('adminedit?&u=' + newemail + '&k=' + md5calc(find_user[0].password+str(find_user[0].lastUpdated)))
        
        self.redirect('dashboard')

class DetailView(webapp.RequestHandler):
    def get(self):
        
        is_admin, result = validate_admin(self)
        
        if not is_admin:
            self.response.out.write(basicTemplate(*result))
            return
        
        # Username link from Dashboard clicked for a certain user
        email = self.request.get('u').lower()
    
        
        errormsg = ''
        
        if not email:
            errormsg += 'Error: An error occured. Invalid email'
            return
        
        # lookup the passed email
        find_user = UserAccount.all().filter('email = ', email).fetch(1)
        
        if not find_user:
            self.response.out.write('Error: An error occured')
            return
        
                
        confirm_checked = 'No'
        
        if find_user[0].confirmed:
            confirm_checked = 'Yes'
        
        template_values = {
        
            'edit_user_link': make_link('Edit This User', 'adminedit?&u=' + find_user[0].email + '&k=' + md5calc(find_user[0].password+str(find_user[0].lastUpdated))),
        
            'email_lbl': 'Email:', 
            'pass_lbl': 'Password:', 
            'phone_lbl': 'Phone #:', 
            'confirm_lbl': 'Confirmed:', 
           
            'datereg_lbl': 'Date Registered:', 
            'genremain_lbl': 'Generates Remaining:', 
            
            'genused_lbl': 'Generates Used:',
            'dateoflastcodegen_lbl': 'Date of last Generate:',
            'timesdownloadedide_lbl': 'Times Downloaded IDE:',
            'datelastdownloadedide_lbl': 'Date Downloaded IDE:',
            'timesloggedin_lbl': 'Times Logged In:',
            'lastloggedin_lbl': 'Last Logged In:',
            'ip_lbl': 'IP Address: ' + make_link('Whois', 'http://www.ip-adress.com/ip_tracer/%s' % find_user[0].regIPAddress),

            'email_val': email, 
            
            'phone_val': find_user[0].phone, 
            'confirm_val': confirm_checked, 
          
            'datereg_val': find_user[0].dateReg, 
            'genremain_val': find_user[0].remainingGenerations,
            
            'genused_val': find_user[0].codeGensUsed,
            'dateoflastcodegen_val': find_user[0].dateOfLastCodeGen,
            'timesdownloadedide_val': find_user[0].timesDownloadedIDE,
            'datelastdownloadedide_val': find_user[0].dateLastDownloadedIDE,
            'timesloggedin_val': find_user[0].timesLoggedIn,
            'lastloggedin_val': find_user[0].lastLoggedIn,
            'ip_val': find_user[0].regIPAddress,
            

            'logout_link': logout_link(self, result)
        }
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'detailview.html')
        page = basicTemplate(OK, 'Admin - User Detail', template.render(path, template_values))
        self.response.out.write(page)
    

class MostActive(webapp.RequestHandler):
    def get(self): 
        
        # this page should only be available if they are logged in via google account
        # service and they are in the list of valid users
        
        is_admin, result = validate_admin(self)
        
        if not is_admin:
            self.response.out.write(basicTemplate(*result))
            return
        
        # only show users that have been using codegens
        # sort decending by # of code gens used
        all_users = UserAccount.all().filter('codeGensUsed >', 0).order('-codeGensUsed').fetch(10000)
        
        html = ''
        
        for user in all_users:
            html += '<tr>'
            if user.confirmed:
                html += '<td><strong>%s</strong></td>' % user.email
            else:
                html += '<td>%s</td>' % user.email
             
            # num code gens used
            html += '<td>%s code gens used</td>' % user.codeGensUsed
                      
            # whois
            html += '<td>%s</td>' % make_link('Whois', 'http://www.ip-adress.com/ip_tracer/%s' % user.regIPAddress)
            
            # edit
            html += '<td>%s</td>' % make_link('Edit', 'adminedit?&u=' + user.email + '&k=' + md5calc(user.password+str(user.lastUpdated)))
            html += '</tr>'
        
        
        # no active users :(
        if not all_users:
            html = '<tr><td><center>Currently there are no active users generating code</center></td></tr>'

        
        template_values = {
            'table_content': html,
            'logout_link': logout_link(self, result)
        }
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'mostactive.html')
        page = basicTemplate(OK, 'Admin - Most Active Users', template.render(path, template_values))
        self.response.out.write(page)


class Dashboard(webapp.RequestHandler):
    def get(self): 
        
        # this page should only be available if they are logged in via google account
        # service and they are in the list of valid users
        
        is_admin, result = validate_admin(self)
        
        if not is_admin:
            self.response.out.write(basicTemplate(*result))
            return
        
        # get all user accounts
        # form a html table
        # load it into template
        all_users = UserAccount.all().fetch(10000)
        
        html = ''
        
        for user in all_users:
            html += '<tr>'
            if user.confirmed:
                html += '<td><strong>%s</strong></td>' % make_link(user.email, 'detailview?&u=' + user.email)
            else:
                html += '<td>%s</td>' % make_link(user.email, 'detailview?&u=' + user.email)
                
            # whois
            html += '<td>%s</td>' % make_link('Whois', 'http://www.ip-adress.com/ip_tracer/%s' % user.regIPAddress)
            
            # edit
            html += '<td>%s</td>' % make_link('Edit', 'adminedit?&u=' + user.email + '&k=' + md5calc(user.password+str(user.lastUpdated)))
            html += '</tr>'
        
        # no users :(
        if not all_users:
            html = '<tr><td><center>Currently there are no users</center></td></tr>'

        
        template_values = {
            'table_content': html,
            'logout_link': logout_link(self, result)
        }
        
        path = os.path.join(os.path.dirname(__file__), TemplateDir + 'dashboard.html')
        page = basicTemplate(OK, 'Admin - Home', template.render(path, template_values))
        self.response.out.write(page)



# -----------------------

AdminMgtPages = [('/adminedit', AdminEdit),
                ('/detailview', DetailView),
                ('/mostactive', MostActive),
                ('/dashboard', Dashboard)]
