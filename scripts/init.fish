#!/bin/fish

set BASE_DIR (dirname (readlink -m (status filename)))/..
set SCRIPTS_DIR (dirname (readlink -m (status filename)))
set DEP_DIR $BASE_DIR/../dependencies

rm -rf -v $DEP_DIR
mkdir -p -v $DEP_DIR

cd "$DEP_DIR"
git clone git@github.com:EsGeh/fishshell-cmd-opts.git
cd fishshell-cmd-opts/
git checkout e446daf1741b416ecb83e4741b4cb7f99645bc11

cd "$BASE_DIR"
eval "$SCRIPTS_DIR/utils/run_autotools.fish"
