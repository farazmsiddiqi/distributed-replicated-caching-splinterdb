# replicated-splinterdb

## TODOs

- [ ] implement state machine for splinterdb
    - see [state machine example](third-party/nuraft/examples/calculator/calc_state_machine.hxx)
    - see [log store example](third-party/nuraft/examples/in_memory_log_store.hxx)
    - [ ] state machine implementation
    - [ ] log storage implementation
- [ ] create a API for server
    - Invokes the state machine API
    - [ ] read
    - [ ] update
    - [ ] insert
    - [ ] delete 
- [ ] create a client API
    - [ ] read
    - [ ] update
    - [ ] insert
    - [ ] delete 
- [] [LATER] YCSB binding to client API