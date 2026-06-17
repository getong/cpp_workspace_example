# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(cmake_project_demoTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(cmake_project_demo_test source/cmake_project_demo_test.cpp)
target_link_libraries(
    cmake_project_demo_test PRIVATE
    cmake_project_demo_lib
    Catch2::Catch2WithMain
)
target_compile_features(cmake_project_demo_test PRIVATE cxx_std_20)

catch_discover_tests(cmake_project_demo_test)

# ---- End-of-file commands ----

add_folders(Test)
