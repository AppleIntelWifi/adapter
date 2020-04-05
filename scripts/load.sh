#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
echo "Build the kext with ./scripts/build.sh before running this script."
exit 123
fi

KEXT="$BUILD/Build/Products/Debug/AppleIntelWifiAdapterV2.kext"
(./unload.sh | true)
sudo chown -R $USER $KEXT
read # Wait for user input here
sudo chown -R root:wheel $KEXT && sudo kextload -v 6 $KEXT
