install(
    TARGETS cpp-20-exmaple_exe
    RUNTIME COMPONENT cpp-20-exmaple_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
