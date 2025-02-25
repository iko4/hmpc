add_executable(host-tests
    signedness.cpp
    detail/utility.cpp
    detail/type_id.cpp
    detail/type_set.cpp
    detail/type_map.cpp
    core/mdsize.cpp
    core/uint.cpp
    ints/uint.cpp
    ints/bigint.cpp
    ints/literals.cpp
    ints/mod.cpp
    ints/num/theory/root_of_unity.cpp
    expr/cache.cpp
    shape.cpp
    index.cpp
    crypto/chacha.cpp
    crypto/cipher.cpp
    random/binomial.cpp
    random/uniform.cpp
    typing/reference.cpp
)
target_include_directories(host-tests PRIVATE .)
target_link_libraries(host-tests PRIVATE hmpc Catch2::Catch2WithMain fmt::fmt)
target_compile_definitions(host-tests PRIVATE HMPC_TESTING=1)
catch_discover_tests(host-tests)

add_executable(device-tests
    ints/poly.cpp
    ints/poly_mod.cpp
    comp/queue.cpp
    expr/bit_monomial.cpp
    expr/crypto/cipher.cpp
    expr/crypto/lhe/enc.cpp
    expr/random/number_generator.cpp
    expr/reduce.cpp
    expr/matrix_product.cpp
    expr/matrix_vector_product.cpp
)
target_include_directories(device-tests PRIVATE .)
target_link_libraries(device-tests PRIVATE hmpc Catch2::Catch2WithMain fmt::fmt)
target_compile_definitions(device-tests PRIVATE HMPC_TESTING=1 HMPC_ASSERT_LEVEL=1)
catch_discover_tests(device-tests)

add_executable(ffi-tests
    net/ffi.cpp
    net/queue.cpp
)
target_include_directories(ffi-tests PRIVATE .)
target_link_libraries(ffi-tests PRIVATE hmpc hmpc-rs Catch2::Catch2WithMain fmt::fmt)
target_compile_definitions(ffi-tests PRIVATE HMPC_TESTING=1)
catch_discover_tests(ffi-tests
    WORKING_DIRECTORY ${HMPC_RS_WORKING_DIR}
    PROPERTIES
        FIXTURES_REQUIRED networking-files
)

set(HMPC_TEST_TARGETS host-tests device-tests ffi-tests)

if (HMPC_TEST_COVERAGE)
    # Generate new target that contains all tests.
    # Reason:
    # Above, we generate multiple test binaries to avoid ODR violations with different HMPC_ASSERT_LEVEL defines.
    # llvm-conv assumes that the ODR applies, so we compile all tests together with the same defines.
    add_executable(tests-with-coverage
        $<TARGET_PROPERTY:host-tests,SOURCES>
        $<TARGET_PROPERTY:device-tests,SOURCES>
        $<TARGET_PROPERTY:ffi-tests,SOURCES>
    )
    target_include_directories(tests-with-coverage PRIVATE .)
    target_link_libraries(tests-with-coverage PRIVATE hmpc hmpc-rs Catch2::Catch2WithMain fmt::fmt)
    target_compile_options(tests-with-coverage PRIVATE -Wno-error=option-ignored -fprofile-instr-generate -fcoverage-mapping)
    target_compile_definitions(tests-with-coverage PRIVATE HMPC_TESTING=1 HMPC_ASSERT_LEVEL=1)
    target_link_options(tests-with-coverage PRIVATE -fprofile-instr-generate)

    set(HMPC_COVERAGE_FILE_RAW ${CMAKE_BINARY_DIR}/hmpc.profraw)
    set(HMPC_COVERAGE_FILE_PROCESSED ${CMAKE_BINARY_DIR}/hmpc.profdata)

    add_custom_target(test-coverage
        DEPENDS tests-with-coverage
        COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${HMPC_COVERAGE_FILE_RAW} $<TARGET_FILE:tests-with-coverage>
        COMMAND llvm-profdata merge -o ${HMPC_COVERAGE_FILE_PROCESSED} ${HMPC_COVERAGE_FILE_RAW}
        COMMAND llvm-cov show --instr-profile=${HMPC_COVERAGE_FILE_PROCESSED} --output-dir=${PROJECT_SOURCE_DIR}/reports/coverage -ignore-filename-regex=/tmp/* --show-instantiations=false --show-branches=count -format=html $<TARGET_FILE:tests-with-coverage>
    )

    list(APPEND HMPC_TEST_TARGETS tests-with-coverage)
endif()

# Meta target for all tests
add_custom_target(tests)
add_dependencies(tests ${HMPC_TEST_TARGETS})

list(APPEND HMPC_TARGETS ${HMPC_TEST_TARGETS})
set(HMPC_TARGETS ${HMPC_TARGETS} PARENT_SCOPE)
