#!/bin/bash

set -e

git clone --depth 1 --branch iotc-c-lib-patch https://github.com/avnet-iotconnect/iotc-c-lib.git
test -d iotc-c-lib
cp -f common/iotc-c-lib-overlay/CMakeLists.txt iotc-c-lib/
