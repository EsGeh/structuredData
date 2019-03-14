#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"


eval "$SCRIPTS_DIR/test_exit.fish"
eval "$SCRIPTS_DIR/utils/run_autotools.fish"
rm -rf -v "$DEP_DIR"
./configure
make maintainer-clean
rm -v aclocal.m4
rm -v -r build-aux
rm -v configure
rm -v Makefile.in
rm -r build
