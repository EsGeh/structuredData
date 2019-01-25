#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

if test ! -e $SCRIPTS_DIR/utils/cmd_args.fish
	echo "error: structuredData not installed!"
	exit 1
end

source $SCRIPTS_DIR/utils/cmd_args.fish

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

set build_dir $doc_dir/build

# where to install:
set install_prefix $doc_dir/install

# where to install nonC version:
set install_nonC $doc_dir/installNonC

set cmds build install

#################################################
# functions
#################################################

function print_help
	echo "configure and execute CMD in '$install_prefix'"
	echo "USAGE: "(status -f)" [CMD...]"
	echo "  default for CMD: build install"
end

#################################################
# actual script
#################################################

if [ "$argv[1]" = "-h" ]; or [ "$argv[1]" = "--help" ]
	print_help
	exit 1
end

if test (count $argv) != 0
	set cmds $argv
end

mkdir -pv $build_dir

and begin
	set cmd "$SCRIPTS_DIR/build.fish --build-dir '$build_dir' --prefix '$install_prefix' $cmds"
	echo "executing: '$cmd'"
	eval "$cmd"
end

and begin
	set cmd "$BASE_DIR/../legacy/scripts/install.fish -d '$install_nonC'"
	echo "executing: '$cmd'"
	eval "$cmd"
end
