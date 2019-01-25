#!/bin/fish

source (dirname (readlink -m (status filename)))/utils/cmd_args.fish


#################################################
# variables
#################################################

# set src "pd_objs"
set install_dir "$HOME/.local/lib/pd/extra/structuredData"
# set install_dir "$HOME/.sgPD"

set options_descr 'h/help/print help' 'd/dest=/destination dir'


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
end


#################################################
# actual script
#################################################

echo "install dir: $install_dir"

set cmd "rm -r $install_dir"

echo "executing: '$cmd'"

eval $cmd

# rm -v -r $install_dir
