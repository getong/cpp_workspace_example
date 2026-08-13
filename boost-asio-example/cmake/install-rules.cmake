install(
    TARGETS boost-asio-example_exe
    RUNTIME COMPONENT boost-asio-example_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
