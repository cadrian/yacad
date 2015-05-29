#!/bin/bash
root=$(dirname $(dirname $(readlink -f $0)))
cd $root/test/integ

sed 's!#PATH#!'"$root"'!g' runner.conf.template > runner.conf

if [ ${gdb-no} = yes ]; then
    exec gdb $root/target/yacad_runner --args $root/target/yacad_runner "$@"
elif [ ${strace-no} = yes ]; then
    strace $root/target/yacad_runner "$@"
elif [ ${valgrind-no} = yes ]; then
    valgrind $root/target/yacad_runner "$@"
fi
exec $root/target/yacad_runner "$@"
