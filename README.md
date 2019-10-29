# structuredData (sd)

A library for the famous [puredata](https://puredata.info/) music software.

## purpose

This library shows one flavour of writing abstractions/patches in puredata and how to exchange specifically structured messages between them.

In more detail: The library provides utilities to realize specific design principles in puredata, namely:

- structured messages

	- hierarchically structured, arbitrarily deeply nested messages 
	- these can be accumulated, serialized, filtered, ...

- objects maintaining internal state

	- a stateful object has "properties"
	- properties can be set/retrieved in a standardized way

The goals/benefits are:

- a generic GUI to inspect and manipulate stateful objects
- record all "events" happening at runtime
- stateful objs can track other objects state
- the state of an object can be saved/loaded to/from RAM/disk

## implementation

Most objects are written in C, a few are provided as puredata "abstractions".

## dependencies

- zexy (just needed for the "gui" object)

## build dependencies

- fish shell
- autotools
- git

## installation

### into some local dir (recommended)

see how the scripts used in chapter "testing" work...!

### into the system dir

1. init project (download dependencies, ...):

		$ ./scripts/init.fish

2. build and install:

	build the library:

		$ ./scripts/build.fish build install

	(for options append `--help`)

3. add to pd libraries:

	you have two options:

	- start pd from the command line like this:

			$ pd [-path <install_path>] -lib structuredDataC

	- add this to `~/.pdsettings` :

			...
			loadlib1: structuredDataC
			...

		you might also have to adjust the library search path:

			...
			path1: <install_path>
			...

	(manuall adjust the numbering!)

## other scripts

- uninstall:

		$ ./scripts/build.fish uninstall

- clean project (remove all all temporary files, ...)

		$ ./scripts/exit.fish

## documentation

The Documentation is provided as puredata example patches.
Right click, select "help" opens an example.

## testing

to test the library, issue these commands:

1. install library to a local dir

		$ ./scripts/test_init.fish

2. run docu

		$ ./scripts/test_run.fish

	all objects should appear and work as expected...

3. uninstall library from the local dir

		$ ./scripts/test_exit.fish
