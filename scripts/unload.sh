#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
echo "Build the kext with ./scripts/build.sh before running this script."
exit 123
fi

KEXT="$BUILD/Build/Products/Debug/AppleIntelWifiAdapterV2.kext"
sudo kextunload -v 6 -b com.apple.AppleIntelWifiAdapterV2
