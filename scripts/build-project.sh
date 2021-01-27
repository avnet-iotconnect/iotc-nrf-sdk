#!/bin/bash
set -e

this_dir=$(dirname "${0}")
source "${this_dir}"/env.sh

source "${NCS_ROOT}"/zephyr/zephyr-env.sh

"${this_dir}"/patch-gettimeofday.sh

west build -p auto -b thingy91_nrf9160ns -- \
  -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
  -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
  -DCONFIG_LTE_NETWORK_MODE_LTE_M=y
