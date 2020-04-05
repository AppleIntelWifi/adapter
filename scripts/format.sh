#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if ! [ -x "$(command -v clang-format)" ]; then
        echo "Please install clang-format on your machine"
        exit 1
fi

find -E $SRC -type f \
-name '*.cpp' \
-not -path "$SRC/compat/linux/*" \
-not -path "$SRC/compat/openbsd/crypto/*" \
-not -path "$SRC/compat/openbsd/net80211/*" \
-not -path "$SRC/compat/openbsd/sys/*" \
-not -path "$SRC/fw/*" \
-not -path "$SRC/device/*" \
-not -path "$SRC/apple80211/*" \
-not -path "$SRC/firmware/*" \
-exec clang-format -i --style=Google {} \;

find -E $SRC -type f \
-name '*.c' \
-not -path "$SRC/compat/linux/*" \
-not -path "$SRC/compat/openbsd/crypto/*" \
-not -path "$SRC/compat/openbsd/net80211/*" \
-not -path "$SRC/compat/openbsd/sys/*" \
-not -path "$SRC/fw/*" \
-not -path "$SRC/device/*" \
-not -path "$SRC/apple80211/*" \
-not -path "$SRC/firmware/*" \
-exec clang-format -i --style=Google {} \;

find -E $SRC -type f \
-name '*.h' \
-not -path "$SRC/compat/linux/*" \
-not -path "$SRC/compat/openbsd/crypto/*" \
-not -path "$SRC/compat/openbsd/net80211/*" \
-not -path "$SRC/compat/openbsd/sys/*" \
-not -path "$SRC/fw/*" \
-not -path "$SRC/device/*" \
-not -path "$SRC/apple80211/*" \
-not -path "$SRC/firmware/*" \
-exec clang-format -i --style=Google {} \;

find -E $SRC -type f \
-name '*.hpp' \
-not -path "$SRC/compat/linux/*" \
-not -path "$SRC/compat/openbsd/crypto/*" \
-not -path "$SRC/compat/openbsd/net80211/*" \
-not -path "$SRC/compat/openbsd/sys/*" \
-not -path "$SRC/fw/*" \
-not -path "$SRC/device/*" \
-not -path "$SRC/apple80211/*" \
-not -path "$SRC/firmware/*" \
-exec clang-format -i --style=Google {} \;

