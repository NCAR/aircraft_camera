#!/bin/sh

if grep -q Fedora /etc/redhat-release; then
    echo "Error: noarch RPMs built on Fedora seem to be incompatible with RHEL"
    echo "Until this is resolved, do this build on a RHEL system."
    echo "/etc/redhat-release = $(</etc/redhat-release)"
    echo "rpmbuild --version = " `rpmbuild --version`
    exit 1
fi

script=`basename $0`

if [ "$1" == "-h" -o "$1" == "--help" ];then
    echo "$script [-i] "
    echo "-i: install RPM on EOL yum repository (if accessible)"
    exit 1
fi

doinstall=false

case $1 in
-i)
    doinstall=true
    shift
    ;;
esac

source repo_scripts/repo_funcs.sh

get_version () {
    awk '/^Version:/{print $2}' $1
}

topdir=`get_rpm_topdir`
rroot=`get_eol_repo_root`

log=/tmp/$script.$$
trap "{ rm -f $log; }" EXIT

set -o pipefail

dopkg=all

pkg=capture-camserver
if [ "$dopkg" == all -o "$dopkg" == $pkg ];then
    version=`get_version $pkg.spec`
    mkdir $pkg
    cp -r * $pkg
    tar czf ${topdir}/SOURCES/${pkg}-${version}.tar.gz --exclude .svn --exclude "*.swp" $pkg 
    rm -rf $pkg
    #rpmbuild -ba --clean  ${pkg}.spec | tee -a $log  || exit $?
    rpmbuild -ba  ${pkg}.spec | tee -a $log  || exit $?
fi

echo "RPMS:"
egrep "^Wrote:" $log
rpms=`egrep '^Wrote:' $log | egrep /RPMS/ | awk '{print $2}'`

if $doinstall; then
    if [ -d $rroot ]; then
        echo "Moving rpms to $rroot"
        copy_rpms_to_eol_repo $rpms
    else
        echo "$rroot not found. Leaving RPMS in $topdir"
    fi
else
    echo "-i option not specified, RPMs will not be installed in $rroot"
fi
