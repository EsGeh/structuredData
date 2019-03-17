#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR_DEFAULT "$BASE_DIR/dependencies"

set fishshell_cmd_opts_version e446daf1741b416ecb83e4741b4cb7f99645bc11


# (syntax: short/long/description)
set options \
	'h/help' \
	"d/deps-dir="

#################################################
# functions
#################################################

function print_help
	echo "usage: "(status -f)" [OPTIONS...]"
	echo
	echo "OPTIONS:"
	echo "  -h | --help"
	echo "  -d | --deps-dir"
end

#################################################
# cmd line interface
#################################################

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
	if set -q _flag_deps_dir
		set deps_dir $_flag_deps_dir
	end
end

#################################################
# actual script
#################################################

if set -q deps_dir
	set DEP_DIR $deps_dir
else
	set DEP_DIR $DEP_DIR_DEFAULT
end


mkdir -p -v $DEP_DIR

function is_up_to_date
	if [ -d $DEP_DIR/fishshell-cmd-opts ]
		cd "$DEP_DIR"/fishshell-cmd-opts
		if [ (git rev-parse HEAD) = $fishshell_cmd_opts_version ]
			cd -
			return 0
		end
		cd -
	end
	return 1
end

if is_up_to_date
	echo "fishshell-cmd-opts version $fishshell_cmd_opts_version found"
else
	if [ "$DEP_DIR" = "$DEP_DIR_DEFAULT" ]
		rm -rf -v $DEP_DIR
		mkdir -p -v $DEP_DIR

		cd "$DEP_DIR"
		# cmd line utils:
		git clone git@github.com:EsGeh/fishshell-cmd-opts.git
		cd fishshell-cmd-opts/
		git checkout $fishshell_cmd_opts_version
	else
		echo "WARNING: incompatible versions!"
	end
end

cd "$BASE_DIR"
eval "$SCRIPTS_DIR/utils/run_autotools.fish"
