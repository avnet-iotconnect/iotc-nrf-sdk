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
  target_board=nrf9160dk_nrf9160ns
  ;;

thingy91)
  echo "Building for THINGY:91..."
  target_board=thingy91_nrf9160ns
  ;;

avt9152)
  target_board=nrf9160_avt9152ns
  echo "Building for AVT9152-EVB..."
  ;;

*)
  echo "Usge: $0 <dk|thingy91|avt9152>"
  exit 1
  ;;

esac

build_dir=build_$target

# workaround for spm config being invalid, maybe applicable only on windows?
# echo q | west build -t spm_menuconfig -b $target_board -d $build_dir

west build -p auto -b $target_board -d $build_dir -- \
  -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
  -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
  -DCONFIG_LTE_NETWORK_MODE_LTE_M=y

mkdir -p precompiled_image
cp $build_dir/zephyr/merged.hex precompiled_image/merged_$target.hex
cp $build_dir/zephyr/app_signed.hex precompiled_image/app_signed_$target.hex
cp $build_dir/zephyr/app_update.bin precompiled_image/app_update_$target.bin
rm -r $build_dir

