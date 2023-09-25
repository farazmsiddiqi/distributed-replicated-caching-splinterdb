# replicated-splinterdb

## Development Process

Run `make dev` in a shell to start a container with all the necessary dependencies to build this project. This make rule will also mount the `src/`, `apps/`, and `include/` directories onto the `/work` directory in the container. The `libnuraft` and `libsplinterdb` libraries will be built during the container image build stage.

The container will start in a shell in the `/work/build` directory. Run `./build` to build the source code. You can make chanegs to any files in the `src/`, `apps/`, and `include/` directories and recompile them without exiting and restarting the container. Simply make changes and run `./build` again to recompile.

You will need to restart the container if you make changes to the `libnuraft` and/or `libsplinterdb` libraries/source code.

## TODOs

- [ ] implement state machine for splinterdb
    - see [state machine example](third-party/nuraft/examples/calculator/calc_state_machine.hxx)
    - see [log store example](third-party/nuraft/examples/in_memory_log_store.hxx)
    - [ ] state machine implementation
    - [ ] log storage implementation
    - state machine and log will individually will invoke splinterdb API on different DB instances (see [splinterdb API](third-party/splinterdb/include))
- [ ] create a API for server
    - Invokes the state machine API
    - [ ] read
    - [ ] update
    - [ ] insert
    - [ ] delete 
    - [ ] dump cache (no-op for now)
- [ ] create a client API
    - [ ] read
    - [ ] update
    - [ ] insert
    - [ ] delete 
    - [ ] dump cache
- [ ] [LATER] YCSB binding to client API
- [ ] [LATER] Examine cache and dump to file/stderr/stdout