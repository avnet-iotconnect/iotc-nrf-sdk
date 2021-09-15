#!/bin/bash
set -e
set -x

this_dir=$(dirname "${0}")
source "${this_dir}"/env.sh

apt-get update
apt-get install -y git wget cmake ninja-build gperf ccache dfu-util \
  python3-pip python3-setuptools python3-wheel xz-utils file make gcc-multilib

sudo apt-get -y remove python-cryptography python3-cryptography


# seems that the intended dtc version 1.4.7 to be installed is missing.. try to just install whatever is available
#[ $(apt-cache show device-tree-compiler | grep '^Version: .*$' | grep -Po '(\d.\d.\d+)' | sed 's/\.//g') -ge '146' ] && \\
#  apt-get install device-tree-compiler || \\
#  (wget "${DTC_INSTALL_URL}" && dpkg -i $(basename "${DTC_INSTALL_URL}") && rm -f $(basename "${DTC_INSTALL_URL}"))
apt-get -y install device-tree-compiler

mkdir -p $(dirname "${CMAKE_INSTALL_PATH}")
pushd $(dirname "${CMAKE_INSTALL_PATH}")
wget -q "${CMAKE_INSTALL_URL}" -O cmake-install.sh
sh cmake-install.sh --skip-license | cat
rm -f cmake-install.sh
popd


pushd $(dirname "${GNUARMEMB_TOOLCHAIN_PATH}")
wget -q "${GNUARMEMB_TOOLCHAIN_URL}" -O gcc-arm-none-eabi-linux.tar.bz2
tar -xf gcc-arm-none-eabi-linux.tar.bz2
rm -f gcc-arm-none-eabi-linux.tar.bz2
ln -sf $(basename "${GNUARMEMB_TOOLCHAIN_PATH}") gnuarmemb
popd
