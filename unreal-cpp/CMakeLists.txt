cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    unreal-cpp
    VERSION 0.1.0
    DESCRIPTION "Experiments with Unreal Engine containers outside UnrealBuildTool"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Unreal Engine Core headers ----

# Unreal Engine is not available through vcpkg (Epic's EULA forbids
# redistribution and the engine builds with UnrealBuildTool, not CMake).
# Instead we compile against the Core headers of a local engine install,
# providing the few missing Core-module symbols in ue_core_stub.cpp.
set(UE_ROOT "/Users/Shared/Epic Games/UE_5.8" CACHE PATH "Unreal Engine install root")

if(NOT EXISTS "${UE_ROOT}/Engine/Source/Runtime/Core/Public/Containers/Array.h")
  message(
      FATAL_ERROR
      "Unreal Engine not found at UE_ROOT=${UE_ROOT}; "
      "set -DUE_ROOT=<engine install> to point at a UE 5.8 installation"
  )
endif()

add_library(ue_core_minimal OBJECT source/ue_core_stub.cpp)

target_include_directories(
    ue_core_minimal SYSTEM PUBLIC
    "${UE_ROOT}/Engine/Source/Runtime/Core/Public"
    "${UE_ROOT}/Engine/Source/Runtime/TraceLog/Public"
    "${UE_ROOT}/Engine/Source/Runtime/AutoRTFM/Public"
)

# Definitions UnrealBuildTool would normally inject
target_compile_definitions(
    ue_core_minimal PUBLIC
    PLATFORM_MAC=1
    PLATFORM_APPLE=1
    UE_BUILD_DEVELOPMENT=1
    WITH_EDITOR=0
    WITH_ENGINE=0
    WITH_UNREAL_DEVELOPER_TOOLS=0
    WITH_PLUGIN_SUPPORT=0
    WITH_SERVER_CODE=1
    WITH_EDITORONLY_DATA=0
    UE_SERVER=0
    UE_MERGED_MODULES=0
    IS_MONOLITHIC=1
    IS_PROGRAM=1
    CORE_API=
    TRACELOG_API=
    UBT_COMPILED_PLATFORM=Mac
)

target_compile_features(ue_core_minimal PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(tarray-demo source/tarray_demo.cpp)
add_executable(unreal-cpp::exe ALIAS tarray-demo)

target_link_libraries(tarray-demo PRIVATE ue_core_minimal)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT unreal-cpp_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of unreal-cpp"
  )
endif()

include(cmake/dev-mode.cmake)
