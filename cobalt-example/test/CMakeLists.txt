# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(cobalt-exampleTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(cobalt-example_test source/cobalt-example_test.cpp)
target_link_libraries(
    cobalt-example_test PRIVATE
    cobalt-example_lib
    Catch2::Catch2WithMain
)
target_compile_features(cobalt-example_test PRIVATE cxx_std_20)

catch_discover_tests(cobalt-example_test)

# ---- End-of-file commands ----

add_folders(Test)
