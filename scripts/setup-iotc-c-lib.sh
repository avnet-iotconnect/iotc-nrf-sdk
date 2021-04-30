#!/bin/bash

set -e

git clone --depth 1 --branch v2.0.0 git://github.com/Avnet/iotc-c-lib.git
test -d iotc-c-lib
ln -sf ../common/iotc-c-lib-overlay/CMakeLists.txt iotc-c-lib/
