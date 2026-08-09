# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(raft-rpcTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(raft-rpc_test source/raft-rpc_test.cpp)
target_link_libraries(
    raft-rpc_test PRIVATE
    raft-rpc_lib
    Catch2::Catch2WithMain
)
target_compile_features(raft-rpc_test PRIVATE cxx_std_20)

catch_discover_tests(raft-rpc_test)

# ---- End-of-file commands ----

add_folders(Test)
