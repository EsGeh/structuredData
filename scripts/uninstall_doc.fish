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

rm -rf $build_dir
rm -rf $install
