Name:		        
Version:        
Release:        1
Summary:     	SAFplus is SA-Forum API compatible middleware   
Group:          Application/Programming
License:	commercial license or GPL-3.0+
URL:		http://www.openclovis.com   
Vendor:		OpenClovis Inc
Packager:	OpenClovisInc <http://www.openclovis.com/blog>
Requires:       boost >= 1.50 gdbm protobuf 
Prefix: 
%description
SAFplus is SA-Forum API compatible middleware that provides High 
Availability (HA), System Management, and other services, addressing 
the infrastructure needs of complex devices found in the telecom, 
defense, financial and server cluster appliance (MMORPG gaming, etc) markets

%build
echo "Building binary RPM PACKAGE"

%install
echo "Installing the SAFPLUS SOFTWARE"
echo "prefix = %prefix"
for file in $RPM_BUILD_DIR/*
do
  echo $file
  if [ -f $file ]
  then
       if [ $(basename "$file") != "Makefile" ]
       then
           mkdir -p $RPM_BUILD_ROOT/%prefix 
           cp $file $RPM_BUILD_ROOT/%prefix
       fi
  else    
      make ROOT="$RPM_BUILD_ROOT" INSTALLDIR="%prefix" PKG_DIRS="$file" install
  fi
done
%clean
rm -rf $RPM_BUILD_ROOT


%files
/%prefix/*

%changelog
