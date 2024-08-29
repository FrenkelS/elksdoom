## ELKSDOOM
![ELKSDOOM](readme_imgs/elksdoom.png?raw=true)

Doom was originally designed in 1993 for 32-bit DOS computers with 4 MB of RAM.
It's mostly written in C code with very little assembly code.
It has been ported to all kinds of systems.
Usually these systems are 32-bit or more and have a flat memory model.

ELKSDOOM is a port for [ELKS](https://github.com/ghaerr/elks).
It's based on [Doom8088](https://github.com/FrenkelS/Doom8088).

## Controls:
Not implemented yet

## Cheats:
Not implemented yet

## Command line arguments:
|Command line argument|Effect       |
|---------------------|-------------|
|-timedemo demo3      |Run benchmark|

## Building:
1) Install [Open Watcom V2.0](https://github.com/open-watcom/open-watcom-v2) on Ubuntu.

2) Change the paths in `setenvwc.sh` and run `source ./setenvwc.sh` to set the environment variables.

3) [Build the ELKS C Library](https://github.com/ghaerr/elks/wiki/Using-OpenWatcom-C-with-ELKS#build-the-elks-c-library) for the medium memory model.

4) Run `./compelks.sh` to compile `elksdoom.os2`.
