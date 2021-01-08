#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_include_directories(
        include
)

zephyr_library_sources(
        src/iotconnect_mqtt.c
        src/nrf_modem_if.c
        src/nrf_cert_store.c
        src/nrf_fota.c
)

