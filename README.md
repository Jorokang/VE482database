# LemonDB Recovered Files

## Introduction

This is the milestone 1 for LemonDB, a single-threaded database system implemented in C++. It supports basic SQL-like table management and data manipulation queries. The compiled binary can handle queries sequentially from standard input or from a file following Lemonion Inc.'s query format.

## Project Structure

- `./src`
  Contains the single-threaded implementation source code. The code is built on top of the provided codebase and uses CMake for building. The code is organized into the following directories:

  - `./src/db`
    Contains the database implementation code.

  - `./src/query`
    Contains the query parser, builder, and executor code. The subdirectory `./src/query/data/` contains the data manipulation query implementation. The subdirectory `./src/query/management/` contains the table management query implementation.

  - `./src/utils`
    - Contains utility code for the project.

## Installation

See `INSTALL.md` for more detailed instructions on building from source.

### Prerequisites

1. C++ compiler: Clang >= 6.0
2. CMake >= 2.7

### Quick Start

First download the source code from the repository. Then under the project root directory, run the following commands:

```bash
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release ../src
cmake --build . -- -j8
cd ..
```

If everything goes fine, you will have a lemondb binary `./build/lemondb`. Then run the following command to launch the database:

```bash
cd build
./lemondb --listen <filename>
```

where `<filename>` is the name of the file containing the queries to be executed.

## Copyright

Lemonion Inc. 2018
