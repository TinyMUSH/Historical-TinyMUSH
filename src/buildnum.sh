#! /bin/sh
#
#	Shell script to update the build number
#
PATH=/bin:/usr/bin:/usr/ucb
#
if test ! -f buildnum.data; then
    echo 0 > buildnum.data
fi
bnum=`awk '{ print $1 + 1 }' < buildnum.data`
echo $bnum > buildnum.data
echo $bnum
