#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
mkdir $BUILD
fi

GIT_COMMIT=$(git rev-parse HEAD | cut -c 1-8)

BUILDER="\"Built from $GIT_COMMIT by $USER@$(hostname)\""

if ! [ -x "$(command -v xcpretty)" ]; then
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug -derivedDataPath "$BUILD"
else
  xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug \
  GCC_PREPROCESSOR_DEFINITIONS='$GCC_PREPROCESSOR_DEFINITIONS KERNEL_LOG=1' \
  BUILDER="$BUILDER" \
  -derivedDataPath "$BUILD" | xcpretty
fi
