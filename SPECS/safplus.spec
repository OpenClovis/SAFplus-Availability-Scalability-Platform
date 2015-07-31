Name:		safplus	        
Version:	        
Release:        
Group:          Development/Libraries
License:	commercial license or GPL-3.0+
Source0:	%{name}_%{version}.tar.gz
URL:		http://www.openclovis.com   
Vendor:		OpenClovis Inc
Packager:	OpenClovisInc <http://www.openclovis.com/blog>
Requires:       libxml2-devel gcc-c++ boost-devel libdb-devel gdbm-devel sqlite-devel 
Summary:     	SAFplus is SA-Forum API compatible middleware   
Prefix:         /opt/%{name}/%{version}/sdk 
%description
SAFplus is SA-Forum API compatible middleware that provides High 
Availability (HA), System Management, and other services, addressing 
the infrastructure needs of complex devices found in the telecom, 
defense, financial and server cluster appliance (MMORPG gaming, etc) markets
This package provides SAFplus source code, header files and libraries for developing 
the applications 
%prep
%setup -cn %{name} 
%build
echo "Building RPM PACKAGE"
cd src
make

%install

echo "Installing the SAFPLUS SOFTWARE"
echo "prefix = %prefix"
mkdir -p $RPM_BUILD_ROOT/%prefix
shopt -s dotglob
for file in $RPM_BUILD_DIR/%{name}/*
do
  cp -r $file $RPM_BUILD_ROOT/%prefix
done
shopt -u dotglob
%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root,-) 
/%prefix/*
/%prefix/.git/*

%changelog
* Fri Jul 10 2015 OpenClovis Inc <http://www.openclovis.com/blog> - 7.0-1.beta
- Initial package version for openclovis SAFplus software. 
