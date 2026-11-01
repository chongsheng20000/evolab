#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
${CXX:-g++} -std=c++17 -pthread -O3 -DNDEBUG -Wall -Wextra omega_core.cpp -o omega_core
printf 'Compiled native/omega_core\n'
