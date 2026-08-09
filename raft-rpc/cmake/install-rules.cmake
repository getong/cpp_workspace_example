install(
    TARGETS raft-rpc_exe
    RUNTIME COMPONENT raft-rpc_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
