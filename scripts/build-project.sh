#!/bin/bash
set -e

target="${1}"
if [[ -z "$target" ]]; then
  echo "Usage: $0 <dk|thingy91|avt9152>"
  exit 1
fi

this_dir=$(dirname "${0}")
source "${this_dir}"/env.sh

source "${NCS_ROOT}"/zephyr/zephyr-env.sh

"${this_dir}"/patch-gettimeofday.sh

case $target in

dk)
  echo "Building for DK..."
  west build -p auto -b nrf9160dk_nrf9160ns -d build_$target -- \
    -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
    -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
    -DCONFIG_LTE_NETWORK_MODE_LTE_M=y
  ;;

thingy91)
  echo "Building for THINGY:91..."
  west build -p auto -b thingy91_nrf9160ns -d build_$target -- \
    -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
    -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
    -DCONFIG_LTE_NETWORK_MODE_LTE_M=y
  ;;

avt9152)
  echo "Building for AVT9152-EVB..."
  west build -p auto -b nrf9160_avt9152ns -d build_$target -- \
    -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
    -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
    -DCONFIG_LTE_NETWORK_MODE_LTE_M=y \
    -DBOARD_ROOT="${this_dir}"/..
  ;;

*)
  echo "Usge: $0 <dk|thingy91|avt9152>"
  exit 1
  ;;

esac

mkdir -p precompiled_image
cp build_$target/zephyr/merged.hex precompiled_image/merged_$target.hex
cp build_$target/zephyr/app_signed.hex precompiled_image/app_signed_$target.hex
cp build_$target/zephyr/app_update.bin precompiled_image/app_update_$target.bin
rm -r build_$target

