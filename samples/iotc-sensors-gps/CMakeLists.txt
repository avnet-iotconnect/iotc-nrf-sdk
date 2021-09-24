#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
option(CLION "CLion IDE Integration" OFF)

cmake_minimum_required(VERSION 3.8.2)

if (CLION)
    # instructions for CLION:
    # use File-Settings->Build,Execution,Deployment->Cmake
    # cmake options: -DCLION=on
    # environment CLION=yes;GNUARMEMB_TOOLCHAIN_PATH=<path_to>/gnuarmemb;ZEPHYR_BASE=<path_to>/ncs/zephyr
    # build options: -- -j 4 (if you want faster builds)


    message("Using CLION with " $ENV{GNUARMEMB_TOOLCHAIN_PATH} " " $ENV{ZEPHYR_BASE})

    set(ENV{BOARD} thingy91_nrf9160ns)
    set(ENV{BOARD_DIR} $ENV{ZEPHYR_BASE}/boards/arm/thingy91_nrf9160)

    set(ENV{ZEPHYR_TOOLCHAIN_VARIANT} gnuarmemb)

    set(ENV{CMAKE_AR} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-ar)
    set(ENV{CMAKE_AS} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-as)
    set(ENV{CMAKE_ASM_COMPILER} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc)
    set(ENV{CMAKE_ASM_COMPILER_AR} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc-ar)
    set(ENV{CMAKE_ASM_COMPILER_RANLIB} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc-ranlib)

    set(ENV{CMAKE_ASM_FLAGS_DEBUG} -g)
    set(ENV{CMAKE_ASM_FLAGS_MINSIZEREL} -Os -DNDEBUG)
    set(ENV{CMAKE_ASM_FLAGS_RELEASE} -O3 -DNDEBUG)
    set(ENV{CMAKE_ASM_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG)

    set(ENV{CMAKE_CXX_COMPILER} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-g++)
    set(ENV{CMAKE_CXX_COMPILER_AR} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc-ar)
    set(ENV{CMAKE_CXX_COMPILER_RANLIB} $ENV{GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc-ranlib)
endif ()

set(PM_STATIC_YML_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/project-files/configuration/${BOARD}/pm_static.yml
        )
list(APPEND mcuboot_OVERLAY_CONFIG
        "${CMAKE_CURRENT_SOURCE_DIR}/../../common/project-files//mcuboot_overlay-rsa.conf"
        )

# we use some pragmas to suppress CLion warnings
add_definitions("-Wno-unknown-pragmas")

set(WITH_FEATURE_ZEPHYR)

# TODO: figure out how to deal with path warnings
cmake_policy(SET CMP0076 NEW)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../../common/drivers/sensor)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(iotc-sensors-gps)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
zephyr_compile_definitions(_POSIX_C_SOURCE=200809L)

target_sources(app PRIVATE src/app_common.h)
target_sources(app PRIVATE src/command_handling.c)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/test_certs.c)

# several ways to add the cJSON latest library version 1.7.13
# - update the source files in ncs/nrf/ext/cjson/ and include JSON library in prj.config
# - include the actual library as-is and link it in
# - just add cJSON dir and cJSON.c file. This is the simples approach that we are using, though not the cleanest.
#   NOTE: cJSON is currently in .gitignore. Modify it if you want to add it to your source control

zephyr_include_directories(../../cJSON)
zephyr_library_sources(../../cJSON/cJSON.c)

add_subdirectory_ifdef(CONFIG_BH1749 ../../common/sensors/light_sensor build/common/sensors/light_sensor)
add_subdirectory(../../common/sensors/motion build/common/sensors/motion)
add_subdirectory(../../common/sensors/env_sensors build/common/sensors/env_sensors)
add_subdirectory_ifdef(CONFIG_BOARD_THINGY91_NRF9160NS ../../common/ui build/common/ui)
add_subdirectory(src/gps_controller)

add_subdirectory(../../iotconnect-sdk build/iotconnect-sdk)
