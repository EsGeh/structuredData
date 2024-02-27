#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))

if test ! -e $SCRIPTS_DIR/utils/cmd_args.fish
	echo "error: fishshell-cmd-opts not installed!"
	exit 1
end

source $SCRIPTS_DIR/utils/cmd_args.fish

#################################################
# variables
#################################################

set doc_dir $BASE_DIR/doc

#################################################
# functions
#################################################

function print_help
	echo "call './build.fish --dest '$doc_dir' install"
end

#################################################
# actual script
#################################################

if [ "$argv[1]" = "-h" ]; or [ "$argv[1]" = "--help" ]
	print_help
	exit 1
end

and begin
	set cmd "$SCRIPTS_DIR/build.fish --prefix '$doc_dir' install"
	echo "executing: '$cmd'"
	eval "$cmd"
end
