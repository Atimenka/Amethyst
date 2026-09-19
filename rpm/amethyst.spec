%global qt_ver 6

Name:           amethyst
Version:        ${TAG_VERSION}
Release:        10%{?dist}

Summary:        Windows-focused IDE for low-level development (fork of Cremniy)
Summary(ru):    Среда разработки низкоуровневого ПО (форк Cremniy)

License:        GPL-3.0
URL:            https://github.com/Atimenka/Amethyst

Source0:        %{name}-%{version}.tar.gz

BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: qt%{qt_ver}-qtbase-devel
BuildRequires: qt%{qt_ver}-qttools-devel
BuildRequires: qt%{qt_ver}-qtsvg-devel
BuildRequires: desktop-file-utils
BuildRequires: http-parser-devel

Requires: qt%{qt_ver}-qtbase
Requires: qt%{qt_ver}-qttools
Requires: qt%{qt_ver}-qtsvg

%description
Amethyst is a low-level development environment, combining code editor,
HEX editor, and disassembler. It is a fork of Cremniy.

%description -l ru
Amethyst — интегрированная среда для низкоуровневой разработки, объединяющая
редактор кода, HEX-редактор и дизассемблер. Является форком Cremniy.

%prep
%autosetup -n %{name}

%build
rm -rf build

%cmake -S src/ \
    -DCMAKE_BUILD_TYPE=Release

%cmake_build

%install
%{__rm} -rf %{buildroot}

%cmake_install

install -Dm644 \
    docs/amethyst_icon_stroke.svg \
    %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/amethyst.svg

desktop-file-install \
    --dir=%{buildroot}%{_datadir}/applications \
    rpm/%{name}.desktop

install -d %{buildroot}%{_bindir}/Resources/translations

%{__cp} %{__cmake_builddir}/Resources/translations/*.qm \
   %{buildroot}%{_bindir}/Resources/translations/

%post
%{_bindir}/update-desktop-database %{_datadir}/applications >/dev/null 2>&1 || :
%icons_scriptlet

%postun
%{_bindir}/update-desktop-database %{_datadir}/applications >/dev/null 2>&1 || :
%icons_scriptlet

%files
%license LICENSE
%doc README.md

%{_bindir}/amethyst

%{_datadir}/applications/%{name}.desktop

%{_datadir}/icons/hicolor/scalable/apps/amethyst.svg

%{_bindir}/Resources/translations/*.qm

%changelog
* Sat Sep 19 2026 Atimenka <pelmendikii@gmail.com>
- Initial Amethyst fork package
