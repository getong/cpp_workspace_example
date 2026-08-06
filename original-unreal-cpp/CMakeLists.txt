cmake_minimum_required(VERSION 3.14)

project(
    unreal-cpp-demo
    VERSION 0.1.0
    DESCRIPTION "Unreal Engine TArray C++ demo"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES NONE
)

include(cmake/unreal-engine.cmake)

add_unreal_engine_targets(
    PROJECT_FILE "${PROJECT_SOURCE_DIR}/UnrealCppDemo.uproject"
    TARGET UnrealCppDemo
)
