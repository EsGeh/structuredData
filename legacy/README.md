# structuredData (sd)

this is the legacy implementation based solely on pd abstractions. Its mainly kept as a reference implementation.

## directory structure

- structured messages (see `./pd_objs/data/`)

	- hierarchically structured, arbitrarily deeply nested messages 
	- these can be accumulated, serialized, filtered, ...

- objects maintaining internal state (see `./pd_objs/obj_state/`)

	- a stateful object has "properties"
	- properties can be set/retrieved in a standardized way
	- stateful objs can track other objects state
	- stateful objs can be stored/loaded to/from ram or disk
	- (experimental) gui to inspect and edit objects state

## install dependencies

	$ ./scripts/install_deps.fish

## install

the script

	$ ./scripts/install.fish

will copy all objects into some directory. (check `-h` for options).
To use the objects add this directory to pd's search path.

## uninstall

	$ ./scripts/uninstall.fish [-d <dest>]

(check `-h` for options).

## Attention:

This library contains an experimental Object `sgTest.pd` which is dependent on [structuredDataC](c) .
It is the only object with external dependencies.

## documentation

open `./doc/*` in pd.

# structuredData (sd) in C

Alternative implementation in C. For documentation, see `./c/README.md` .
