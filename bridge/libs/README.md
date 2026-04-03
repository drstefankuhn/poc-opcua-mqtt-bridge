# `libs/`

Self-contained CMake libraries (e.g. OPC UA helpers, MQTT publisher) live here, each with its own `CMakeLists.txt` and `tests/` when added. Root `bridge/CMakeLists.txt` should `add_subdirectory(libs/<name>)` for each library.
