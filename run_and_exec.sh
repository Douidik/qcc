#!/bin/bash

# Check if a file argument is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <source_file>"
    exit 1
fi

# Extract the file name without the extension
filename=$(basename "$1" .c)

# Run ninja to build the project
if ! ninja -C "build"; then
    exit 1
fi

# Generate assembly code using qcc-cmd
if ! ./build/bin/qcc-cmd "$1" > "build/$filename.asm"; then
    batcat $1
    exit 1
fi

# Assemble the assembly code using NASM
if ! nasm -g -f elf64 "build/$filename.asm"; then
    batcat "build/$filename.asm"
    exit 1
fi

output=./build/$filename

# Link the object file using the GNU linker
if ! ld -m elf_x86_64 "build/$filename.o" -o $output; then
    batcat "build/$filename.asm"
    exit 1
fi


chmod +x $output
sh $output
