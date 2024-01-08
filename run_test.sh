#!/bin/bash

if ! ninja -C "build"; then
    exit 1
fi

if ! ./build/bin/qcc-test --gtest_color=yes ; then
    exit 1
fi
