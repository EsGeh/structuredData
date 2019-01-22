#!/bin/fish


set parent_dir (dirname (readlink -m (status filename)))

set DEP_DIR $parent_dir/../dependencies

rm -rf $DEP_DIR
rm $parent_dir/utils/cmd_args.fish
mkdir -p $DEP_DIR
cd $DEP_DIR
git clone git@github.com:EsGeh/fishshell-cmd-opts.git
cd $parent_dir/utils
# ln -s $DEP_DIR/fishshell-cmd-opts/cmd_args.fish
