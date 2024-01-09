#!/bin/bash

if [ "$#" -le 2 ]; then
    echo "Usage: $0 <source_file> <output>"
    exit 1
fi

filepath=$1
output=$2

if ! ninja -C "build"; then
    exit 1
fi

if ! ./build/bin/qcc-cmd -f $filepath -o $output $3 > /tmp/qcc_log; then
    batcat /tmp/qcc_log -l s -P
    exit 1
fi

chmod +x $output
gdb $output
