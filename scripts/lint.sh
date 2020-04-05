#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if ! [ -x "$(command -v cpplint)" ]; then
	echo "Please install cpplint on your machine"
	exit 1
fi

cpplint --exclude="$SRC"/compat/linux/* \
--exclude=$SRC/compat/openbsd/crypto/* \
--exclude=$SRC/compat/openbsd/net80211/* \
--exclude=$SRC/compat/openbsd/sys/* \
--exclude=$SRC/fw/api/* \
--exclude=$SRC/device/* \
--exclude=$SRC/apple80211/* \
--recursive \
--verbose 4 \
$SRC 
