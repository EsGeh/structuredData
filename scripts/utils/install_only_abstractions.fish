#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/../..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))/..

source "$SCRIPTS_DIR/utils/cmd_args.fish"


#################################################
# variables
#################################################


set src (dirname (readlink -m (status filename)))/../pd_objs
set install_dir "$HOME/.local/lib/pd/extra/structuredData"
set rename '{}'

# (syntax: short/long/description)
set options_descr \
	"h/help/print help" \
	"l/link/don't copy files but create symbolic links instead" \
	"d/dest=/destination dir (default: '$install_dir')" \
	"s/source=/source dir (default: '$src')" \
	"r/rename=/pattern for filenames on dest ({} is the basename on src). example: '\$(basename {} .pd)_ext.pd'"


#################################################
# functions
#################################################

function print_help
	echo "usage: "(status -f)" [OPTIONS...]"
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
	if set -q _flag_dest
		set install_dir $_flag_dest
	end
	if set -q _flag_rename
		set rename $_flag_rename
	end
	if set -q _flag_link
		set copyCmd "ln -v -s"
	else
		set copyCmd "cp -v"
	end
	if set -q _flag_source
		set src $_flag_source
	end
end

# make '{}' to refer to the (basename {})
set rename (string replace --regex --all -- '{}' '\\\$(basename {})' "$rename")

set copyCmd (string join -- ' ' $copyCmd '"{}"' "\"$install_dir/$rename\"")

#################################################
# actual script
#################################################

echo "src dir: '$src'"
echo "install dir: '$install_dir'"
echo "copyCmd: $copyCmd"

mkdir -v -p $install_dir

find "$src" \
	\( \
		-type f \
		-o \
		-type l \
	\) \
	\( \
		-name '*.pd' \
		-o \
		-name '*.script' \
	\) \
	-exec /bin/bash -c "$copyCmd" \;
