#!/bin/bash
DIR=$(echo $PWD | rev | cut -d '/' -f1 | rev)
BUILD="../build"
PROJECT="../AppleIntelWifiAdapter.xcodeproj"

if [ "$DIR" != "scripts" ]; then
BUILD="./build"
PROJECT="./AppleIntelWifiAdapter.xcodeproj"
fi

if [ ! -d "$BUILD" ]; then
mkdir $BUILD
fi

if ! [ -x "$(command -v xcpretty)" ]; then
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD"
else
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD" | xcpretty
fi
