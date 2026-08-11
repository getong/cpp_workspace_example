install(
    TARGETS cobalt-example_exe
    RUNTIME COMPONENT cobalt-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
