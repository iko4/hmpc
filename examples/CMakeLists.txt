add_executable(fma
    fma.cpp
)
target_link_libraries(fma PRIVATE hmpc)

add_executable(ntt
    ntt.cpp
)
target_link_libraries(ntt PRIVATE hmpc)

add_executable(bgv
    bgv.cpp
)
target_link_libraries(bgv PRIVATE hmpc)

set(HMPC_EXAMPLE_TARGETS fma ntt bgv)

if (HMPC_BUILD_EXTERNAL_EXAMPLES)
    add_subdirectory(external)
endif()

# Meta target for all examples
add_custom_target(examples)
add_dependencies(examples ${HMPC_EXAMPLE_TARGETS})

# Test for examples
if (HMPC_BUILD_TESTING AND HMPC_TEST_EXAMPLES)
    foreach(TARGET ${HMPC_EXAMPLE_TARGETS})
        add_test(NAME "Test examples/${TARGET}" COMMAND ${TARGET} 1 1)
    endforeach()
endif()

list(APPEND HMPC_TARGETS ${HMPC_EXAMPLE_TARGETS})
set(HMPC_TARGETS ${HMPC_TARGETS} PARENT_SCOPE)
