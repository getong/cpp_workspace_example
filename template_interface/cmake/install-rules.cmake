install(
    TARGETS template_interface_exe
    RUNTIME COMPONENT template_interface_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
