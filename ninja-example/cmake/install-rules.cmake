install(
    TARGETS ninja-example_exe
    RUNTIME COMPONENT ninja-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
