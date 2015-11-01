#!/usr/bin/env bash
set -e

# Find out where blaze lives.
shopt -s expand_aliases
source ~/.bash_aliases

# Compile compiler.
blaze build :joosc

# Ensure output directory exists and is empty.
rm -rf output && mkdir -p output

# Copy the runtime.
cp third_party/cs444/stdlib/5.0/runtime.s output/runtime.s

# Run the compiler over the stdlib and given files.
bazel-bin/joosc $@ `find third_party/cs444/stdlib/5.0 -name "*.java"`

# Assemble our assembly.
for f in `ls output`
do
  nasm -O1 -f elf -g -F dwarf output/$f
done

# Link our binary.
ld -melf_i386 -o a.out output/*.o
