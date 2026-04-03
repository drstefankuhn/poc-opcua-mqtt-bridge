#!/bin/sh
set -e

# Listener port comes from env (see repo root .env); keeps Mosquitto in sync with MQTT_PORT.
_port="${MQTT_PORT:-1883}"

cat <<EOF > /tmp/mosquitto.generated.conf
listener ${_port}
allow_anonymous true
log_dest stdout
EOF

exec mosquitto -c /tmp/mosquitto.generated.conf
