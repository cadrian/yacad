#!/bin/dash

catcount() {
    echo -n "| $1"
    emph="$2"
    shift 2
    echo -n ' | '"$emph"$(cat "$@" | wc -l)' lines'"$emph"
    echo -n ' | '"$emph"$(cat "$@" | egrep -v '^ *(/?(\*|/)|$)' | wc -l)' lines'"$emph"
    echo ' |'
}

count() {
    echo '| Name | With comments | Without comments |'
    echo '| ----: | ----: | ----: |'
    catcount '_Total_' '_' "$@"
    for f in "$@"; do
        catcount '`'"$f"'`' '' "$f"
    done
    echo
}

#echo '\defgroup stats Statistics'
echo '# Statistics #'
echo

while read dir name; do
    echo "# $name files #"

    echo '## Header files ##'
    (
        cd src; count $(find $dir -name \*.h | sed 's|^\./||g')
    )

    echo '## Source files ##'
    (
        cd src; count $(find $dir -name \*.c | sed 's|^\./||g')
    )
done <<EOF
common Common
core Core
runner Runner
EOF

echo '# Test files #'

echo '## Unit tests ##'
(
    cd test/unit; count *.[ch]
)
