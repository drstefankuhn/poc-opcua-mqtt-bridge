#!/usr/bin/env bash
set -euo pipefail

cd bridge

conan install . --build=missing -s build_type=Release -s compiler.cppstd=23
cmake --preset conan-release
cmake --build --preset conan-release
