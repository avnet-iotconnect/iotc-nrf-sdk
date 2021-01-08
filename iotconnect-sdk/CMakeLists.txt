#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(
        include
        cert
)

zephyr_library_sources(
        src/iotconnect.c
)

add_subdirectory(../iotc-c-lib build/iotc-c-lib)
add_subdirectory(nrf-layer-lib build/nrf-layer-lib)





