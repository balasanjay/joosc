#!/usr/bin/env bash
set -e

# Assemble our assembly.
for f in `ls output`
do
  nasm -O1 -f elf -g -F dwarf output/$f
done

# Link our binary.
ld -melf_i386 -o a.out output/*.o

