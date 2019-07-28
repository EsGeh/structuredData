#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

if test ! -e $SCRIPTS_DIR/utils/cmd_args.fish
	echo "error: '$SCRIPTS_DIR/utils/cmd_args.fish' not found"
	exit 1
end

source $SCRIPTS_DIR/utils/cmd_args.fish

#################################################
# variables
#################################################

set --erase build_dir
set --erase debug # release mode is default
set --erase only_configure # install after build

# one of these is used if not overwritten on cmd line:
set build_dir_dbg "$BASE_DIR/build/debug"
set build_dir_release "$BASE_DIR/build/release"

# where to install:
set install_prefix "$HOME/.local/lib/pd/extra/structuredDataC"

# (syntax: short/long/description)
set options_descr \
	'h/help/print help' \
	"p/prefix=/where to install (default: '$install_prefix')" \
	"d/debug/build in debug mode" \
	"b/build-dir=/where to build (default: one of '$build_dir_release' or '$build_dir_dbg')" \
	"c/only-configure/stop after configuring, don\'t run 'make'"

#################################################
# functions
#################################################

function print_help
	echo "usage: "(status -f)" [OPTIONS...] [CMD...]"
	echo "  calls 'configure', then run 'make' for every CMD (or just 'make' in the case no CMD is given)"
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
	if set -q _flag_prefix
		set install_prefix (readlink -m $_flag_prefix)
	end
	if set -q _flag_debug
		set debug
	end
	if set -q _flag_build_dir
		set build_dir $_flag_build_dir
	end
	if set -q _flag_only_configure
		set only_configure
	end
end

#################################################
# actual script
#################################################

if not set -q build_dir
	if not set -q debug
		set build_dir $build_dir_release
	else
		set build_dir $build_dir_dbg
	end
end

mkdir -p -v "$build_dir"

# build
begin
	echo "cd $build_dir"
	cd $build_dir

	# configure:
	if not set -q debug
		set cmd "$BASE_DIR/configure --prefix=$install_prefix"
	else
		set flags -g -DDEBUG -O0
		set cmd "$BASE_DIR/configure CFLAGS='$flags' --prefix=$install_prefix"
	end
	eval $cmd

	if set -q only_configure
		exit
	end

	set make_cmds $argv
	if test (count $make_cmds) -eq 0
		set make_cmds ''
	end

	# make:
	for cmd in $make_cmds
		echo "executing: '$cmd'"
		if test $cmd = ''
			make
		else
			make $cmd
		end
		or exit 1
	end
end
