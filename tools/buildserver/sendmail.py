#!/usr/bin/env python

import smtplib
import os, sys, pdb
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import COMMASPACE, formatdate
from email import Encoders

def send_mail(send_from, send_to, subject, text, files=[], server="localhost"):
  assert type(send_to)==list
  assert type(files)==list

  msg = MIMEMultipart()
  msg['From'] = send_from
  msg['To'] = COMMASPACE.join(send_to)
  msg['Date'] = formatdate(localtime=True)
  msg['Subject'] = subject

  msg.attach( MIMEText(text) )

  for f in files:
    part = MIMEBase('application', "octet-stream")
    content = open(f, 'rb').read()
    part.set_payload(content)
    Encoders.encode_base64(part)
    part.add_header('Content-Disposition', 'attachment; filename="%s"' % os.path.basename(f))
    msg.attach(part)

  smtp = smtplib.SMTP(server)
  smtp.sendmail(send_from, send_to, msg.as_string())
  smtp.close()
 
if __name__ == '__main__':
  if len(sys.argv) < 5:
    print 'Usage: sendmail.py <from> <to> <subject> <body-file> [<attachment-file1> [<attachment-file-2]...]'
    sys.exit(1)

  _from = sys.argv[1]
  to = sys.argv[2].split(',')
  subject = sys.argv[3]
  text = file(sys.argv[4]).read()
  send_mail(_from, to, subject, text, sys.argv[5:])
