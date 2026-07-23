install(
    TARGETS lambda_example_exe
    RUNTIME COMPONENT lambda_example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
