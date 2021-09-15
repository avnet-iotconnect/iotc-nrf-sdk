export CMAKE_INSTALL_URL=${CMAKE_INSTALL_URL:-https://github.com/Kitware/CMake/releases/download/v3.13.1/cmake-3.13.1-Linux-x86_64.sh}
export GNUARMEMB_TOOLCHAIN_URL=${GNUARMEMB_TOOLCHAIN_PATH:-https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=108bd959-44bd-4619-9c19-26187abf5225&hash=8B0FA405733ED93B97BAD0D2C3D3F62A}
export DTC_INSTALL_URL=http://mirrors.kernel.org/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.5.1-1_amd64.deb


export CMAKE_INSTALL_PATH=${CMAKE_INSTALL_PATH:-/opt/cmake/cmake-3.13.1-Linux-x86_64}
export GNUARMEMB_TOOLCHAIN_PATH=${GNUARMEMB_TOOLCHAIN_PATH:=/opt/gcc-arm-none-eabi-9-2019-q4-major}



export NCS_ROOT=${NCS_ROOT:-/ncs}
export ZEPHYR_BASE="${NCS_ROOT}/zephyr"

export SDK_VERSION=v1.6.1
export LC_ALL=${LC_ALL:-C.UTF-8}
export LANG=${LANG:=C.UTF-8}
export PATH=${GNUARMEMB_TOOLCHAIN_PATH}/bin:${CMAKE_INSTALL_PATH}/bin:${PATH}
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
