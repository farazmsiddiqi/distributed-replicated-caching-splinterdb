#!/usr/bin/env bash

NODE_COUNT=3
SERVER_IMAGE="neilk3/replicated-splinterdb"
NETWORK="splinterdb-network"

cleanup() {
    echo "Stopping and removing containers..."
    for i in $(seq 1 $NODE_COUNT); do
        docker stop "replicated-splinterdb-node-$i"
        docker rm "replicated-splinterdb-node-$i"
    done
    docker network rm $NETWORK
}

cleanup

set -e
echo -n "Creating Docker network. Hash: "
docker network create $NETWORK

echo -n "Starting replicated-splinterdb node 1. Hash: "
set -e
docker run --rm -d \
    --name "replicated-splinterdb-node-1" \
    --hostname "replicated-splinterdb-node-1" \
    --network $NETWORK \
    $SERVER_IMAGE \
    -serverid 1 \
    -raftport 10001 \
    -joinport 10002 \
    -clientport 10003

sleep 5

for i in $(seq 2 $NODE_COUNT); do
    echo -n "Starting replicated-splinterdb node $i. Hash: "
    set -e
    docker run --rm -d \
        --name "replicated-splinterdb-node-$i" \
        --hostname "replicated-splinterdb-node-$i" \
        --network $NETWORK \
        $SERVER_IMAGE \
        -serverid $i \
        -raftport 10001 \
        -joinport 10002 \
        -clientport 10003 \
        -join_endpoint replicated-splinterdb-node-1:10002
    set +e
done
