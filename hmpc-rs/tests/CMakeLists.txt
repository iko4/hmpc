add_test(
    NAME hmpc-rs
    COMMAND cargo ${HMPC_RS_CARGO_TOOLCHAIN} test ${HMPC_RS_CARGO_ARGS} -- --nocapture
    WORKING_DIRECTORY ${HMPC_RS_WORKING_DIR}
)
set_tests_properties(hmpc-rs
    PROPERTIES
        FIXTURES_REQUIRED networking-files
)

add_test(
    NAME setup-networking
    COMMAND cargo ${HMPC_RS_CARGO_TOOLCHAIN} run ${HMPC_RS_CARGO_ARGS} --bin setup -- --verbose --config tests/mpc.yaml certificate --and-signing-key --force 0 1 2 3 4 10 11 12 13 14 20 21 22 23
    WORKING_DIRECTORY ${HMPC_RS_WORKING_DIR}
)
set_tests_properties(setup-networking
    PROPERTIES
        FIXTURES_SETUP networking-files
)
