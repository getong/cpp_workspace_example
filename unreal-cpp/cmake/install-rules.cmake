install(
    TARGETS tarray-demo
    RUNTIME COMPONENT unreal-cpp_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
