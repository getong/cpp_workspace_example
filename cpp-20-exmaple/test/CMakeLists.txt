# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(cpp-20-exmapleTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(cpp-20-exmaple_test source/cpp-20-exmaple_test.cpp)
target_link_libraries(
    cpp-20-exmaple_test PRIVATE
    cpp-20-exmaple_lib
    Catch2::Catch2WithMain
)
target_compile_features(cpp-20-exmaple_test PRIVATE cxx_std_20)

catch_discover_tests(cpp-20-exmaple_test)

# ---- End-of-file commands ----

add_folders(Test)
