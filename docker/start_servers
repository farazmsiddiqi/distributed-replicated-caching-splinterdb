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
LOGSDIR=$PWD/logs/node-1
rm -rf $CACHEDIR $LOGSDIR
mkdir -p $CACHEDIR $LOGSDIR

echo -n "Starting replicated-splinterdb node 1. Hash: "
HASH=$(docker run --rm -d \
    --name "replicated-splinterdb-node-1" \
    --hostname "replicated-splinterdb-node-1" \
    --network $NETWORK \
    -v $CACHEDIR:/cachepages \
    -v $LOGSDIR:/.logs \
    $SERVER_IMAGE \
    -serverid 1 \
    -raftport 10001 \
    -joinport 10002 \
    -clientport 10003 \
    -nthreads $NSERVER_THREADS \
    -dbfilesize 2048 \
    -cachesize $CACHESIZE)

echo $HASH
docker logs -f $HASH > $LOGSDIR/server.log 2>&1 &
echo "Mounted directory ($CACHEDIR) on host to /cachepages of container"

sleep 5

for i in $(seq 2 $NODE_COUNT); do
    echo -n "Starting replicated-splinterdb node $i. Hash: "
    set -e

    CACHEDIR=$PWD/caches/node-$i
    LOGSDIR=$PWD/logs/node-$i
    rm -rf $CACHEDIR $LOGSDIR
    mkdir -p $CACHEDIR $LOGSDIR

    HASH=$(docker run --rm -d \
        --name "replicated-splinterdb-node-$i" \
        --hostname "replicated-splinterdb-node-$i" \
        --network $NETWORK \
        -v $CACHEDIR:/cachepages \
        -v $LOGSDIR:/.logs \
        $SERVER_IMAGE \
        -serverid $i \
        -raftport 10001 \
        -joinport 10002 \
        -clientport 10003 \
        -nthreads $NSERVER_THREADS \
        -seed replicated-splinterdb-node-1:10002 \
        -dbfilesize 2048 \
        -cachesize $CACHESIZE)
    
    echo $HASH
    docker logs -f $HASH > $LOGSDIR/server.log 2>&1 &
    echo "Mounted directory ($CACHEDIR) on host to /cachepages of container"
    sleep 1

    set +e
done
