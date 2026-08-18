# Weqi RPM spec（二进制打包）
# 由 package-rpm.sh 调用，把 dist/ 中预编译的二进制与 Python 适配器打包成 RPM。
# 预编译二进制无源码，禁用自动生成的 debuginfo/debugsource 子包。
%global debug_package %{nil}

Name:           weqi
Version:        0.1.0
Release:        1%{?dist}
Summary:        A modern open-source desktop chess application

License:        MIT
URL:            https://github.com/openwelabs/weqi
Source0:        %{name}-%{version}.tar.gz

# 运行依赖：Qt6 Widgets 与 Python 3
Requires:       qt6-qtbase-gui >= 6.2
Requires:       python3

%description
Weqi is a modern, clean open-source desktop chess application.
All chess rules are implemented in a local C++ engine, with support
for human vs human, human vs AI, AI vs AI, and game replay.

%prep
%setup -q -n %{name}-%{version}

%install
rm -rf %{buildroot}
install -d %{buildroot}%{_bindir}
install -d %{buildroot}%{_datadir}/%{name}/ai_adapter/providers

install -m 0755 Weqi %{buildroot}%{_bindir}/%{name}
install -m 0644 ai_adapter/main.py %{buildroot}%{_datadir}/%{name}/ai_adapter/main.py
install -m 0644 ai_adapter/parser.py %{buildroot}%{_datadir}/%{name}/ai_adapter/parser.py
install -m 0644 ai_adapter/providers/__init__.py %{buildroot}%{_datadir}/%{name}/ai_adapter/providers/__init__.py
install -m 0644 ai_adapter/providers/openai_compatible.py %{buildroot}%{_datadir}/%{name}/ai_adapter/providers/openai_compatible.py

%files
%{_bindir}/%{name}
%{_datadir}/%{name}/ai_adapter/main.py
%{_datadir}/%{name}/ai_adapter/parser.py
%{_datadir}/%{name}/ai_adapter/providers/__init__.py
%{_datadir}/%{name}/ai_adapter/providers/openai_compatible.py

%changelog
* Wed Aug 19 2026 Weqi Developers <liuhuiquan12022@outlook.com> - 0.1.0-1
- 初始 RPM 打包
