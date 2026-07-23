# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(lambda_exampleTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(lambda_example_test source/lambda_example_test.cpp)
target_link_libraries(
    lambda_example_test PRIVATE
    lambda_example_lib
    Catch2::Catch2WithMain
)
target_compile_features(lambda_example_test PRIVATE cxx_std_20)

catch_discover_tests(lambda_example_test)

# ---- End-of-file commands ----

add_folders(Test)
