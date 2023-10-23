#!/usr/bin/env bash

if [[ ! $1 ]]; then
    echo "Usage: $0 <server id> [-d] [args...]"
    exit 1
fi

set -e

FLAGS=""
ARGS="${@:2}"
if [[ "$2" == "-d" ]]
then
    FLAGS=$2
    ARGS="${@:3}"
fi

docker run --rm $FLAGS \
    --name "replicated-splinterdb-node-$1" \
    --hostname "replicated-splinterdb-node-$1" \
    --network splinterdb-network \
    neilk3/replicated-splinterdb \
    -serverid $1 \
    -raftport 10001 \
    -joinport 10002 \
    -clientport 10003 \
    $ARGS