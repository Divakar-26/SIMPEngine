#!/bin/bash
# Usage: Android/build.sh <project folder> [debug|release] [extra gradle args]
set -e
if [ -z "$1" ] || [ ! -d "$1" ]; then echo "Usage: Android/build.sh <project folder> [debug|release] [gradle args]"; exit 1; fi
PROJECT="$(cd "$1" && pwd)"
cd "$(dirname "$0")"
VARIANT="${2:-debug}"
shift 2 || true
python3 tools/pack_project.py "$PROJECT"
if [ -z "$ANDROID_HOME" ] && [ -d "$HOME/Android/Sdk" ]; then export ANDROID_HOME="$HOME/Android/Sdk"; fi
if ! grep -q '^cmake.dir' local.properties 2>/dev/null && ! [ -d "$ANDROID_HOME/cmake" ]; then
    echo "cmake.dir=$(dirname "$(dirname "$(command -v cmake)")")" >> local.properties
fi
if [ "$VARIANT" = "release" ]; then TASK=assembleRelease; else TASK=assembleDebug; fi
./gradlew "$TASK" "$@"
find app/build/outputs/apk -name '*.apk'
