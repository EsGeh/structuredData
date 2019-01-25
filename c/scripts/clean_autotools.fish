#!/bin/fish

./scripts/run_autotools.fish
./configure
make maintainer-clean
rm -v aclocal.m4
rm -v -r build-aux
rm -v configure
rm -v Makefile.in
rm -r build
