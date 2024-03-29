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
  board_root_flag=
  ;;

thingy91)
  echo "Building for THINGY:91..."
  target_board=thingy91_nrf9160ns
  board_root_flag=
  ;;

avt9152)
  target_board=nrf9160_avt9152ns
  echo "Building for AVT9152-EVB..."
  board_root_flag="-DBOARD_ROOT=${this_dir}/.."
  ;;

*)
  echo "Usge: $0 <dk|thingy91|avt9152>" >&2
  exit 1
  ;;

esac

build_dir=build_$target

# No longer needed with CONFIG_PARTITION_MANAGER_ENABLED=y in mcuboot_overlay-rsa.conf
# Workaround for MCUboot config being invalid when building the first time.
# Otherwise one has to build twice to get a valid build.
# west build -t rebuild_cache -b ${target_board} -d ${build_dir} -- \
#  -DCONFIG_PARTITION_MANAGER_ENABLED=y \
#    "${board_root_flag}"

west build -p auto -b $target_board -d $build_dir -- \
  -DCONFIG_IOTCONNECT_CPID=\"${NRF_SAMPLE_CPID}\" \
  -DCONFIG_IOTCONNECT_ENV=\"${NRF_SAMPLE_ENV}\" \
  -DCONFIG_LTE_NETWORK_MODE_LTE_M=y \
    "${board_root_flag}"

# validate that mcuboot is being built properly
cpme=$(grep CONFIG_PARTITION_MANAGER_ENABLED "${build_dir}/mcuboot/zephyr/.config")
if [[ "${cpme}" != "CONFIG_PARTITION_MANAGER_ENABLED=y" ]]; then
    echo 'MCUboot Build is likely incorrect. Aboriting build!'
    exit 2
fi


mkdir -p precompiled_image
cp $build_dir/zephyr/merged.hex precompiled_image/merged_$target.hex
cp $build_dir/zephyr/app_signed.hex precompiled_image/app_signed_$target.hex
cp $build_dir/zephyr/app_update.bin precompiled_image/app_update_$target.bin
rm -r $build_dir

