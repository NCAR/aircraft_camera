Summary: ieee1394 camera image capture/archive program
Name: capture-camserver
Version: 0.5
Release: 1
Group: Applications/Text
Source: capture-camserver-0.5.tar.bz2
License: none
URL: http://svn/svn/raf/trunk/instruments/camera
Distribution: RHEL 5.3 Linux
Requires: libdc1394 >= 2.1.0-1 libjpeg libpng libpqxx exiv2 httpd php
BuildRequires: libdc1394 >= 2.1.0-1 libjpeg-devel libpng-devel libpqxx-devel exiv2-devel
Buildroot: %{_tmppath}/%{name}-root

%description
This program finds any cameras  on  the  Firewire  (ieee1394)  bus  and
attempts  to  capture  images from them. The program can output raw CCD
data, uncompressed color images (.ppm), lossy compressed  imges  (.jpg)
or  lossess  compressed  images  (.png). The Program also keeps a Post-
greSQL database up to date with the latest image written,  as  well  as
the direction/guid of each camera, and  process  status  (running/not).

The  program  attempts  to  capture  images  once  per second, the only
notable exception to this occurs when png output is  enabled.  The  png
comression takes too long and the once/second deadline is not met.

%prep
%setup

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{{s,}bin,share/man/man{1,5}}
mkdir -p $RPM_BUILD_ROOT/etc/init.d/
mkdir -p $RPM_BUILD_ROOT/etc/
mkdir -p $RPM_BUILD_ROOT/var/run/capture/
mkdir -p $RPM_BUILD_ROOT/var/www/html/camera
cp capture $RPM_BUILD_ROOT/%{_bindir}/
cp capture.1.gz $RPM_BUILD_ROOT/%{_mandir}/man1/
cp capture.5.gz $RPM_BUILD_ROOT/%{_mandir}/man5/
cp capture.conf $RPM_BUILD_ROOT/etc/
cp capture_reset_bus $RPM_BUILD_ROOT/usr/sbin/
cp capture_monitor.sh $RPM_BUILD_ROOT/usr/sbin/
cp capture.sh $RPM_BUILD_ROOT/etc/init.d/capture
cp capture.php $RPM_BUILD_ROOT/var/www/html/camera/

%post
chkconfig httpd on

%files
%dir %attr(0775,ads,apache) /var/run/capture
%config(noreplace) %attr(0664,ads,apache) /etc/capture.conf
%attr(0775,ads,apache) %{_bindir}/capture
%attr(644,root,root) %doc %{_mandir}/man1/capture.1.gz
%attr(644,root,root) %doc %{_mandir}/man5/capture.5.gz
%attr(0774,ads,apache) /etc/init.d/capture
%attr(0664,ads,apache) /var/www/html/camera/capture.php
%attr(0774,ads,apache) /usr/sbin/capture_monitor.sh
%attr(0774,ads,apache) /usr/sbin/capture_reset_bus

%changelog
* Thu Aug 6 2009 <dlagreca@ucar.edu> 0.5-1
- added the capture_reset_bus and capture_monitor portions

* Tue Jul 28 2009 <dlagreca@ucar.edu> 0.5-1
- merged capture and capture-camserver into capture-cameserver

* Tue Jun 30 2009 <dlagreca@ucar.edu> 0.5-1
- created initial package
