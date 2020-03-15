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

sudo chown -R $USER "$BUILD"
xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD" | xcpretty
