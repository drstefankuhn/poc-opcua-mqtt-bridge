#!/usr/bin/env bash
set -euo pipefail

all=false
while getopts "a" opt; do
    case $opt in
        a) all=true ;;
        *) echo "Usage: $0 [-a]" >&2; exit 1 ;;
    esac
done

cd bridge

generate_docs() {
    local project_name="$1"
    local output_dir="$2"
    local extract_static="$3"
    local extract_anon="$4"
    shift 4
    local input_paths=("$@")

    local input=""
    for p in "${input_paths[@]}"; do
        input+="$p "
    done

    mkdir -p "$output_dir"

    echo "Generating docs for ${project_name} -> ${output_dir}/html/"

    doxygen - <<EOF
PROJECT_NAME           = "${project_name}"
OUTPUT_DIRECTORY       = ${output_dir}
INPUT                  = ${input}
RECURSIVE              = YES
FILE_PATTERNS          = *.hpp *.cpp
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
EXTRACT_ALL            = NO
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = ${extract_static}
EXTRACT_ANON_NSPACES   = ${extract_anon}
HAVE_DOT               = NO
QUIET                  = YES
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = NO
WARN_AS_ERROR          = NO
SOURCE_BROWSER         = YES
EOF
}

if $all; then
    static=YES
    anon=YES
else
    static=NO
    anon=NO
fi

# Logger
if $all; then
    generate_docs "Logger" libs/logger/docs "$static" "$anon" \
        libs/logger/include libs/logger/src
else
    generate_docs "Logger" libs/logger/docs "$static" "$anon" \
        libs/logger/include
fi

# MQTT
if $all; then
    generate_docs "MQTT" libs/mqtt/docs "$static" "$anon" \
        libs/mqtt/include libs/mqtt/src
else
    generate_docs "MQTT" libs/mqtt/docs "$static" "$anon" \
        libs/mqtt/include
fi

# OPC UA
if $all; then
    generate_docs "OPC UA" libs/opcua/docs "$static" "$anon" \
        libs/opcua/include libs/opcua/src
else
    generate_docs "OPC UA" libs/opcua/docs "$static" "$anon" \
        libs/opcua/include
fi

# Bridge (app)
if $all; then
    generate_docs "Bridge" app/docs "$static" "$anon" \
        app/bridge.hpp app/bridge.cpp app/main.cpp
else
    generate_docs "Bridge" app/docs "$static" "$anon" \
        app/bridge.hpp
fi

echo "Done."
