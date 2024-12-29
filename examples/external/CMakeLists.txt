find_package(OpenMP 5)
if (NOT ${OpenMP_FOUND})
    set(OpenMP_CXX_FLAGS -fopenmp)
    set(OpenMP_CXX_LIB_NAMES libomp)
    set(OpenMP_libomp_LIBRARY libiomp5.so)
    find_package(OpenMP 5 REQUIRED)
endif()

include(FetchContent)

FetchContent_Declare(mpspdz-repository
    GIT_REPOSITORY https://github.com/data61/MP-SPDZ
    GIT_TAG 2813c0ef0fa9ca48ced3f70b7081c071f0bd28fd
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(mpspdz-repository)

add_library(mpspdz STATIC
    ${mpspdz-repository_SOURCE_DIR}/Math/bigint.cpp
    ${mpspdz-repository_SOURCE_DIR}/Math/gf2n.cpp
    ${mpspdz-repository_SOURCE_DIR}/Math/gf2nlong.cpp
    ${mpspdz-repository_SOURCE_DIR}/Math/gfpvar.cpp
    ${mpspdz-repository_SOURCE_DIR}/Math/Setup.cpp
    ${mpspdz-repository_SOURCE_DIR}/Math/Zp_Data.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Ciphertext.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/DiscreteGauss.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/FFT.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/FFT_Data.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/FHE_Keys.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/FHE_Params.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/P2Data.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Plaintext.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Random_Coins.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Ring.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Ring_Element.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Rq_Element.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Subroutines.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/Matrix.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/NoiseBounds.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHE/NTL-Subs.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/DataSetup.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/DistKeyGen.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/EncCommit.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/FHE-Subroutines.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/PairwiseSetup.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/Proof.cpp
    ${mpspdz-repository_SOURCE_DIR}/FHEOffline/TemiSetup.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/Player.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/Server.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/ServerSocket.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/Sender.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/sockets.cpp
    ${mpspdz-repository_SOURCE_DIR}/Networking/Receiver.cpp
    ${mpspdz-repository_SOURCE_DIR}/Processor/BaseMachine.cpp
    ${mpspdz-repository_SOURCE_DIR}/Processor/DataPositions.cpp
    ${mpspdz-repository_SOURCE_DIR}/Processor/OnlineOptions.cpp
    ${mpspdz-repository_SOURCE_DIR}/Processor/ThreadQueue.cpp
    ${mpspdz-repository_SOURCE_DIR}/Processor/ThreadQueues.cpp
    ${mpspdz-repository_SOURCE_DIR}/Protocols/CowGearOptions.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/aes.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/benchmarking.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/BitVector.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Commit.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Coordinator.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Exceptions.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Hash.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Lock.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/mkpath.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/names.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/NetworkOptions.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/octetStream.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/random.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Signal.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/Subroutines.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/time-func.cpp
    ${mpspdz-repository_SOURCE_DIR}/Tools/TimerWithComm.cpp
)
target_include_directories(mpspdz PUBLIC ${mpspdz-repository_SOURCE_DIR})
target_compile_definitions(mpspdz PUBLIC
    MAX_MOD_SZ=3
)
target_link_options(mpspdz PUBLIC -lgmp -lgmpxx -lcrypto -lsodium -lssl -lboost_system)
add_library(MP-SPDZ ALIAS mpspdz)

add_executable(mpspdz-fma
    mpspdz-fma.cpp
)
target_compile_features(mpspdz-fma PRIVATE cxx_std_23)
target_compile_options(mpspdz-fma PRIVATE ${OpenMP_CXX_FLAGS})
target_link_libraries(mpspdz-fma PRIVATE MP-SPDZ fmt::fmt OpenMP::OpenMP_CXX)
target_link_options(mpspdz-fma PRIVATE ${OpenMP_CXX_FLAGS})

add_executable(mpspdz-ntt
    mpspdz-ntt.cpp
)
target_compile_features(mpspdz-ntt PRIVATE cxx_std_17)
target_compile_options(mpspdz-ntt PRIVATE ${OpenMP_CXX_FLAGS})
target_link_libraries(mpspdz-ntt PRIVATE MP-SPDZ fmt::fmt OpenMP::OpenMP_CXX)
target_link_options(mpspdz-ntt PRIVATE ${OpenMP_CXX_FLAGS})

add_executable(mpspdz-bgv
    mpspdz-bgv.cpp
)
target_compile_features(mpspdz-bgv PRIVATE cxx_std_17)
target_compile_options(mpspdz-bgv PRIVATE ${OpenMP_CXX_FLAGS})
target_link_libraries(mpspdz-bgv PRIVATE MP-SPDZ fmt::fmt OpenMP::OpenMP_CXX)
target_link_options(mpspdz-bgv PRIVATE ${OpenMP_CXX_FLAGS})

list(APPEND HMPC_EXAMPLE_TARGETS mpspdz-fma mpspdz-ntt mpspdz-bgv)

add_custom_target(fma-comparison-make_directory
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/docs/examples/fma
    VERBATIM
)
add_custom_target(bgv-comparison-make_directory
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/docs/examples/bgv
    VERBATIM
)
if (HMPC_ENABLE_CUDA)
    add_custom_target(fma-comparison
        DEPENDS fma-comparison-make_directory mpspdz-fma fma
        COMMAND python3 ${PROJECT_SOURCE_DIR}/examples/fma.py 8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728 1,2,4,0 $<TARGET_FILE:fma> $<TARGET_FILE:mpspdz-fma> --time_per_item --log_log=2 "--legend=('ours','MP-SPDZ')" --repeats=11
        COMMAND python3 ${PROJECT_SOURCE_DIR}/examples/fma.py 8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728 -1 $<TARGET_FILE:fma> --time_per_item --log_log=2 "--legend=('ours (GPU)',)" --plot="comparison-gpu.pdf" --data="comparison-gpu.json" --repeats=11
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/examples/fma
        VERBATIM
    )
else()
    add_custom_target(fma-comparison
        DEPENDS fma-comparison-make_directory mpspdz-fma fma
        COMMAND python3 ${PROJECT_SOURCE_DIR}/examples/fma.py 8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728 1,2,4,0 $<TARGET_FILE:fma> $<TARGET_FILE:mpspdz-fma> --time_per_item --log_log=2 "--legend=('ours','MP-SPDZ')" --repeats=11
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/examples/fma
        VERBATIM
    )
endif()
add_custom_target(bgv-comparison
    DEPENDS bgv-comparison-make_directory mpspdz-bgv bgv
    COMMAND python3 ${PROJECT_SOURCE_DIR}/examples/fma.py 100,200,300,400,500,750,1000,1500,2000,2500,3000,4000 1,0 $<TARGET_FILE:bgv> $<TARGET_FILE:mpspdz-bgv> --time_per_item --log_log=2 "--legend=('ours','MP-SPDZ')" --repeats=11
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/examples/bgv
    VERBATIM
)

set(HMPC_EXAMPLE_TARGETS ${HMPC_EXAMPLE_TARGETS} PARENT_SCOPE)
