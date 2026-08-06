# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(unreal-cppTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(unreal-cpp_test source/unreal-cpp_test.cpp)
target_link_libraries(
    unreal-cpp_test PRIVATE
    ue_core_minimal
    Catch2::Catch2WithMain
)
target_compile_features(unreal-cpp_test PRIVATE cxx_std_20)

catch_discover_tests(unreal-cpp_test)

# ---- End-of-file commands ----

add_folders(Test)
