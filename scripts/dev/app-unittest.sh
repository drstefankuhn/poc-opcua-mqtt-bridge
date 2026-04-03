#!/usr/bin/env bash
set -euo pipefail

cd bridge

ctest --test-dir build/Release -V --output-on-failure
