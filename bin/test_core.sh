#!/bin/bash
root=$(dirname $(dirname $(readlink -f $0)))
cd $root/test/integ

sed 's!#PATH#!'"$root"'!g' core.conf.template > core.conf

if [ ${gdb-no} = yes ]; then
    exec gdb $root/target/yacad_core --args $root/target/yacad_core "$@"
elif [ ${strace-no} = yes ]; then
    strace $root/target/yacad_core "$@"
elif [ ${valgrind-no} = yes ]; then
    valgrind $root/target/yacad_core "$@"
fi
exec $root/target/yacad_core "$@"
