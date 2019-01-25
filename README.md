# structuredData (sd)

a library for puredata.

## purpose

this library aims at the following goals:

- structured messages

	- hierarchically structured, arbitrarily deeply nested messages 
	- these can be accumulated, serialized, filtered, ...

- objects maintaining internal state

	- a stateful object has "properties"
	- properties can be set/retrieved in a standardized way
	- stateful objs can track other objects state
	- stateful objs can be stored/loaded to/from ram or disk
	- (experimental) gui to inspect and edit objects state

the actual Documentation is in `./c/README.md`

## Directories

- `./c/` : this is the latest version, based mostly on a compiled library (a pd "external") and is written in C.

	For more information, see `./c/README.md`

- `./legacy/`: this directory contains a reference implementation based solely on pd abstractions. This version is no longer maintained and just kept as reference.

	For more information, see `./legacy/README.md`
