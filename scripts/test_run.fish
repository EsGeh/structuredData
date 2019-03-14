#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

set doc_dir $BASE_DIR/doc


pd -noaudio \
	-noprefs \
	-lib zexy \
	-path "$doc_dir" \
	-lib "structuredDataC" \
	"$doc_dir/structuredDataC_doc.pd"
