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
	https://github.com/EsGeh/fishshell-cmd-opts.git \
	e446daf1741b416ecb83e4741b4cb7f99645bc11

set c_container_types \
	c_container_types \
	https://github.com/EsGeh/c_container_types.git \
	1dd08da6c24fb7e9fc062e48b5760b406810be5f
