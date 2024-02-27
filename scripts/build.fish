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

# where to install:
set install_prefix "$HOME/.local/lib/pd/extra/structuredDataC"

#################################################
# cmd line opts:
#################################################

# (syntax: short/long/description)
set options_descr \
	'h/help/print help' \
	"p/prefix=/where to install (default: '$install_prefix')" \
	"d/debug/build in debug mode" \
	"s/symlink/symlink pd patches" \
	"c/only-configure/stop after configuring, don\'t run 'make'"

#################################################
# functions
#################################################

function print_help
	echo "usage: "(status -f)" [OPTIONS...] [ARGS...]"
	echo "  build source code. ARGS are passed to make"
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
	if set -q _flag_only_configure
		set only_configure
	end
	if set -q _flag_symlink
		set symlink
	end
end

#################################################
# actual script
#################################################

# build
begin

	if set -q only_configure
		exit
	end
	cd $BASE_DIR/

	set make_args $argv

	# make:
	set cmd make
	if test $install_prefix != ''
		set --append cmd DESTDIR=$install_prefix
	end
	if set --query symlink
		set --append cmd INSTALL_DATA='ln -s'
	end
	set --append cmd $make_args
	echo -e "executing: $cmd"
	$cmd
	or exit 1
end
