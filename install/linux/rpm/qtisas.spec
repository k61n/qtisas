
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
BuildRequires: gl2ps-devel
BuildRequires: gsl-devel
BuildRequires: hdf5-devel
BuildRequires: libtiff-devel
BuildRequires: muParser-devel
BuildRequires: yaml-cpp-devel
BuildRequires: zlib-devel
BuildRequires: python3-devel
BuildRequires: sip6
BuildRequires: PyQt-builder
%if 0%{?rhel} == 9
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel
BuildRequires: python3-qt5-devel
Requires:      python3-qt5
Requires:      qt5-qtsvg
%endif
%if 0%{?rhel} >= 10 || 0%{?fedora} >= 40
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtsvg-devel
BuildRequires: python3-pyqt6-devel
Requires:      python3-pyqt6
Requires:      qt6-qtsvg
%endif

%undefine __cmake_in_source_build

%description
%{summary}

%prep
%setup -q -n %{name}-%{fixedversion}

%build
%cmake %{?_cmake_skip_rpath} -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
%cmake_build

%install
%cmake_install

%files
%defattr(-,root,root)
%{_bindir}/qtisas
%{_datadir}/applications/qtisas.desktop
%{_datadir}/qtisas/python/*
%{_datadir}/qtisas/qtisas_logo.png
