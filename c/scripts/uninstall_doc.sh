#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

set build_dir $doc_dir/build

# where to install:
set install_prefix $doc_dir/install

# where to install nonC version:
set install_nonC $doc_dir/installNonC

rm -rf $build_dir
rm -rf $install_prefix
rm -rf $install_nonC
