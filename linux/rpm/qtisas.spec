
%if 0%{?mlz}
%define fixedversion %{version}
%else
# use fixedversion for builds on build.opensuse.org - needed for deb builds.
%define fixedversion fixed
%define compression gz
%endif

Summary:       SANS analysis and data reduction software
Name:          qtisas
Version:       0.9.18
Release:       1%{?dist}
License:       GPL-3.0+
Source:        %{name}-%{fixedversion}.tar%{?compression:.%{compression}}
BuildRequires: git
BuildRequires: cmake
BuildRequires: mesa-libGLU-devel
BuildRequires: hdf5-devel
BuildRequires: libtiff-devel
BuildRequires: yaml-cpp-devel
BuildRequires: zlib-devel
BuildRequires: python3-devel
%if 0%{?rocky} <= 9
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel
BuildRequires: python3-qt5-devel
%endif
%if 0%{?fedora} >= 39
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtsvg-devel
BuildRequires: python3-pyqt6-devel
%endif
%if 0%{?rocky} == 9 || 0%{?fedora} >= 37
BuildRequires: sip6
BuildRequires: PyQt-builder
%endif
Requires:      mesa-libGLU
Requires:      hdf5
Requires:      libtiff
Requires:      yaml-cpp
Requires:      zlib
Requires:      python3
%if 0%{?rocky} <= 9
Requires:      python3-qt5
%endif
%if 0%{?fedora} >= 39
Requires:      python3-pyqt6
%endif

%undefine __cmake_in_source_build

%description
%{summary}

%prep
%setup -q -n %{name}-%{fixedversion}

%build
%cmake %{?_cmake_skip_rpath} -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
%cmake_build

%install
%cmake_install

%files
%defattr(-,root,root)
%{_bindir}/qtisas
%{_datadir}/applications/qtisas.desktop
%{_datadir}/qtisas/python/*
%{_datadir}/qtisas/qtisas_logo.png
%{_includedir}/gsl115/gsl/*
%{_libdir}/libgsl115.a
