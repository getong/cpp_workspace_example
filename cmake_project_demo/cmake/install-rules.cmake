install(
    TARGETS cmake_project_demo_exe
    RUNTIME COMPONENT cmake_project_demo_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
