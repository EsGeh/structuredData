#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

set doc_dir $BASE_DIR/doc

# where to install:
set install $doc_dir/install
set install_legacy $doc_dir/install_legacy


pd -noaudio \
	-noprefs \
	-lib zexy \
	-path "$install" \
	-lib "structuredDataC" \
	-path "$install_legacy" \
	"$doc_dir/structuredDataC_doc.pd"
