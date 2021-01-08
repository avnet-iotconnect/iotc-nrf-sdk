#!/bin/bash
set -e

this_dir=$(dirname "${0}")
source "${this_dir}"/env.sh

source "${NCS_ROOT}"/zephyr/zephyr-env.sh

if [[ -z $ZEPHYR_BASE ]]; then
  ZEPHYR_BASE=$1
fi

if [[ -z $ZEPHYR_BASE ]]; then
  echo 'Need to export "ZEPHYR_BASE" or pass it as first argument to '$0
  exit -1
fi

if [[ ! -f $ZEPHYR_BASE/lib/libc/newlib/libc-hooks.c ]]; then
  echo 'ZEPHYR_BASE does not seem to be correct, or SDK 1.3.0 is not used'
  echo 'Please make sure to export "ZEPHYR_BASE" or pass it as first argument to '$0
  exit -2
fi

sed -i 's#^int _gettimeofday\(.*\)#__weak int _gettimeofday\1#g' $ZEPHYR_BASE/lib/libc/newlib/libc-hooks.c
