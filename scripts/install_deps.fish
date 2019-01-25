#!/bin/fish


set parent_dir (dirname (readlink -m (status filename)))

set DEP_DIR $parent_dir/../dependencies

rm -rf -v $DEP_DIR
mkdir -p -v $DEP_DIR
cd $DEP_DIR
git clone git@github.com:EsGeh/fishshell-cmd-opts.git
