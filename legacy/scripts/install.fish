#!/bin/fish

source (dirname (readlink -m (status filename)))/utils/cmd_args.fish


#################################################
# variables
#################################################


set src (dirname (readlink -m (status filename)))/../pd_objs
set install_dir "$HOME/.local/lib/pd/extra/structuredData"

# (syntax: short/long/description)
set options_descr 'h/help/print help' 'l/link/don\'t copy files but create symbolic links instead' "d/dest=/destination dir (default: $install_dir)"


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
	if set -q _flag_link
		set copyCmd "ln -v -s \"{}\" \"$install_dir\""
	else
		set copyCmd "cp -v \"{}\" \"$install_dir\""
	end
end


#################################################
# actual script
#################################################

echo "install dir: $install_dir"
echo "copyCmd: $copyCmd"

mkdir -v -p $install_dir

find "$src" \
	-type f \
	\( \
		-name '*.pd' \
		-o \
		-name '*.script' \
	\) \
	-exec /bin/bash -c "$copyCmd" \;
