#!/usr/bin/env bash

if [[ ! $1 ]]; then
    echo "Usage: $0 <cache size (MiB)>"
    exit 1
fi

NODE_COUNT=3
NSERVER_THREADS=10
CACHESIZE=$1
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

CACHEDIR=$PWD/caches/node-1
rm -rf $CACHEDIR
mkdir -p $CACHEDIR

echo -n "Starting replicated-splinterdb node 1. Hash: "
docker run --rm -d \
    --name "replicated-splinterdb-node-1" \
    --hostname "replicated-splinterdb-node-1" \
    --network $NETWORK \
    -v $CACHEDIR:/cachepages \
    $SERVER_IMAGE \
    -serverid 1 \
    -raftport 10001 \
    -joinport 10002 \
    -clientport 10003 \
    -nthreads $NSERVER_THREADS \
    -dbfilesize 2048 \
    -cachesize $CACHESIZE

echo "Mounted directory ($CACHEDIR) on host to /cachepages of container"

sleep 5

for i in $(seq 2 $NODE_COUNT); do
    echo -n "Starting replicated-splinterdb node $i. Hash: "
    set -e

    CACHEDIR=$PWD/caches/node-$i
    rm -rf $CACHEDIR
    mkdir -p $CACHEDIR

    docker run --rm -d \
        --name "replicated-splinterdb-node-$i" \
        --hostname "replicated-splinterdb-node-$i" \
        --network $NETWORK \
        -v $CACHEDIR:/cachepages \
        $SERVER_IMAGE \
        -serverid $i \
        -raftport 10001 \
        -joinport 10002 \
        -clientport 10003 \
        -nthreads $NSERVER_THREADS \
        -seed replicated-splinterdb-node-1:10002 \
        -dbfilesize 2048 \
        -cachesize $CACHESIZE

    echo "Mounted directory ($CACHEDIR) on host to /cachepages of container"

    set +e
done
