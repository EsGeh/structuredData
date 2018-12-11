# structured data

a library for puredata (pd) provided as a collection of "abstractions" (puredata terminology).

## purpose

this library offers utilities to aim for the following goals:

- structured messages (see ./pd_objs/data/)

	- hierarchically structured, arbitrarily deeply nested messages 
	- these can be accumulated, serialized, filtered, ...

- objects maintaining internal state (see ./pd_objs/obj_state/)

	- a stateful object has "properties"
	- properties can be set/retrieved in a standardized way
	- stateful objs can track other objects state
	- stateful objs can be stored/loaded to/from ram or disk
	- (experimental) gui to inspect and edit objects state

## install

the script

	$ ./scripts/install.fish

will copy all objects into some directory. (check "-h" for options).
To use the objects add this directory to pd's search path.

## documentation

open "./doc/\*" in pd.
