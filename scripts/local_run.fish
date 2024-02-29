#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR "$BASE_DIR/dependencies"

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

set pd_patch "$doc_dir/structuredDataC/structuredDataC_doc.pd"

set debug 0

#################################################
# cmd line opts:
#################################################

# (syntax: short/long/description)
set options_descr \
	'h/help/print help' \
	'd/debug/debug using valgrind'

#################################################
# functions
#################################################

function print_help
	echo "usage: "(status -f)" [OPTIONS...] [ARGS...]"
	echo "  run example patch from local installation"
	echo "OPTIONS:"
	print_options_descr $options_descr
end

#################################################
# cmd line interface
#################################################

set options (options_descr_to_argparse $options_descr)

# parse command line arguments:
argparse $options -- $argv 2>/dev/null
or begin
	print_help
	exit 1
end
if set -q _flag_h
	print_help
	exit 0
else
	if set -q _flag_debug
		set debug 1
	end
end
if test (count $argv) -gt 0
	set pd_patch $argv
end

set cmd pd
if test $debug != 0
	set cmd valgrind \
		--track-origins=yes \
		-- \
		pd $args
end

set --append cmd \
	-noprefs \
	-nostdpath \
	-stderr \
	-jack \
	-nrt \
	-path "$doc_dir" \
	-lib structuredDataC \
	-lib zexy \
	$pd_patch

$cmd
