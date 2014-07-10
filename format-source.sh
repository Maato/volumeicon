#!/bin/sh

set -e

declare -A ignored=(
    ["alsa_volume_mapping"]=0
    ["keybinder"]=0
    ["bind"]=0
)

for f in src/*.{h,c}; do
    if [[ ${ignored[$(basename ${f%.*})]} ]]; then
        echo "Skipping $f"
        continue
    fi
    echo "Formatting $f"
    clang-format -i -style=file $f
done

