#!/bin/bash
root=$(dirname $(readlink -f $0))
cd $root/../test/integ

sed 's!#PATH#!'"$root"'!g' runner.conf.template > runner.conf

if [ ${gdb-no} = yes ]; then
    exec gdb ../target/yacad_runner --args ../target/yacad_runner "$@"
elif [ ${strace-no} = yes ]; then
    strace ../target/yacad_runner "$@"
elif [ ${valgrind-no} = yes ]; then
    valgrind ../target/yacad_runner "$@"
fi
exec ../target/yacad_runner "$@"
