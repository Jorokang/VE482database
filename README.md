# VE482 coursework

## Introduction

LemonDB is a high-performance, multi-threaded database system written in C++. It supports SQL-like table management and data manipulation queries, optimized for scalability and parallelism. The system can process queries sequentially or concurrently from standard input or files in Lemonion Inc.'s custom query format.

## Key Features

- **SQL-like Query Language**: LemonDB supports table management and data manipulation operations. including `SELECT`, `INSERT`, `DELETE`, `COUNT` and more.
- **Multi-threaded Design**: LemonDB is designed to be multi-threaded, allowing for concurrent query processing and improved performance.
- **Modular Architecture**: The source code is organized with clear separation of database logic, thread management, and query processing, making it easy to extend and maintain.

## Project Structure

- `./src`
  Contains the single-threaded implementation source code. The code is built on top of the provided codebase and uses CMake for building. The code is organized into the following directories:

  - `./src/db`
    Contains the database implementation code.

  - `./src/query`

    - Contains the query parser, builder, thread, manager, and executor code. The subdirectory `./src/query/data/` contains the data manipulation query implementation. The subdirectory `./src/query/management/` contains the table management query implementation.
    - The `./src/query/Multithread.h` file contains the multithreaded query processing logic.

  - `./src/utils`
    - Contains utility code for the project.

## Installation

See `INSTALL.md` for more detailed instructions on building from source.

### Prerequisites

1. C++ compiler: Clang >= 6.0
2. CMake >= 2.7

### Building from Source

First download the source code from the repository. Then under the project root directory, run the following commands:

```bash
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release ../src
cmake --build . -- -j8
cd ..
```

If everything goes fine, you will have a lemondb binary `./build/lemondb`. Then run the following command to launch the database:

## Quick Start

To run the database, use the following command:

```bash
cd build
./lemondb --listen <filename> --threads=<num_threads>
```

where `<filename>` is the name of the file containing the queries to be executed and `<num_threads>` is the number of threads to be used for query processing.

For more detailed usage information such as query format, see wiki.

## Copyright

Lemonion Inc. 2018
