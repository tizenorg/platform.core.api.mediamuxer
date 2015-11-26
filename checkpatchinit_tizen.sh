#!/bin/bash

patch=`dirname $0`/checkpatch_tizen.pl

for i in `find $1 -name "*.c"`
do
$patch $i
done
exit
