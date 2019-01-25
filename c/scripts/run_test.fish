#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

set doc_dir $BASE_DIR/doc
set build_dir $doc_dir/build

# where to install:
set install_prefix $doc_dir/install

# where to install nonC version:
set install_nonC $doc_dir/installNonC

pd -noaudio -path "$install_nonC" -lib "$install_prefix/structuredDataC" "$doc_dir/structuredDataC_doc.pd"
