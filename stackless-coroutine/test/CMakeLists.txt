# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(stackless-coroutineTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(stackless-coroutine_test source/stackless-coroutine_test.cpp)
target_link_libraries(
    stackless-coroutine_test PRIVATE
    stackless-coroutine_lib
    Catch2::Catch2WithMain
)
target_compile_features(stackless-coroutine_test PRIVATE cxx_std_20)

catch_discover_tests(stackless-coroutine_test)

# ---- End-of-file commands ----

add_folders(Test)
