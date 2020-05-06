#! /bin/sh
#
#	Shell script to update the build number
#
PATH=/bin:/usr/bin:/usr/ucb
#
bnum=0`cat buildnum.data 2>/dev/null`
bnum=`expr "$bnum" + 1`
echo $bnum > buildnum.data
echo $bnum
