#!/bin/bash
set -e

cd build
cmake ..
make
./Game/GameApp
