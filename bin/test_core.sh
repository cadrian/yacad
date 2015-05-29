#!/bin/bash
root=$(dirname $(readlink -f $0))
cd $root/../test/integ

sed 's!#PATH#!'"$root"'!g' core.conf.template > core.conf

if [ ${gdb-no} = yes ]; then
    exec gdb ../target/yacad_core --args ../target/yacad_core "$@"
elif [ ${strace-no} = yes ]; then
    strace ../target/yacad_core "$@"
elif [ ${valgrind-no} = yes ]; then
    valgrind ../target/yacad_core "$@"
fi
exec ../target/yacad_core "$@"
