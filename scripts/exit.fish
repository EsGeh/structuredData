#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"


eval "$SCRIPTS_DIR/test_exit.fish"
$SCRIPTS_DIR/build.fish clean
rm -rf "$DEP_DIR"
