install(
    TARGETS cast-example_exe
    RUNTIME COMPONENT cast-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
