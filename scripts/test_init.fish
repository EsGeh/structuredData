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

and begin
	set cmd "$SCRIPTS_DIR/build.fish --prefix '$doc_dir' build install"
	echo "executing: '$cmd'"
	eval "$cmd"
end
# replace copies with links:
and begin
	and rm $doc_dir/*.pd
	set cmd "$SCRIPTS_DIR/utils/install_only_abstractions.fish --source '$BASE_DIR/pd_objs' --dest '$doc_dir' --link"
	echo "executing: '$cmd'"
	eval "$cmd"
end
