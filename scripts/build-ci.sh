#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
mkdir $BUILD
fi

if ! [ -x "$(command -v xcpretty)" ]; then
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD"
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Release -derivedDataPath "$BUILD"
else
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD" | xcpretty
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Release -derivedDataPath "$BUILD" | xcpretty
fi
