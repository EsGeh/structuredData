#!/bin/fish

########################################
# dependencies of this repository
########################################

set dependencies \
	fishshell_cmd_opts \
	c_container_types

# for every entry in $dependencies:
# 	name uri version [init-command]
set fishshell_cmd_opts \
	fishshell-cmd-opts  \
	git@github.com:EsGeh/fishshell-cmd-opts.git	\
	e446daf1741b416ecb83e4741b4cb7f99645bc11

set c_container_types \
	c_container_types \
	https://github.com/EsGeh/c_container_types.git \
	29b9df7446bb934c37cbd4ee7c2b4d4bd485411a
