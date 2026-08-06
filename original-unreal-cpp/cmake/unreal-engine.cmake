include_guard(GLOBAL)

set(
    UNREAL_ENGINE_ROOT
    ""
    CACHE PATH
    "Unreal Engine installation root (the directory containing Engine)"
)

function(add_unreal_engine_targets)
  set(one_value_args PROJECT_FILE TARGET)
  cmake_parse_arguments(UE "" "${one_value_args}" "" ${ARGN})

  if(NOT UE_PROJECT_FILE OR NOT UE_TARGET)
    message(FATAL_ERROR "add_unreal_engine_targets requires PROJECT_FILE and TARGET")
  endif()

  set(engine_root "${UNREAL_ENGINE_ROOT}")

  if(NOT engine_root AND DEFINED ENV{UNREAL_ENGINE_ROOT})
    set(engine_root "$ENV{UNREAL_ENGINE_ROOT}")
  endif()

  if(NOT engine_root AND APPLE)
    file(GLOB unreal_installations LIST_DIRECTORIES true "/Users/Shared/Epic Games/UE_*")
    if(unreal_installations)
      list(SORT unreal_installations)
      list(REVERSE unreal_installations)
      list(GET unreal_installations 0 engine_root)
    endif()
  endif()

  if(NOT engine_root)
    message(STATUS "Unreal Engine not found; set UNREAL_ENGINE_ROOT to enable Unreal targets")
    return()
  endif()

  get_filename_component(engine_root "${engine_root}" ABSOLUTE)
  set(UNREAL_ENGINE_ROOT "${engine_root}" CACHE PATH "Unreal Engine installation root" FORCE)

  if(APPLE)
    set(unreal_platform Mac)
    set(build_script "${engine_root}/Engine/Build/BatchFiles/Mac/Build.sh")
    set(editor_command "${engine_root}/Engine/Binaries/Mac/UnrealEditor-Cmd")
  elseif(WIN32)
    set(unreal_platform Win64)
    set(build_script "${engine_root}/Engine/Build/BatchFiles/Build.bat")
    set(editor_command "${engine_root}/Engine/Binaries/Win64/UnrealEditor-Cmd.exe")
  elseif(UNIX)
    set(unreal_platform Linux)
    set(build_script "${engine_root}/Engine/Build/BatchFiles/Linux/Build.sh")
    set(editor_command "${engine_root}/Engine/Binaries/Linux/UnrealEditor-Cmd")
  else()
    message(STATUS "Unsupported Unreal host platform; Unreal targets are disabled")
    return()
  endif()

  if(NOT EXISTS "${build_script}" OR NOT EXISTS "${editor_command}")
    message(
        WARNING
        "UNREAL_ENGINE_ROOT does not contain the expected build tools: ${engine_root}"
    )
    return()
  endif()

  get_filename_component(project_file "${UE_PROJECT_FILE}" ABSOLUTE)

  add_custom_target(
      unreal-editor-build
      COMMAND
      "${build_script}"
      "${UE_TARGET}Editor"
      "${unreal_platform}"
      Development
      "${project_file}"
      -WaitMutex
      COMMENT "Building ${UE_TARGET}Editor with UnrealBuildTool"
      USES_TERMINAL
      VERBATIM
  )

  add_custom_target(
      unreal-tarray-test
      COMMAND
      "${CMAKE_COMMAND}"
      -E
      env
      UE_SKIP_UBT_SDK_SETUP=1
      "${editor_command}"
      "${project_file}"
      -Unattended
      -NullRHI
      -NoSplash
      -NoSound
      -NoP4
      -NoTraceServer
      "-LogCmds=Global off, LogTArrayDemo Display, LogAutomationCommandLine Display, LogAutomationController Display"
      "-ExecCmds=Automation RunTests UnrealCppDemo.Containers.TArray; Quit"
      "-TestExit=Automation Test Queue Empty"
      -Log
      DEPENDS unreal-editor-build
      COMMENT "Running the Unreal TArray automation test"
      USES_TERMINAL
      VERBATIM
  )

  add_custom_target(
      unreal-compile-commands
      COMMAND
      "${build_script}"
      -Mode=GenerateClangDatabase
      "${UE_TARGET}Editor"
      "${unreal_platform}"
      Development
      "${project_file}"
      "-OutputDir=${CMAKE_SOURCE_DIR}"
      -OutputFilename=compile_commands.json
      "-Include=${CMAKE_SOURCE_DIR}/Source/..."
      -WaitMutex
      COMMENT "Generating compile_commands.json with UnrealBuildTool"
      USES_TERMINAL
      VERBATIM
  )

  message(STATUS "Unreal Engine targets enabled from ${engine_root}")
endfunction()
