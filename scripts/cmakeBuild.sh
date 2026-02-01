#!/bin/bash

# Navigate to the project root (one level up from /scripts)
cd "$(dirname "$0")/.."

# Create build directory if it doesn't exist
mkdir -p build

# Enter build directory, configure, and compile
cd build && cmake .. && make -j4