cmake_minimum_required(VERSION 3.22.1)

project(hmpc CXX)

option(HMPC_BUILD_TESTING "Build tests for the library" ON)
option(HMPC_TEST_COVERAGE "Add coverage to tests" OFF)
option(HMPC_BUILD_EXTERNAL_EXAMPLES "Build external examples" ON)
option(HMPC_BUILD_EXAMPLES "Build examples" ON)
option(HMPC_TEST_EXAMPLES "Test if examples run" ON)
option(HMPC_TIDY "Add clang-tidy targets" OFF)

option(HMPC_ENABLE_CUDA "Add CUDA support" OFF)
option(HMPC_ENABLE_SESSIONS "Enable sessions" ON)
option(HMPC_ENABLE_SIGNING "Enable signing of messages" ON)
option(HMPC_ENABLE_STATISTICS "Enable statistics, e.g., for networking" ON)

set(HMPC_DEVICE_TARGETS "spir64_x86_64" CACHE STRING "SYCL targets (comma separated; for example: spir64,spir64_x86_64,native_cpu)")

message("HMPC: Build:\n - CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}\n - CMAKE_CONFIGURATION_TYPES: ${CMAKE_CONFIGURATION_TYPES}\n - CMAKE_EXPORT_COMPILE_COMMANDS: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
message("HMPC: Build tests:\n - library tests: ${HMPC_BUILD_TESTING}\n - code coverage for tests: ${HMPC_TEST_COVERAGE}")
message("HMPC: Build examples:\n - any examples: ${HMPC_BUILD_EXAMPLES}\n - external examples: ${HMPC_BUILD_EXTERNAL_EXAMPLES}\n - test examples: ${HMPC_TEST_EXAMPLES}")
message("HMPC: Tidy:\n - adding tidy targets: ${HMPC_TIDY}")
message("HMPC: Features:\n - CUDA: ${HMPC_ENABLE_CUDA}\n - sessions: ${HMPC_ENABLE_SESSIONS}\n - signing: ${HMPC_ENABLE_SIGNING}\n - statistics: ${HMPC_ENABLE_STATISTICS}")
if (HMPC_ENABLE_CUDA)
    set(HMPC_DEVICE_TARGETS "${HMPC_DEVICE_TARGETS},nvptx64-nvidia-cuda")
    set(HMPC_CUDA_ARCH "sm_70" CACHE STRING "CUDA architecture. \"sm_50\" is the lowest supportet value. V100 supports \"sm_70\". A100 supports \"sm_80\".")
    set(HMPC_CUDA_PATH $ENV{CUDA_PATH} CACHE PATH "CUDA toolkit directory")

    message("HMPC: CUDA architecture: ${HMPC_CUDA_ARCH}")
    if (NOT HMPC_CUDA_PATH)
        message(WARNING "HMPC: CUDA path empty")
    else()
        message("HMPC: CUDA path: ${HMPC_CUDA_PATH}")
    endif()
endif()
message("HMPC: Targets: ${HMPC_DEVICE_TARGETS}")

set(CMAKE_CXX_STANDARD 26)

add_library(hmpc INTERFACE)
target_include_directories(hmpc INTERFACE .)
# target_compile_features(hmpc INTERFACE cxx_std_26)
if (HMPC_ENABLE_SESSIONS)
    target_compile_definitions(hmpc INTERFACE
        HMPC_ENABLE_SESSIONS=1
    )
endif()
if (HMPC_ENABLE_SIGNING)
    target_compile_definitions(hmpc INTERFACE
        HMPC_ENABLE_SIGNING=1
    )
endif()
if (HMPC_ENABLE_STATISTICS)
    target_compile_definitions(hmpc INTERFACE
        HMPC_ENABLE_STATISTICS=1
    )
endif()
target_compile_options(hmpc INTERFACE -Wall -Wextra -Wpedantic -Wunknown-pragmas -Werror -fsycl -fsycl-targets=${HMPC_DEVICE_TARGETS} -fconstexpr-steps=999999999)
target_link_options(hmpc INTERFACE -fsycl -fsycl-targets=${HMPC_DEVICE_TARGETS})
if (HMPC_ENABLE_CUDA)
    set(HMPC_CUDA_FLAGS -Xsycl-target-backend=nvptx64-nvidia-cuda --cuda-gpu-arch=${HMPC_CUDA_ARCH} --cuda-path=${HMPC_CUDA_PATH})
    set(HMPC_CUDA_LINK_FLAGS ${HMPC_CUDA_FLAGS} -Xcuda-ptxas --maxrregcount=64)
    target_compile_options(hmpc INTERFACE ${HMPC_CUDA_FLAGS})
    target_link_options(hmpc INTERFACE ${HMPC_CUDA_LINK_FLAGS})
endif()

# List of all targets that have source and could be built
set(HMPC_TARGETS)

if (HMPC_BUILD_TESTING)
    find_package(Catch2 3 REQUIRED)

    include(CTest)
    include(Catch)
endif()

add_subdirectory(hmpc-rs)

if (HMPC_BUILD_TESTING)
    find_package(FMT 10 REQUIRED)

    add_subdirectory(tests)
endif()

if (HMPC_BUILD_EXAMPLES)
    find_package(FMT 10 REQUIRED)

    add_subdirectory(examples)
endif()

add_custom_target(apply-clang-format
    COMMAND git ls-files --exclude-standard | grep -E ".(cpp|hpp)$" | xargs clang-format --verbose -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

add_custom_target(apply-format)
add_dependencies(apply-format apply-clang-format apply-rustfmt)

if (HMPC_TIDY)
    add_custom_target(tidy-make_directory
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/reports/clang-tidy
    )

    set(HMPC_TIDY_TARGETS)
    foreach(TARGET ${HMPC_TARGETS})
        add_custom_target(tidy-${TARGET}
            COMMAND clang-tidy --config-file ${CMAKE_SOURCE_DIR}/.clang-tidy --verify-config && clang-tidy --config-file ${CMAKE_SOURCE_DIR}/.clang-tidy -p ${CMAKE_BINARY_DIR} $<TARGET_PROPERTY:${TARGET},SOURCES> > ${PROJECT_SOURCE_DIR}/reports/clang-tidy/${TARGET}.log
            COMMAND_EXPAND_LISTS
            WORKING_DIRECTORY $<TARGET_PROPERTY:${TARGET},SOURCE_DIR>
        )
        add_dependencies(tidy-${TARGET} tidy-make_directory)
        list(APPEND HMPC_TIDY_TARGETS tidy-${TARGET})
    endforeach()

    add_custom_target(tidy)
    add_dependencies(tidy ${HMPC_TIDY_TARGETS})
endif()
