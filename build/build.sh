#!/usr/bin/env bash

# Called by build tools (Travis...), don't use directly

cd $(dirname $(readlink -f $0))/..

rm -rf debian
cp -a build/debian.skel debian
make target/version
version=$(< target/version)
echo version=$version

if [[ -n "$1" ]]; then
    echo "Building: $1"
    for f in build/debian.$1/*; do
        sed "s/#VERSION#/$version/g" $f > debian/${f##*/}
    done

    rm -f debian/files
    exec make release.$1
fi
