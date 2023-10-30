# replicated-splinterdb

## Building from Scratch

```bash
export NTHREADS=10
export CC=clang
export LD=clang
export CXX=clang++
git clone https://github.com/nkaush/replicated-splinterdb.git
cd replicated-splinterdb/
git submodule update --init --recursive
sudo ./setup_server.sh
mkdir build && cd build
cmake .. && make -j $NTHREADS all spl-server
export PATH=`pwd`/apps:$PATH  # OPTIONAL: Add newly built executables to PATH
```

## Development Process

Run `make dev` in a shell to start a container with all the necessary dependencies to build this project. This make rule will also mount the `src/`, `apps/`, and `include/` directories onto the `/work` directory in the container. The `libnuraft` and `libsplinterdb` libraries will be built during the container image build stage.

The container will start in a shell in the `/work/build` directory. Run `./build` to build the source code. You can make chanegs to any files in the `src/`, `apps/`, and `include/` directories and recompile them without exiting and restarting the container. Simply make changes and run `./build` again to recompile.

## TODOs

- [ ] Client cache reset API
- [ ] YAML config parsing (see [yaml-cpp](https://github.com/jbeder/yaml-cpp/wiki/Tutorial))

# Starting:
```
./spl-server -serverid 1 -raftport 10000 -joinport 10001 -clientport 10002

./spl-server -serverid 2 -raftport 10003 -joinport 10004 -clientport 10005 -join_endpoint localhost:10001

./spl-server -serverid 3 -raftport 10006 -joinport 10007 -clientport 10008 -join_endpoint localhost:10001
```