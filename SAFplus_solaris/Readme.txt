For installing OpenClovis in Solaris system. 

This WIP(Work In Progress) version contain:
1. Installation procedure for 3rd Party support packages.
2. (WIP)Installation procedure for SAF-OpenClovis 6.1.

Kindly follow the below steps:

1.	Create a directory (ex: openclovis) and through git clone the following from openclovis Github (https://github.com/OpenClovis).
	a.	SAFplus_IDE.
	b.	SAFplus-Availability-Scalability-Platform.
	c.	3rdparty
2.	Rename the following repositories.
	a.	SAFplus_IDE to IDE.
	b.	SAFplus-Availability-Scalability-Platform to SAFplus
3.	 Navigate to the 3rdparty directory.
	a.	Untar the eclipse-SDK-x.x.x-linux-gtk.tar.gz (for 32-bit) OR
	b.	Untar the eclipse-SDK-x.x.x-linux-gtk-x86_64.tar.gz (for 64-bit)
	c.	Move the eclipse directory to idedev and navigate to idedev  directory. 
	d.	Then unzip the fallowing zip files in the current directory (3rdparty directory)
	e.	emf-runtime-x.x.x.zip
	f.	GEF-runtime-x.x.x.zip
4.	 Move the eclipse directory in 3rdparty directory as well as the openclovis directory.
5.	 Copy the following jar files inside location openclovis/eclipse/plugins/ :
	a.	org.eclipse.cdt.launch_7.1.0.201302132326.jar
	b.	org.eclipse.cdt.core_5.4.1.201302132326.jar
	c.	org.eclipse.cdt.managedbuilder.core_8.1.0.201302132326.jar
	d.	org.eclipse.cdt.debug.core_7.2.0.201302132326.jar
6.	JDK-7 needs to be installed using 
          # pkg install jdk-7
7.	Navigate to the openclovis/IDE directory.
8.	Run gmake –f release.mk
9.	An openclovis 6.1 private directory is created along with its .tar and .md5
10.	Navigate to the  path/to/the/<openclovis 6.1 private directory> (will be created while installation).
11.	Get packages.py and install.py from location: (https://github.com/OpenClovis/SAFplus-Availability-Scalability-Platform/)
12.	Relocate packages.py file to path/to/the/<openclovis 6.1 private directory>/src/install/
13.	Relocate install.py to path/to/the/<openclovis 6.1 private directory>/
14.	Follow the README in the GITHUB location to create the 3rd party tarball.
15.	Place the tarball in the openclovis directory.
16.	Navigate to path/to/the/<openclovis 6.1 private directory> & execute install.py.

Issues:
While installing OpenClovis6.1, prebuild section during PostInstallation() is having error as:

# /clovis/6.1/sdk/src/SAFplus/configure --with-safplus-build   > build.log
find: stat() error /clovis/6.1/sdk/prebuild/asp/build/local/servers/Makefile: No such file or directory
find: cycle detected for /lib/32/
find: cycle detected for /lib/secure/32/
find: cycle detected for /lib/crypto/32/
find: cycle detected for /usr/lib/brand/solaris10/32/
find: cycle detected for /usr/lib/lwp/32/
find: cycle detected for /usr/lib/link_audit/32/
find: cycle detected for /usr/lib/security/32/
find: cycle detected for /usr/lib/vmware-tools/lib/i86/
find: cycle detected for /usr/lib/secure/32/
find: cycle detected for /usr/lib/elfedit/32/
find: cycle detected for /usr/lib/fm/topo/plugins/32/
find: cycle detected for /usr/lib/locale/en_US.UTF-8/LO_LTYPE/32/
find: cycle detected for /usr/lib/locale/en_US.UTF-8/32/
find: cycle detected for /usr/lib/locale/en_US.UTF-8/LC_CTYPE/32/
find: cycle detected for /usr/lib/32/
configure: error: Sorry, can't do anything for you
Building SAFplus local
make: Fatal error: Don't know how to make target `safplus-libs'

NOTE: We are working on this and will update the complete version shortly.
