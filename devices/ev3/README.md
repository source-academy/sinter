# sinter-ev3

This directory contains a build script to build the Sinter runner for
ev3dev-stretch on the Lego Mindstorms EV3.

You need Docker installed. To build, simply run `./build.sh`. The binary should
be produced at `build-ev3/sinter-ev3`, which you can copy to the EV3 and run.

If you just want to test compilation locally, you can just build as per usual
CMake: (from this directory)

```
$ mkdir build && cd build
$ cmake ..
$ make
```
