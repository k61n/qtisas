
Summary:       SANS analysis and data reduction software
Name:          qtisas
Version:
Release:       1%{?dist}
License:       GPL-3.0+
Source:        %{name}-fixed.tar
BuildRequires: git
BuildRequires: cmake
BuildRequires: mesa-libGLU-devel
BuildRequires: hdf5-devel
BuildRequires: libpng-devel
BuildRequires: libtiff-devel
BuildRequires: yaml-cpp-devel
BuildRequires: zlib-devel
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel
BuildRequires: python3-devel
BuildRequires: python3-qt5-devel
%if 0%{?rocky} == 9  || 0%{?fedora} >= 37
BuildRequires: sip6
BuildRequires: PyQt-builder
%endif
Requires:      mesa-libGLU
Requires:      python3
Requires:      python3-qt5
Obsoletes:     qtiSAS == 2021.05.12

%undefine __cmake_in_source_build

%description
%{summary}

%prep
%setup -q -n %{name}-fixed

%build
%cmake %{?_cmake_skip_rpath} -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_BUILD_TYPE=Release -DWITH_PYTHON=ON
%cmake_build

%if 0%{?fedora}

%install
%cmake_install

%else

install -d %{buildroot}/usr/bin
install -d %{buildroot}/usr/share/applications
install -d %{buildroot}/usr/share/qtisas/python
strip bin/qtisas
cp bin/qtisas %{buildroot}/usr/bin
cp linux/qtisas.desktop %{buildroot}/usr/share/applications
cp qtisas/python/qtisasrc.py %{buildroot}/usr/share/qtisas/python
cp qtisas/python/python-qtiUtil.py %{buildroot}/usr/share/qtisas/python
cp qtisas/python/qti_wordlist.txt %{buildroot}/usr/share/qtisas/python
cp qtisas/icons/qtisas_logo.png %{buildroot}/usr/share/qtisas

%endif

%files
%defattr(-,root,root)
%{_bindir}/qtisas
%{_datadir}/applications/qtisas.desktop
%{_datadir}/qtisas/python/*
%{_datadir}/qtisas/qtisas_logo.png
