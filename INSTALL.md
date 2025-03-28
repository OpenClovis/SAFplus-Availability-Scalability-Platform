
# How to install OpenClovis SAFplus Software Development Kit
This software can work and be installed on most Linux distros. The following guidlines are applied for Ubuntu, Debian, Fedore or CentOS.
Other distros, please contact us via email: support@openclovis.org

## Prerequisites
This software needs some dependencies to be installed first. You can install them manually below
### For Ubuntu (22.04 or newer)
Installing by apt: build-essential, gettext, libglib2.0-dev, libperl-dev
### For Debian (11 or newer)
Installing by apt: build-essential, gettext, libglib2.0-dev, libperl-dev, tclsh, gawk, ed
In case of debian server version: to run the SAFplus-IDE, you need to install more dependencies like cups, libgtk-3-dev and libgtk2.0-0
### For Fedora and CentOS:
Installing by yum install: TBD

## Installation
Run ./install and follow the instructions

When the installation completes, please download the Java Runtime Environment (JRE) version 8 (jre-8u181-linux-x64.tar.gz). Then move it to 
the buildtool directory of the installed SDK (gotten from the Installation step above). For example, if the installed SDK is /home/<user_name>/opt/clovis/sdk-6.0, then it's /home/<user_name>/opt/clovis/buildtool/local and extract it here.

## Support

   If you have any question or need any support about this project, don't hesitate to send us an email to support@openclovis.org.
   We'll respond immediately.
