#!/bin/bash

if ! ninja -C "build"; then
    exit 1
fi

if ! ./build/bin/qcc-test ; then
    exit 1
fi
