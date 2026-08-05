install(
    TARGETS stackless-coroutine_exe
    RUNTIME COMPONENT stackless-coroutine_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
