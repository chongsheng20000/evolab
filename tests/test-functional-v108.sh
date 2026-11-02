#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
${CXX:-g++} -std=c++17 -pthread -O1 -w tests/functional-v108.cpp -o /tmp/evolab-v108-function-check
/tmp/evolab-v108-function-check
