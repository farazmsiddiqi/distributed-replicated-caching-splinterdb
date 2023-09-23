# replicated-splinterdb

## TODOs

- [ ] implement state machine for splinterdb
    - see [state machine example](third-party/nuraft/examples/calculator/calc_state_machine.hxx)
    - see [log store example](third-party/nuraft/examples/in_memory_log_store.hxx)
    - [ ] state machine implementation
    - [ ] log storage implementation
    - state machine will invoke splinterdb API (see [splinterdb API](third-party/splinterdb/include))
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