#Below two macros are added to skip *.pyc and *.pyo files in the RPM package
%define __python 0
%define _python_bytecompile_errors_terminate_build 0
my_build_arch:
%define saf_prefix %{prefix}/sdk/target/%{tmp_target_platform}
%define saf_src_prefix %{prefix}/sdk
Name:		safplus
Version:        
Group:          Development/Libraries
License:	commercial license or GPL-3.0+
URL:		http://www.openclovis.com   
Vendor:		OpenClovis Inc
Packager:	OpenClovisInc <info@openclovis.com>
Obsoletes:	%{name} <= %{version}
BuildRequires:  libxml2-devel, boost-devel, libdb-devel, gdbm-devel, sqlite-devel, protobuf-devel >= 2.5, protobuf-python >= 2.5 bzip2-devel
Requires:       libxml2-devel, gcc-c++, boost-devel, libdb-devel, gdbm-devel, sqlite-devel, protobuf-devel >= 2.5, protobuf-python >= 2.5 python-devel
Requires:	python-pip wx-i18n >= 3.0 wxPython >= 3.0.2 bzip2-devel lksctp-tools lksctp-tools-devel librsvg2-devel gnome-python2-rsvg
Summary:     	SAFplus is SA-Forum API compatible middleware   
Prefix:		
%description
SAFplus is SA-Forum API compatible middleware that provides High 
Availability (HA), System Management, and other services, addressing 
the infrastructure needs of complex devices found in the telecom, 
defense, financial and server cluster appliance (MMORPG gaming, etc) markets
This package provides SAFplus source code, header files and libraries for developing 
the applications 
%package src
Summary: source code for %{name}
Group: Development/Libraries
Requires: wxPython >= 3.0.2 boost-devel libdb-devl gdbm-devel sqlite-devl protobuf-devel >= 2.5 protobuf-python >= 2.5 python-devl python-pip 
Requires: libxml2-devel wx-i18n >= 3.0 bzip2-devel rpm-build gcc-c++ lksctp-tools lksctp-tools-devel

%description src
This package contains safplus %{version} high availability middleware source code.

%build
echo "Building RPM PACKAGE"

