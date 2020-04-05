#!/bin/bash
DIR=$(echo $PWD | rev | cut -d '/' -f1 | rev)
BUILD="../build"
PROJECT="../AppleIntelWifiAdapter.xcodeproj"
SRC="../AppleIntelWifiAdapter/"

if [ "$DIR" != "scripts" ]; then
BUILD="./build"
PROJECT="./AppleIntelWifiAdapter.xcodeproj"
SRC="./AppleIntelWifiAdapter/"
fi
