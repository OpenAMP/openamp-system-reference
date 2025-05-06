# For Xilinx-AMD tooling enable control flow path whereby only target app
# is configured to build. In the future we can add options for each
# app but for now enable as a one-shot flow switch
if ("${PROJECT_VENDOR}" STREQUAL "xlnx")
  # Only do check for this property if for Xilinx-AMD platform.
  # Otherwise Cmake configure step can fail trying at property lookup.
  get_property (OPENAMP_APP_NAME GLOBAL PROPERTY OPENAMP_APP_NAME)
  if (NOT OPENAMP_APP_NAME STREQUAL "")
    add_subdirectory (${OPENAMP_APP_NAME})
    return()
  endif()
endif()

option (WITH_LOAD_FW "Include loading firmware example" OFF)

if (MACHINE MATCHES ".*microblaze.*")
  add_subdirectory (echo)
else ()
  add_subdirectory (echo)
  add_subdirectory (nocopy_echo)
  add_subdirectory (rpmsg_sample_echo)
  add_subdirectory (matrix_multiply)
  if (WITH_LOAD_FW)
    add_subdirectory (load_fw)
  endif (WITH_LOAD_FW)

  if (WITH_PROXY_APPS)
    add_subdirectory (rpc_demo)
    add_subdirectory (linux_rpc_demo)
  endif (WITH_PROXY_APPS)
endif (MACHINE MATCHES ".*microblaze.*")