%install
rm -rf %{buildroot}
echo %{saf_prefix}
echo %{saf_src_prefix}
mkdir -p %{buildroot}%{saf_prefix}
cp -rf %{_builddir}/target/%{tmp_target_platform}/* %{buildroot}%{saf_prefix}
cd %{_builddir}/src/ide && make clean
for files in %{_builddir}/*
do
 file_name=`basename "$files"`
 if [ "$file_name" != "target" ] && [ "$file_name" != "bin" ] && [ "$file_name" != "src" ]
 then
  cp -rf $files %{buildroot}%{saf_src_prefix}
 fi
done
for files in %{_builddir}/src/*
do
 file_name=`basename "$files"`
 if [ "$file_name" != "include" ]
 then
  cp -rf $files %{buildroot}%{saf_src_prefix}/src
 fi
done
rsync -rL %{_builddir}/src/include %{buildroot}%{saf_src_prefix}/src
mkdir -p %{buildroot}%{prefix}/ide
cp -rf %{_builddir}/bin/*   %{buildroot}%{prefix}/ide  

%clean
rm -rf %{buildroot}

%pre 
check_and_install_pypi_packages() {
  python -c "$1" >/dev/null 2>&1
  pkg_status=$?
  if [ "$pkg_status" -ne 0 ]; then
    printf "Installing $2 using pip\n"
    output="$(pip install $2)"
    if [ $? -ne 0 ]; then
      printf "$1 installation failed with below message $output\n"
      exit -1
    fi
  fi
}
check_and_install_pypi_packages "import watchdog" "watchdog"
check_and_install_pypi_packages "import pyang"  "pyang"
check_and_install_pypi_packages "import genshi" "genshi"

%pre src
check_and_install_pypi_packages() {
  python -c "$1" >/dev/null 2>&1
  pkg_status=$?
  if [ "$pkg_status" -ne 0 ]; then
    printf "Installing $2 using pip\n"
    output="$(pip install $2)"
    if [ $? -ne 0 ]; then
      printf "$1 installation failed with below message $output\n"
      exit -1
    fi
  fi
}
check_and_install_pypi_packages "import watchdog" "watchdog"
check_and_install_pypi_packages "import pyang"  "pyang"
check_and_install_pypi_packages "import genshi" "genshi"

%post
SYM_LINK_DIR="/usr/local/bin"
IDE_NAME="safplus_ide"
IDE_DIR=%{prefix}/ide
if [ ! -f $SYM_LINK_DIR/$IDE_NAME ]; then
  /bin/ln -s $IDE_DIR/$IDE_NAME $SYM_LINK_DIR/$IDE_NAME
fi

%post src
SYM_LINK_DIR="/usr/local/bin"
IDE_NAME="safplus_ide"
IDE_DIR=%{prefix}/ide
if [ ! -f $SYM_LINK_DIR/$IDE_NAME ]; then
  /bin/ln -s $IDE_DIR/$IDE_NAME $SYM_LINK_DIR/$IDE_NAME
fi

%preun 
SYM_LINK_DIR="/usr/local/bin"
IDE_NAME="safplus_ide"
IDE_DIR=%{prefix}/ide
rpm -q %{name}-src >/dev/null 2>&1
pkg_status=$?
if [ $pkg_status -ne 0 ]
then
  /bin/unlink $SYM_LINK_DIR/$IDE_NAME
fi

%preun src
SYM_LINK_DIR="/usr/local/bin"
IDE_NAME="safplus_ide"
IDE_DIR=%{prefix}/ide
rpm -q %{name} >/dev/null 2>&1
pkg_status=$?
if [ $pkg_status -ne 0 ]
then
  /bin/unlink $SYM_LINK_DIR/$IDE_NAME
fi

%postun
IDE_DIR=%{prefix}/ide
safplus_top_prefix=%{saf_src_prefix}
check_and_remove_pkg_files(){
  rpm -q %{name}-src >/dev/null 2>&1
  pkg_status=$?
  rpm -q %{name}-mgt >/dev/null 2>&1
  pkg_mgt_status=$?
  if [ "$pkg_status" -ne 0 ]; then
    if [ -d "$(dirname $safplus_top_prefix)" ]
    then
      cd $(dirname $IDE_DIR) && rm -rf $(basename $IDE_DIR)
      if [ "$pkg_mgt_status" -ne 0 ]
      then
        cd $(dirname $safplus_top_prefix) && rm -rf $(basename $safplus_top_prefix)
      else
        for files in %{saf_prefix}/*
        do
          file_name=`basename "$files"`
	  if [ "$file_name" != "plugin" ]
            rm -rf $files
          fi
        done
      fi
    fi
  fi
}
if [ "$1" == "0" ]
then
  print "Preparing to remove the safplus...\n"
  check_and_remove_pkg_files
else
  print "Preparing to upgrade the safplus"
fi

%postun src
IDE_DIR=%{prefix}/ide
safplus_top_prefix=%{saf_src_prefix}
check_and_remove_pkg_files(){
  rpm -q %{name} >/dev/null 2>&1
  pkg_status=$?
  rpm -q %{name}-mgt >/dev/null 2>&1
  pkg_mgt_status=$?
  if [ "$pkg_status" -ne 0 ]; then
    if [ -d "$(dirname $safplus_top_prefix)" ]
    then
      cd $(dirname $IDE_DIR) && rm -rf $(basename $IDE_DIR)
      if [ "$pkg_mgt_status" -ne 0 ] 
      then
        cd $(dirname $safplus_top_prefix) && rm -rf $(basename $safplus_top_prefix)
      else
        for files in %{saf_prefix}/*
        do
          file_name=`basename "$files"`
          if [ "$file_name" != "plugin" ]
            rm -rf $files
          fi
        done
      fi
    fi
  else
    if [ -d "$(dirname $safplus_top_prefix)" ]
    then
      for file in $(safplus_top_prefix)/*
      do
        file_name=`basename "$file"`
        if [ "$file_name" != "src" ] && [ "$file_name" != "target" ]
        then
          rm -rf $file
        fi
      done
      for  file in $(safplus_top_prefix)/src/*
      do
        file_name=`basename "$file"`
        if [ "$file_name" != "include" ] && [ "$file_name" != "mk" ]
        then
          rm -rf $file
        fi
      done
    fi 
  fi
}
if [ "$1" == "0" ]
then
  print "Preparing to remove the safplus-src...\n"
  check_and_remove_pkg_files
else
  print "Preparing to upgrade the safplus-src"
fi
%files 
%defattr(-,root,root,-)
%{saf_prefix}/*
%{saf_src_prefix}/src/include/*
%{saf_src_prefix}/src/mk/* 
%{prefix}/ide/*

%files src
%defattr(-,root,root,-)
%{saf_src_prefix}/COPYING
%{saf_src_prefix}/Makefile
%{saf_src_prefix}/PACKAGE_GEN
%{saf_src_prefix}/README
%{saf_src_prefix}/VERSION
%{saf_src_prefix}/doc/*
%{saf_src_prefix}/examples/*
%{saf_src_prefix}/DEB/* 
%{saf_src_prefix}/SPECS/*  
%{saf_src_prefix}/src/*  
%{saf_src_prefix}/templates/*
%{saf_src_prefix}/test/*  
%{prefix}/ide/*

%changelog
* Mon Feb 08 2016 OpenClovis Inc <info@openclovis.com>
- This release will contains the git-hub code changes upto commit 220ac7c add code generation menu item and its status bar message.

* Fri Jul 10 2015 OpenClovis Inc <info@openclovis.com> 
- Initial package version for openclovis SAFplus software. 
