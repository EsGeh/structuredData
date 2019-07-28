#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR_DEFAULT "$BASE_DIR/dependencies"

set get_deps_version "4ce0cc27b92d6ef78bcf318f6045958ac84a2562"

mkdir -p "$DEP_DIR_DEFAULT"

set get_deps_uri "https://github.com/EsGeh/git_deps/raw/$get_deps_version/get_deps.fish"
begin
	wget \
		--output-document="$DEP_DIR_DEFAULT/get_deps.fish" \
			"$get_deps_uri" 2>/dev/null
	and chmod u+x "$DEP_DIR_DEFAULT/get_deps.fish"
end
or begin
	echo "ERROR while downloading '$repo_uri'"
	exit 1
end


and begin
	eval "$DEP_DIR_DEFAULT/get_deps.fish" $argv
end

eval "$SCRIPTS_DIR/utils/run_autotools.fish"
