#!/bin/bash

set -e
git clone --depth 1 --branch v1.7.13 git://github.com/DaveGamble/cJSON.git
test -d cJSON
