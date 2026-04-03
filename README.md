# PoC: OPC UA to MQTT Bridge (C++)

A C++ bridge that reads values from an OPC UA server and publishes them as JSON to an MQTT broker. What started as a small proof of concept -- integrating two open-source C/C++ libraries (open62541 for OPC UA, Paho for MQTT) -- quickly grew into a compact showcase of the tools, practices, and C++ features I use day to day.

The bridge itself is developed inside a Dev Container with a full C++23 toolchain. Docker Compose spins up two additional containers alongside it: a Python-based OPC UA simulator (asyncua) providing live sensor data, and an Eclipse Mosquitto MQTT broker receiving the bridge output. This gives the project a self-contained development and simulation environment out of the box.

## Getting Started

Clone the repository and reopen it in a Dev Container. Docker Compose builds all three containers automatically. Cursor requires the `anysphere.remote-containers` extension; VS Code requires the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension (and adjustments to the extensions listed in `.devcontainer/devcontainer.json`). Once inside the Dev Container:

```bash
scripts/dev/app-build.sh      # build the bridge
scripts/dev/app-unittest.sh   # run all unit tests
scripts/dev/mqtt-listen.sh    # in a separate terminal: subscribe to MQTT output
scripts/dev/app-run.sh        # execute one bridge cycle against the simulator
```

## Scripts

| Where | Script | Purpose |
|---|---|---|
| Dev container | `scripts/dev/app-build.sh` | Conan install, CMake configure, build |
| Dev container | `scripts/dev/app-unittest.sh` | Run all unit tests via CTest |
| Dev container | `scripts/dev/app-run.sh` | Run the bridge binary once (loads `.env`) |
| Dev container | `scripts/dev/mqtt-listen.sh` | Subscribe to all MQTT topics (`mosquitto_sub`) |
| Dev container | `scripts/dev/check-clang-tidy.sh` | Run clang-tidy on the entire codebase |
| Dev container | `scripts/dev/docs-generate.sh` | Generate Doxygen documentation (`-a` for internal docs) |
| Host | `scripts/host/sim-restart-opcua.sh` | Restart the OPC UA simulator after `server.py` changes |
| Host | `scripts/host/sim-rebuild-opcua.sh` | Rebuild the OPC UA simulator image after Dockerfile changes |
| Host | `scripts/host/sim-logs.sh` | Follow logs for the MQTT broker and OPC UA simulator |

## Project Structure

```
compose.yaml                     # Docker Compose: 3 services
.devcontainer/                   # Dev Container configuration
  Dockerfile                     # C++23 toolchain (GCC, CMake, Conan, Ninja, clang tools, Doxygen)
  devcontainer.json              # VS Code / Cursor integration
simulation/
  opcua/                         # OPC UA simulator (Python, asyncua)
  mqtt/                          # MQTT broker (Eclipse Mosquitto)
bridge/
  CMakeLists.txt                 # Top-level CMake (C++23, Ninja)
  conanfile.txt                  # Conan package manager dependencies
  .clang-format                  # Code formatting rules
  .clang-tidy                    # Static analysis configuration
  app/                           # Bridge application
    main.cpp, bridge.hpp/cpp
    tests/                       # Application-level unit tests
  libs/
    logger/                      # Logger library (ILogger, StdoutLogger)
      include/, src/, tests/
    mqtt/                        # MQTT library (IMqtt, PahoMqtt)
      include/, src/, tests/
    opcua/                       # OPC UA library (IOpcUa, Open62541OpcUa)
      include/, src/, tests/
scripts/
  dev/                           # Developer convenience scripts (build, run, test, lint, docs)
  host/                          # Host-side simulation scripts (logs, restart, rebuild)
```

## Infrastructure

| Component | Technology | Purpose |
|---|---|---|
| **Dev Container** | Debian 13 + GCC + Conan + CMake + Ninja | Reproducible C++23 development environment |
| **OPC UA simulator** | Python (`asyncua`) | Publishes simulated sensor data (counter, temperature, pressure, etc.) |
| **MQTT broker** | Eclipse Mosquitto (Alpine) | Lightweight message broker for bridge output |
| **Orchestration** | Docker Compose | Wires all three services; health checks ensure startup order |
| **Shared configuration** | `.env` file | Single source for hostnames, ports, topics across all containers |

