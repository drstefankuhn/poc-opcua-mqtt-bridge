#!/usr/bin/env bash
set -euo pipefail

set -a
# shellcheck disable=SC1091
source .env
set +a

mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" -t '#' -v
