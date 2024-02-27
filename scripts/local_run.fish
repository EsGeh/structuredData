#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"

set doc_dir $BASE_DIR/doc


set cmd pd

# set cmd valgrind \
# 	--track-origins=yes \
# 	-- \
# 	pd $args

set doc_path "$doc_dir/usr/local/lib/pd-externals"

set zexy_path "$doc_path/zexy"
set sd_path "$doc_path/structuredDataC"

set --append cmd \
	-noprefs \
	-nostdpath \
	-stderr \
	-jack \
	-nrt \
	-path "$zexy_path" \
	-path "$sd_path" \
	-lib zexy \
	-lib "structuredDataC" \
	"$sd_path/structuredDataC_doc.pd"

$cmd