## Build System & Dependencies

| Tool | Role |
|---|---|
| **CMake** (>= 3.23) | Build system generator; CTest for test discovery |
| **Conan** (2.x) | C/C++ package manager (`CMakeDeps` + `CMakeToolchain` generators) |
| **Ninja** | Fast parallel build backend |
| **ccache** | Compiler cache (persistent Docker volume) |

| Dependency | Version | Purpose |
|---|---|---|
| open62541 | 1.5.0 | OPC UA client library (C) |
| paho-mqtt-cpp | 1.6.0 | MQTT client library (C++) |
| nlohmann_json | 3.11.3 | JSON serialization |
| Catch2 | 3.14.0 | Unit testing framework (test-only) |

## Architecture

| Concept | Details |
|---|---|
| **Runtime polymorphism** | Abstract interfaces (`ILogger`, `IMqtt`, `IOpcUa`) with virtual dispatch |
| **PIMPL idiom** | `PahoMqtt` and `Open62541OpcUa` hide all implementation details behind `std::unique_ptr<Impl>` |
| **Dependency injection** | `Bridge` receives `IOpcUa&` and `IMqtt&`; tests inject stubs without linking real libraries |
| **Function dispatch tables** | Swappable function-pointer tables decouple production ops from test stubs |
| **Move-only types** | All public classes are move-only (`= delete` copy, `= default` move) |
| **Error handling without exceptions** | Public APIs return `std::expected<T, std::string>`; exceptions are caught internally and converted |
| **RAII resource management** | `std::experimental::scope_exit` for open62541 C resources; `std::unique_ptr` with custom deleter for `UA_Client` |

## C++ Features

The codebase uses modern C++ across standard versions.

| Standard | Highlights |
|---|---|
| **C++11** | Trailing return types, move semantics, `= default`/`= delete`, `noexcept`, `override`/`final`, lambda expressions, `std::unique_ptr` with custom deleter, `thread_local` |
| **C++14** | Generic lambdas |
| **C++17** | `std::variant`/`std::visit`, `std::optional`, `std::string_view`, nested namespace definitions, structured bindings, `if constexpr`, `if` with initializer, `[[nodiscard]]`, class template argument deduction |
| **C++20** | `std::span`, `std::source_location`, designated initializers, `std::ranges`/`std::views`, `std::format`, `std::chrono` calendar types, `std::to_array`, `constexpr std::string` |
| **C++23** | `std::expected<T, E>` / `std::unexpected` |
| **Experimental** | `std::experimental::scope_exit` (Library Fundamentals TS v3) |

## Code Quality

| Tool | Purpose |
|---|---|
| **clang-format** | Automatic code formatting (LLVM-based style, 120 column limit) |
| **clang-tidy** | Static analysis (clang-analyzer, bugprone, performance, cppcoreguidelines, hicpp, modernize, readability, cert, portability -- all as errors) |
| **Doxygen** | API documentation generation (public and internal modes via `scripts/dev/docs-generate.sh`) |

## Testing Strategy

| Aspect | Approach |
|---|---|
| **Framework** | Catch2 with CTest integration |
| **Isolation** | All tests run without network; real libraries (Paho, open62541) are replaced by function-pointer stubs |
| **Coverage** | Every public method tested for success, failure (std/non-std exceptions), and moved-from state |
| **Internal testing** | Internal headers (`detail/`) are tested directly (value reader, logger bridge, path resolution) |
| **Test state** | `thread_local` singletons avoid global state conflicts |

## AI-Assisted Development

Developed with AI pair programming (Cursor IDE with Claude). All architectural decisions, design direction, and acceptance criteria were set by the developer; the AI served as an implementation and review tool.

| Area | Examples |
|---|---|
| **Implementation** | Rapid prototyping of libraries, bridge application, and test infrastructure |
| **Architecture** | Trade-off analysis (e.g., runtime vs. compile-time polymorphism, error handling strategies) |
| **Review & QA** | Consistency checks, clang-tidy configuration, iterative refinement |
| **Testing** | Unit test suites with stub injection, edge cases, and moved-from object handling |
| **Documentation** | Doxygen comments following a style guide; documentation generation script |
