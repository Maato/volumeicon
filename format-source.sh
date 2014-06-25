#!/bin/sh

for f in src/*.{h,c}; do
    clang-format -style=file $f -i
done

