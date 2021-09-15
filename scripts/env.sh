export CMAKE_INSTALL_URL=${CMAKE_INSTALL_URL:-https://github.com/Kitware/CMake/releases/download/v3.13.1/cmake-3.13.1-Linux-x86_64.sh}
export GNUARMEMB_TOOLCHAIN_URL=${GNUARMEMB_TOOLCHAIN_PATH:-https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2}
export DTC_INSTALL_URL=http://mirrors.kernel.org/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.5.1-1_amd64.deb


export CMAKE_INSTALL_PATH=${CMAKE_INSTALL_PATH:-/opt/cmake/cmake-3.13.1-Linux-x86_64}
export GNUARMEMB_TOOLCHAIN_PATH=${GNUARMEMB_TOOLCHAIN_PATH:=/opt/gcc-arm-none-eabi-8-2019-q3-update}



export NCS_ROOT=${NCS_ROOT:-/ncs}
export ZEPHYR_BASE="${NCS_ROOT}/zephyr"

export SDK_VERSION=v1.6.1
export LC_ALL=${LC_ALL:-C.UTF-8}
export LANG=${LANG:=C.UTF-8}
export PATH=${GNUARMEMB_TOOLCHAIN_PATH}/bin:${CMAKE_INSTALL_PATH}/bin:${PATH}
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
