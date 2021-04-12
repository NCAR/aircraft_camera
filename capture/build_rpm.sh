#!/bin/sh

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

get_version () {
    awk '/^Version:/{print $2}' $1
}

topdir=${TOPDIR:-$(rpmbuild --eval %_topdir)}

rroot=unknown
rf=repo_scripts/repo_funcs.sh
[ -f $rf ] || rf=/net/www/docs/software/rpms/scripts/repo_funcs.sh
if [ -f $rf ]; then
    source $rf
    rroot=`get_eol_repo_root`
else
    [ -d /net/www/docs/software/rpms ] && rroot=/net/www/docs/software/rpms
fi


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
