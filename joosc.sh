#!/usr/bin/env bash
set -e

shopt -s expand_aliases
source ~/.bash_aliases

blaze build :joosc
rm -r output && mkdir -p output
bazel-bin/joosc $@ `find third_party/cs444/stdlib/5.0 -name "*.java"`
for f in `ls output`
do
  nasm -O1 -f elf -g -F dwarf output/$f
done

ld -melf_i386 -o a.out output/*.o
