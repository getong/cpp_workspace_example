install(
    TARGETS caf-example_exe
    RUNTIME COMPONENT caf-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
