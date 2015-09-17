#Below two macros are added to skip *.pyc and *.pyo files in the RPM package
%define __python 0
%define _python_bytecompile_errors_terminate_build 0
Name:
Group:          Development/Libraries
License:	commercial license or GPL-3.0+
URL:		http://www.openclovis.com   
Vendor:		OpenClovis Inc
Packager:	OpenClovisInc <http://www.openclovis.com/blog>
Requires:       libxml2-devel gcc-c++ boost-devel libdb-devel gdbm-devel sqlite-devel 
Summary:     	SAFplus is SA-Forum API compatible middleware   
Prefix:
%description
SAFplus is SA-Forum API compatible middleware that provides High 
Availability (HA), System Management, and other services, addressing 
the infrastructure needs of complex devices found in the telecom, 
defense, financial and server cluster appliance (MMORPG gaming, etc) markets
This package provides SAFplus source code, header files and libraries for developing 
the applications 
%build
echo "Building RPM PACKAGE"

%install
%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root,-) 

%changelog
* Fri Jul 10 2015 OpenClovis Inc <http://www.openclovis.com/blog> - %{version}-{release}.beta
- Initial package version for openclovis SAFplus software. 
