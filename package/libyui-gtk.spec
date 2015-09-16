#
# spec file for package libyui-gtk
#
# Copyright (c) 2014 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#


Name:           libyui-gtk
Version:        2.44.5
Release:        0
Source:         %{name}-%{version}.tar.bz2

%define so_version 7
%define bin_name %{name}%{so_version}

BuildRequires:  boost-devel
BuildRequires:  cmake >= 2.8
BuildRequires:  gcc-c++
BuildRequires:  gtk3-devel
BuildRequires:  libyui-devel >= 3.0.4
BuildRequires:  pkg-config
Provides:       yui_backend = %{so_version}

Url:            http://github.com/libyui/
Summary:        Libyui - Gtk User Interface
License:        LGPL-2.1 or LGPL-3.0
Group:          System/Libraries

%description
This package contains the Gtk user interface
component for libYUI.


%package -n %{bin_name}

Requires:       libyui%{so_version}
Provides:       %{name} = %{version}
Provides:       yast2-gtk = %{version}
Obsoletes:      yast2-gtk < %{version}

Url:            http://github.com/libyui/
Summary:        Libyui - Gtk User Interface
Group:          System/Libraries

%description -n %{bin_name}
This package contains the Gtk user interface
component for libYUI.



%package devel

Requires:       boost-devel
Requires:       glibc-devel
Requires:       libstdc++-devel
Requires:       %{bin_name} = %{version}

Url:            http://github.com/libyui/
Summary:        Libyui-gtk header files
Group:          Development/Languages/C and C++

%description devel
This package contains the Gtk user interface
component for libYUI.


This can be used independently of YaST for generic (C++) applications.
This package has very few dependencies.


%prep
%setup -q -n %{name}-%{version}

%build

export CFLAGS="$RPM_OPT_FLAGS -DNDEBUG -Wno-error=deprecated-declarations"
export CXXFLAGS="$RPM_OPT_FLAGS -DNDEBUG -Wno-error=deprecated-declarations"

./bootstrap.sh %{_prefix}

mkdir build
cd build

%if %{?_with_debug:1}%{!?_with_debug:0}
cmake .. \
    -DPREFIX=%{_prefix} \
    -DDOC_DIR=%{_docdir} \
    -DLIB_DIR=%{_lib} \
    -DCMAKE_BUILD_TYPE=RELWITHDEBINFO
%else
cmake .. \
    -DPREFIX=%{_prefix} \
    -DDOC_DIR=%{_docdir} \
    -DLIB_DIR=%{_lib} \
    -DCMAKE_BUILD_TYPE=RELEASE
%endif

make %{?jobs:-j%jobs}

%install
cd build
make install DESTDIR="$RPM_BUILD_ROOT"
install -m0755 -d $RPM_BUILD_ROOT/%{_docdir}/%{bin_name}/
install -m0755 -d $RPM_BUILD_ROOT/%{_libdir}/yui
install -m0644 ../COPYING* $RPM_BUILD_ROOT/%{_docdir}/%{bin_name}/

%clean
rm -rf "$RPM_BUILD_ROOT"

%post -n %{bin_name} -p /sbin/ldconfig

%postun -n %{bin_name} -p /sbin/ldconfig

%files -n %{bin_name}
%defattr(-,root,root)
%dir %{_libdir}/yui
%{_libdir}/yui/lib*.so.*
%doc %dir %{_docdir}/%{bin_name}
%doc %{_docdir}/%{bin_name}/COPYING*

%files devel
%defattr(-,root,root)
%dir %{_docdir}/%{bin_name}
%{_libdir}/yui/lib*.so
%{_prefix}/include/yui
%{_libdir}/pkgconfig/%{name}.pc
%{_libdir}/cmake/%{name}

%changelog
