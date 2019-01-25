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
set install $doc_dir/install
set install_legacy $doc_dir/install_legacy

set cmds build install

#################################################
# functions
#################################################

function print_help
	echo "install to '$doc_dir'"
	echo "USAGE: "(status -f)
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
	set cmd "$SCRIPTS_DIR/build.fish --prefix '$install' --build-dir '$build_dir' build install"
	echo "executing: '$cmd'"
	eval "$cmd"
end

and begin
	set cmd "$BASE_DIR/../legacy/scripts/install.fish --dest '$install_legacy'"
	echo "executing: '$cmd'"
	eval "$cmd"
end
