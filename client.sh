#!/usr/bin/env bash

if [[ ! $1 ]]; then
    echo "Usage: $0 <node number>"
    exit 1
fi

docker run --rm -it \
    --entrypoint /apps/spl-client \
    --network splinterdb-network \
    neilk3/replicated-splinterdb \
    -endpoint replicated-splinterdb-node-$1:10003 "${@:2}"