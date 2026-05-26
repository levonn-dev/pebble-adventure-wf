cmake_minimum_required(VERSION 3.20)
project(pebble-adventure-wf C)

set(CMAKE_C_STANDARD 11)

# NOTE: This file is ONLY for CLion code intelligence (indexing, navigation,
# resolving Pebble SDK headers). The watchface is NOT built with CMake - the
# real build is waf via `pebble build`. Nothing here is compiled/linked.

# Active Pebble SDK, via the `current` symlink so it survives SDK updates.
set(PEBBLE_SDK_PATH "$ENV{HOME}/.pebble-sdk/SDKs/current/sdk-core/pebble")

include_directories(
    ${PEBBLE_SDK_PATH}/emery/include          # pebble.h and SDK headers
    ${CMAKE_SOURCE_DIR}/src/c
    ${CMAKE_SOURCE_DIR}/build/include          # generated message_keys.auto.h
    ${CMAKE_SOURCE_DIR}/build/emery/src        # generated resource_ids.auto.h
)

# Index all watchface C sources/headers (no worker_src in this project).
file(GLOB_RECURSE APP_SOURCES "src/c/*.c" "src/c/*.h")

add_library(pebble_adventure_wf ${APP_SOURCES})
