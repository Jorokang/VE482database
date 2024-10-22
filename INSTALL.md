# Compilation

## Prerequisites

1. C++ compiler: Clang >= 6.0
2. CMake >= 2.7

## Linux/macOS Quick Start

```bash
$ mkdir build && cd build
$ cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release ../src
$ cmake --build . -- -j8
$ cd ..
```

If everything goes fine, you will have a lemondb binary `./build/lemondb`.

Note: You can use CMAKE_BUILD_TYPE to switch between debug and release.