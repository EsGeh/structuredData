#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

rm -rf $doc_dir
