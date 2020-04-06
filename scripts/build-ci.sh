#!/bin/bash
source "$(cd "$(dirname "$0")" && pwd)"/env.sh

if [ ! -d "$BUILD" ]; then
mkdir $BUILD
fi

GIT_COMMIT=$(git rev-parse HEAD | cut -c 1-8)

BUILDER="\"Built from $GIT_COMMIT by $USER@$(hostname)\""

xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Debug \
	   BUILDER="$BUILDER" \
	   -derivedDataPath "$BUILD" 
xcodebuild -project "$PROJECT" -scheme AppleIntelWifiAdapter -configuration Release \
	   BUILDER="$BUILDER" \
	   -derivedDataPath "$BUILD"
