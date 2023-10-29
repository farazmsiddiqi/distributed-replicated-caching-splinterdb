#!/usr/bin/env bash

set -e 

apt-get update -y
apt-get install -y cmake make clang wget curl git \
    zlib1g-dev libgflags-dev libaio-dev libconfig-dev libxxhash-dev \
    software-properties-common shellcheck yamllint