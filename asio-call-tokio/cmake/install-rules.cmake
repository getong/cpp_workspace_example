install(
    TARGETS asio-call-tokio_exe
    RUNTIME COMPONENT asio-call-tokio_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
