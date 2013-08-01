Summary: ieee1394 camera image capture/archive program
Name: capture-camserver
Version: 1.1
Release: 1
Group: Applications/Text
Source: %{name}-%{version}.tar.gz
License: none
URL: http://svn/svn/raf/trunk/instruments/camera
Distribution: RHEL 6 Linux
Requires: libdc1394 >= 2.1.0-1 libjpeg libpng exiv2 httpd php postgresql-libs
BuildRequires: libdc1394 >= 2.1.0-1 libjpeg-devel libpng-devel exiv2-devel postgresql-devel
Buildroot: %{_tmppath}/%{name}-root

%description
This program finds any cameras on the Firewire (ieee1394) bus and attempts to capture images from them. The program can output raw CCD data, uncompressed color images (.ppm), lossy compressed imges (.jpg) or lossess compressed images (.png). The Program also keeps a Post- greSQL database up to date with the latest image written, as well as the direction/guid of each camera, and process status (running/not).


%prep
%setup -n capture-camserver

%build
make clean
make
gzip -c capture.man.1 > capture.1.gz

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{{s,}bin,share/man/man{1,5}}
mkdir -p $RPM_BUILD_ROOT/etc/{rc.d/rc3.d,init.d}
mkdir -p $RPM_BUILD_ROOT/var/www/html/camera
cp capture $RPM_BUILD_ROOT/%{_bindir}/
cp capture.1.gz $RPM_BUILD_ROOT/%{_mandir}/man1/
cp capture.conf $RPM_BUILD_ROOT/etc/
cp capture_reset_bus $RPM_BUILD_ROOT/usr/sbin/
cp capture_monitor.sh $RPM_BUILD_ROOT/usr/sbin/
cp capture.sh $RPM_BUILD_ROOT/etc/init.d/capture.sh
cp capture.php $RPM_BUILD_ROOT/var/www/html/camera/

%post
chkconfig httpd on

#setup init to kill capture at runlevel 3
ln -s /etc/init.d/capture.sh /etc/rc.d/rc3.d/K99capture.sh

#setup automount so that accam:/mnt/cam/images => acserver:/mnt/r2/camera_images
echo "/mnt/acserver	/etc/local/acserver.map" >> /etc/auto.master
mkdir /etc/local
echo "r2	acserver.raf.ucar.edu:/mnt/r2" >> /etc/local/acserver.map

#setup udev rules to allow 'ads' to access all firewire devices
echo "#firewire cameras " >> /etc/udev/rules.d/50-udev.rules
echo "SUBSYSTEM==\"firewire\", OWNER=\"ads\", GROUP=\"ads\", MODE=\"0666\" " >> /etc/udev/rules.d/50-udev.rules

#setup syslogd to send kernel messages (for firewire info)
#  and local1 messages (for capture info) to acserver's syslog
echo "#send kernel notice and local1 to acserver for firewire cameras" >> /etc/syslog.conf
echo "kern.notice					@acserver" >> /etc/syslog.conf
echo "local1.*						@acserver" >> /etc/syslog.conf

%postun 
rm /etc/rc.d/rc3.d/K99capture

%clean
rm -rf $RPM_BUILD_ROOT

%files
%config(noreplace) %attr(0664,ads,apache) /etc/capture.conf
%attr(0775,ads,apache) %{_bindir}/capture
%attr(644,root,root) %doc %{_mandir}/man1/capture.1.gz
%attr(0774,ads,apache) /etc/init.d/capture.sh
%attr(0664,ads,apache) /var/www/html/camera/capture.php
%attr(0774,ads,apache) /usr/sbin/capture_monitor.sh
%attr(0774,ads,apache) /usr/sbin/capture_reset_bus

%changelog
* Thu Aug 1 2013 <cjw@ucar.edu> 1.1-1
- Only multicast status to the data net (192.168.184.x).
- remove reload_fw

* Sun Jun 2 2013 <tbaltzer@ucar.edu> 1.0-2
- Needed to have add .sh to /etc/init.d/capture so that when restarting and checking for /usr/bin/capture, 
-     we don't accidently find ourself and think that capture is still running
- Also fixed bug for test being for capture_monitor, since it was already running in a restart instance
-     I'm not sure if this ever worked.

* Wed May 29 2013 <cjw@ucar.edu> 1.0-1
- Up version number.  Current build is out of date.

* Fri Mar 23 2012 <tbaltzer@ucar.edu> 0.7-1
- cjw changed capture stop/start to unload and reload firewire modules
-    this is the reload_fw program (source unfound - use strings reload_fw to see what it does)
- updated capture for /etc/init.d to provide more information to user through syslog
- added a make clean in case .o files exist prior to make
- no longer requires libpqxx libraries

* Thu Jul 29 2011 <wasinger@ucar.edu> 0.5-1
- fixed /etc/init.d/capture:
-   starts were not detecting previous starts
-   stops were not stopping

* Thu Aug 6 2009 <dlagreca@ucar.edu> 0.5-1
- added the capture_reset_bus and capture_monitor portions

* Tue Jul 28 2009 <dlagreca@ucar.edu> 0.5-1
- merged capture and capture-camserver into capture-camserver

* Tue Jun 30 2009 <dlagreca@ucar.edu> 0.5-1
- created initial package
