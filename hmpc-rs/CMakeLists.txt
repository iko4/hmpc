set(HMPC_RS_CARGO_TOOLCHAIN "+stable" CACHE STRING "Toolchain for running \"cargo build\"")
set(HMPC_RS_RUSTFMT_TOOLCHAIN "+nightly" CACHE STRING "Toolchain for running \"cargo fmt\"")

set(RUST_PROFILE CMake-$<CONFIG>)
set(HMPC_RS_TMP_DIR ${CMAKE_BINARY_DIR}/rust/tmp)
set(HMPC_RS_STAMP_DIR ${CMAKE_BINARY_DIR}/rust/hmpc-rs-stamp)
set(HMPC_RS_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/rust)
set(HMPC_RS_SOURCE_DIR ${CMAKE_BINARY_DIR}/rust/hmpc-rs)
set(HMPC_RS_INSTALL_DIR ${CMAKE_BINARY_DIR}/rust/)
set(HMPC_RS_BUILD_DIR ${CMAKE_BINARY_DIR}/rust/hmpc-rs-build)
set(HMPC_RS_LOG_DIR ${HMPC_RS_STAMP_DIR})
set(HMPC_RS_WORKING_DIR ${CMAKE_CURRENT_LIST_DIR})
set(HMPC_RS_INCLUDE ${CMAKE_BINARY_DIR}/generated/include)
set(HMPC_RS_INCLUDE_BASE hmpc)
set(HMPC_RS_GENERATED_HEADER ${HMPC_RS_INCLUDE}/${HMPC_RS_INCLUDE_BASE}/ffi.hpp)

function(hmpc_rs_set_paths CONFIG LIB)
    set(${LIB} ${HMPC_RS_BUILD_DIR}/CMake-${CONFIG}/${CMAKE_STATIC_LIBRARY_PREFIX}hmpc_rs${CMAKE_STATIC_LIBRARY_SUFFIX} PARENT_SCOPE)
endfunction()

hmpc_rs_set_paths($<CONFIG> HMPC_RS_LIB)

include(ExternalProject)

if (HMPC_ENABLE_SESSIONS)
    list(APPEND HMPC_RS_FEATURES "sessions")
endif()
if (HMPC_ENABLE_SIGNING)
    list(APPEND HMPC_RS_FEATURES "signing")
endif()
if (HMPC_ENABLE_COLLECTIVE_CONSISTENCY)
    list(APPEND HMPC_RS_FEATURES "collective-consistency")
endif()
if (HMPC_ENABLE_STATISTICS)
    list(APPEND HMPC_RS_FEATURES "statistics")
endif()

if (HMPC_RS_FEATURES)
    list(JOIN HMPC_RS_FEATURES " " HMPC_RS_CARGO_ARG_FEATURES)
    set(HMPC_RS_CARGO_ARG_FEATURES
        "--features"
        "${HMPC_RS_CARGO_ARG_FEATURES}"
    )
else()
    set(HMPC_RS_CARGO_ARG_FEATURES)
endif()

set(HMPC_RS_CARGO_ARG_TARGET
    "--target-dir"
    "${HMPC_RS_BUILD_DIR}"
)
set(HMPC_RS_CARGO_ARG_PROFILE
    "--profile"
    "${RUST_PROFILE}"
)

list(APPEND HMPC_RS_CARGO_ARGS ${HMPC_RS_CARGO_ARG_FEATURES} ${HMPC_RS_CARGO_ARG_TARGET} ${HMPC_RS_CARGO_ARG_PROFILE})

ExternalProject_Add(hmpc-rs-cargo
    TMP_DIR ${HMPC_RS_TMP_DIR}
    STAMP_DIR ${HMPC_RS_STAMP_DIR}
    DOWNLOAD_DIR ${HMPC_RS_DOWNLOAD_DIR}
    SOURCE_DIR ${HMPC_RS_SOURCE_DIR}
    BINARY_DIR ${HMPC_RS_WORKING_DIR}
    INSTALL_DIR ${HMPC_RS_INSTALL_DIR}
    LOG_DIR ${HMPC_RS_LOG_DIR}
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo ${HMPC_RS_CARGO_TOOLCHAIN} build ${HMPC_RS_CARGO_ARGS} --lib
    BUILD_BYPRODUCTS
        ${HMPC_RS_LIB}
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
    LOG_BUILD ON
)

add_custom_command(
    OUTPUT ${HMPC_RS_GENERATED_HEADER}
    COMMAND cbindgen --config "${HMPC_RS_WORKING_DIR}/cbindgen.toml" --output "${HMPC_RS_GENERATED_HEADER}"
    COMMAND clang-format --verbose -i --style "file:${CMAKE_SOURCE_DIR}/.clang-format" "${HMPC_RS_GENERATED_HEADER}"
    DEPENDS
        ${HMPC_RS_LIB}
        ${HMPC_RS_WORKING_DIR}/cbindgen.toml
        ${CMAKE_SOURCE_DIR}/.clang-format
    WORKING_DIRECTORY ${HMPC_RS_WORKING_DIR}
)

add_custom_target(hmpc-rs-header
    DEPENDS ${HMPC_RS_GENERATED_HEADER}
)

# install generated header
install(FILES ${HMPC_RS_GENERATED_HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${HMPC_RS_INCLUDE_BASE}
)

message("HMPC-RS:\n - include: ${HMPC_RS_INCLUDE}\n - generated header: ${HMPC_RS_GENERATED_HEADER}\n - build toolchain: ${HMPC_RS_CARGO_TOOLCHAIN}\n - format toolchain: ${HMPC_RS_RUSTFMT_TOOLCHAIN}\n - features: ${HMPC_RS_FEATURES}\n - cargo args: ${HMPC_RS_CARGO_ARGS}")
# directory needs to be present before we use target_include_directories below
file(MAKE_DIRECTORY ${HMPC_RS_INCLUDE})

add_library(hmpc-rs STATIC IMPORTED GLOBAL)
set_target_properties(hmpc-rs PROPERTIES VERSION ${PROJECT_VERSION})
add_dependencies(hmpc-rs hmpc-rs-cargo hmpc-rs-header)
target_include_directories(hmpc-rs INTERFACE
    $<BUILD_INTERFACE:${HMPC_RS_INCLUDE}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    hmpc_rs_set_paths(${CONFIG} HMPC_RS_LIB)
    message("HMPC-RS:${CONFIG}\n - lib: ${HMPC_RS_LIB}")
    string(TOUPPER ${CONFIG} CONFIG)
    set_target_properties(
        hmpc-rs
        PROPERTIES
            IMPORTED_LOCATION_${CONFIG} ${HMPC_RS_LIB}
    )
    # install built library
    get_target_property(LOCATION hmpc-rs LOCATION_${CONFIG})
    install(
        FILES ${LOCATION} DESTINATION ${CMAKE_INSTALL_LIBDIR}
        CONFIGURATIONS ${CONFIG}
    )
endforeach()

if(HMPC_BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_custom_target(apply-rustfmt
    COMMAND cargo ${HMPC_RS_RUSTFMT_TOOLCHAIN} fmt
    WORKING_DIRECTORY ${HMPC_RS_WORKING_DIR}
)

set(HMPC_RS_WORKING_DIR ${HMPC_RS_WORKING_DIR} PARENT_SCOPE)
