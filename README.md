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

- [puredata](https://puredata.info/)
- [zexy](https://git.iem.at/pd/zexy) (just needed for the "gui" object)

## build dependencies

- fish shell
- git

## utility scripts

- init project (download dependencies, ...):

		$ ./scripts/init.fish

- clean project (remove all all temporary files, ...)

		$ ./scripts/exit.fish

## test locally without installing to system

1. install library to a local dir

		$ ./scripts/local_init.fish

2. run docu

		$ ./scripts/local_run.fish

	all objects should appear and work as expected...

3. uninstall library from the local dir

		$ ./scripts/local_exit.fish

## installation

- build and install:

        $ ./scripts/build.fish install

    (for options append `--help`)

- configure pd to load the library

	start pd and add `structuredDataC` under `File -> Settings -> Start -> New`

- configure pd to find abstractions

	start pd and add `<install_location>/structuredDataC` under `File -> Settings -> Path -> New`

## documentation

The Documentation is provided as puredata example patches accessible in pd under `Help -> Patch Browser -> structuredDataC`.
Alternatively, for help with a specific object, right click on it and select "help".
