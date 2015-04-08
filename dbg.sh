#!/usr/bin/env bash
set -e

# Find out where blaze lives.
shopt -s expand_aliases
source ~/.bash_aliases

# Compile compiler.
blaze build -c dbg :joosc

# Ensure output directory exists and is empty.
rm -r output && mkdir -p output

# Copy the runtime.
cp third_party/cs444/stdlib/5.0/runtime.s output/runtime.s

# Run the compiler over the stdlib and given files.
gdb --args bazel-bin/joosc $@ `find third_party/cs444/stdlib/5.0 -name "*.java"`

