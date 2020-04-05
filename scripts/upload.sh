#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
echo "Could not find the build directory"	
exit 1
fi

if ! [ -x "$(command -v az)" ]; then
        echo "Please install the Azure cli on your machine"
        exit 1
fi

(cd "$BUILD"/Build/Products/Debug; zip -r AppleIntelWifiV2.kext.debug.zip AppleIntelWifiAdapterV2.kext)

(cd "$BUILD"/Build/Products/Release; zip -r AppleIntelWifiV2.kext.release.zip AppleIntelWifiAdapterV2.kext AppleIntelWifiAdapterV2.kext.dSYM)

mv "$BUILD"/Build/Products/Release/AppleIntelWifiV2.kext.release.zip ./
mv "$BUILD"/Build/Products/Debug/AppleIntelWifiV2.kext.debug.zip ./

# Hash both files
cat ./AppleIntelWifiV2.kext.debug.zip | openssl sha256 > AppleIntelWifiV2.kext.debug.zip.sha256 
cat ./AppleIntelWifiV2.kext.release.zip | openssl sha256 > AppleIntelWifiV2.kext.release.zip.sha256 

az storage blob upload -f ./AppleIntelWifiV2.kext.debug.zip -n "kexts/AppleIntelWifiV2.kext.debug.zip" -c '$web'
az storage blob upload -f ./AppleIntelWifiV2.kext.release.zip -n "kexts/AppleIntelWifiV2.kext.release.zip" -c '$web'
az storage blob upload -f ./AppleIntelWifiV2.kext.debug.zip.sha256 -n "kexts/AppleIntelWifiV2.kext.debug.zip.sha256" -c '$web'
az storage blob upload -f ./AppleIntelWifiV2.kext.release.zip.sha256 -n "kexts/AppleIntelWifiV2.kext.release.zip.sha256" -c '$web'
