# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(template_interfaceTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(template_interface_test source/template_interface_test.cpp)
target_link_libraries(
    template_interface_test PRIVATE
    template_interface_lib
    Catch2::Catch2WithMain
)
target_compile_features(template_interface_test PRIVATE cxx_std_23)

catch_discover_tests(template_interface_test)

# ---- End-of-file commands ----

add_folders(Test)
