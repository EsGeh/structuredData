#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

#################################################
# variables
#################################################

set test_dir $BASE_DIR/test

set build_dir $test_dir/build

# where to install:
set install_prefix $test_dir/install

# where to install nonC version:
set install_nonC $test_dir/installNonC

rm -rf $build_dir
rm -rf $install_prefix
rm -rf $install_nonC
