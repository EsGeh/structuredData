#!/bin/fish


#################################################
# variables
#################################################


set src (dirname (readlink -m (status filename)))/../pd_objs
# set src (pwd)/"pd_objs"
set install_dir "$HOME/.sgPD"

set options_with_descr 'h/help/print help' 'l/link/don\'t copy files but create symbolic links instead' 'd/dest=/destination dir'


#################################################
# functions
#################################################

function options_from_option_descr
	for o in $argv
		set split_res (string split '/' $o)
		echo "$split_res[1]/$split_res[2]"
	end
end

function option_info_from_option_descr
	for o in $argv
		set split_res (string split '/' $o)
		set short $split_res[1]
		set long_temp (string split '=' $split_res[2])
		set long $long_temp[1]
		if test (count $long_temp) -ge 2
			set long "$long ARG"
		end
		set descr $split_res[3]
		echo "-$short|--$long:$descr\n"
	end
	# echo -e $opts_info | column -t -s ':'
end

function print_help
	echo "usage: "(status -f)" [OPTIONS...]"
	echo "OPTIONS:"
	set opts_info (option_info_from_option_descr $options_with_descr)
	echo -e $opts_info | column -t -s ':'
end


#################################################
# cmd line interface
#################################################

set options (options_from_option_descr $options_with_descr)

# parse command line arguments:
argparse $options -- $argv
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
