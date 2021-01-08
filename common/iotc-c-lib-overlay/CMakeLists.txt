#
# Copyright (c) 2020 Avnet
#
# cmake file for zephyr
#

option(WITH_FEATURE_ZEPHYR "Enable Zephyr integration" OFF)

if(WITH_FEATURE_ZEPHYR)
    add_definitions(-DUSE_ZEPHYR)
endif()

zephyr_include_directories(
        include
)
zephyr_library_sources(
        src/iotconnect_common.c
        src/iotconnect_event.c
        src/iotconnect_telemetry.c
        src/iotconnect_discovery.c
        src/iotconnect_lib.c
)

