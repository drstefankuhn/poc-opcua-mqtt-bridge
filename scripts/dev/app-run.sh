#!/usr/bin/env bash
set -euo pipefail

set -a
# shellcheck disable=SC1091
source .env
set +a

cd bridge
./build/Release/poc-opcua-mqtt-bridge
