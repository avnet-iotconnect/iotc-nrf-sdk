#!/bin/bash
set -e

this_dir=$(dirname "${0}")
source "${this_dir}"/env.sh

# NOTE: You should be able to pip install this for the USER account, rather than root,
# but some systems crash on cryptography install
sudo pip3 install west

if [[ ! -d ${NCS_ROOT} ]]; then
  mkdir -p "${NCS_ROOT}"
  pushd "${NCS_ROOT}"
  west init -m https://github.com/nrfconnect/sdk-nrf
else
  pushd "${NCS_ROOT}"
fi

west update
west zephyr-export

pushd nrf
git checkout "${SDK_VERSION}"
west update
popd

# NOTE: You should be able to pip install this for the USER account, rather than root,
# but some systems crash on cryptography install
sudo pip3 install -r zephyr/scripts/requirements.txt
sudo pip3 install -r nrf/scripts/requirements.txt
sudo pip3 install -r bootloader/mcuboot/scripts/requirements.txt

popd # "${NCS_ROOT}"

