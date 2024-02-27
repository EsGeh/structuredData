#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"

set doc_dir $BASE_DIR/doc


# (enabled audio, to test performance issues)
pd \
	-noprefs \
	-nostdpath \
	-stderr \
	-jack \
	-nrt \
	-path "$DEP_DIR/zexy" \
	-lib zexy \
	-lib "structuredDataC" \
	"$doc_dir/usr/local/lib/pd-externals/structuredDataC/structuredDataC_doc.pd"
