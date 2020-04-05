#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if ! [ -x "$(command -v clang-format)" ]; then
        echo "Please install clang-format on your machine"
        exit 1
fi



