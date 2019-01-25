#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

set build_dir $doc_dir/build

# where to install:
set install $doc_dir/install
set install_legacy $doc_dir/install_legacy

rm -rf $build_dir
rm -rf $install
rm -rf $install_legacy
