#!/bin/sh
#

run_cmd() {
    echo running $* ...
    if ! $*; then
			echo failed!
			exit 1
    fi
}


run_cmd aclocal
run_cmd autoheader
run_cmd automake --add-missing --copy
run_cmd autoconf

echo
echo "Now type './configure' to configure MAST"
echo
