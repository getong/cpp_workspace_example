install(
    TARGETS delegate-example_exe
    RUNTIME COMPONENT delegate-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
