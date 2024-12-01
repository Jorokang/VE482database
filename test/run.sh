#!/bin/bash

# This script is for testing the program on server.
# It will auto build the program and run the test cases, including correctness and multithread performance.
# For using more test cases, modify QUERY_FILE and OUTPUT_FILE.

# cd ./src

if [ ! -d "build" ]; then
  mkdir build
  cd build
  ln -s /opt/lemondb/db db
  ln -s /opt/lemondb/sample sample
  cmake -DCMAKE_C_COMPILER=/usr/bin/clang-18 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-18 ../src
else
  cd build
fi

cmake --build . -- -j8

QUERY_FILE="./sample/test.query"
OUTPUT_FILE="test.out"

time ./lemondb --listen $QUERY_FILE >$OUTPUT_FILE
time ./lemondb --listen $QUERY_FILE --thread=1 >$OUTPUT_FILE
diff $OUTPUT_FILE /opt/lemondb/sample_stdout/$OUTPUT_FILE
