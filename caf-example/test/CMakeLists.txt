# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(caf-exampleTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(caf-example_test source/caf-example_test.cpp)
target_link_libraries(
    caf-example_test PRIVATE
    caf-example_lib
    Catch2::Catch2WithMain
)
target_compile_features(caf-example_test PRIVATE cxx_std_20)

catch_discover_tests(caf-example_test)

# ---- End-of-file commands ----

add_folders(Test)
