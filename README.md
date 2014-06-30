# libcurvecpr-asio

Boost.ASIO header-only bindings for libcurvecpr.

## Installation

Requires a patched version of libcurvecpr from [kostko/libcurvecpr](https://github.com/kostko/libcurvecpr).

Assuming default library and installation locations, the bindings can be installed by using:

```
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

## Examples

Example server and client implementations can be found under [libcurvecpr-asio/examples](libcurvecpr-asio/examples).