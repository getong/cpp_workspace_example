install(
    TARGETS rust-ffi_exe
    RUNTIME COMPONENT rust-ffi_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
