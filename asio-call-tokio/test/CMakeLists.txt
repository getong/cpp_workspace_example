# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(asio-call-tokioTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(asio-call-tokio_test source/asio-call-tokio_test.cpp)
target_link_libraries(
    asio-call-tokio_test PRIVATE
    asio-call-tokio_lib
    Catch2::Catch2WithMain
)
target_compile_features(asio-call-tokio_test PRIVATE cxx_std_20)

catch_discover_tests(asio-call-tokio_test)

# ---- End-of-file commands ----

add_folders(Test)
