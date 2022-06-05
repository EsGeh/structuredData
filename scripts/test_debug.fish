#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"

set doc_dir $BASE_DIR/doc


# (enabled audio, to test performance issues)

set args \
	-nrt \
	-noprefs \
	-stderr \
	-jack \
	-path "$DEP_DIR/zexy" \
	-lib zexy \
	-path "$doc_dir" \
	-lib "structuredDataC" \
	"$doc_dir/structuredDataC_doc.pd"

# gdb -ex "run $args" pd

valgrind \
	--track-origins=yes \
	-- \
	pd $args
