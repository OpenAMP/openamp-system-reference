
set (_cflags "${CMAKE_C_FLAGS} ${APP_EXTRA_C_FLAGS} -fdata-sections -ffunction-sections")
set (_fw_dir "${APPS_SHARE_DIR}")

collector_list (_list PROJECT_INC_DIRS)
collector_list (_app_list APP_INC_DIRS)
include_directories (${_list} ${_app_list} ${CMAKE_CURRENT_SOURCE_DIR})

collector_list (_list PROJECT_LIB_DIRS)
collector_list (_app_list APP_LIB_DIRS)
link_directories (${_list} ${_app_list})

get_property (_linker_opt GLOBAL PROPERTY APP_LINKER_OPT)
collector_list (_deps PROJECT_LIB_DEPS)

# Enable user to pass in extra linker flags
if (DEFINED DEMO_LINK_FLAGS)
  set (_deps "${_deps} ${DEMO_LINK_FLAGS}")
endif(DEFINED DEMO_LINK_FLAGS)

set (OPENAMP_LIB open_amp)

foreach (_app rpmsg-echo-ping rpmsg-echo)
  collector_list (_sources APP_COMMON_SOURCES)
  if (${_app} STREQUAL "rpmsg-echo-ping")
    list (APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/rpmsg-ping.c")
  elseif (${_app} STREQUAL "rpmsg-echo")
    list (APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/rpmsg-echo.c")
    if (${PROJECT_SYSTEM} STREQUAL "linux")
      list (APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/generic/main.c")
    else()
      list (APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM}/main.c")
    endif(${PROJECT_SYSTEM} STREQUAL "linux")
  endif (${_app} STREQUAL "rpmsg-echo-ping")

  if (WITH_SHARED_LIB)
    add_executable (${_app}-shared ${_sources})
    target_compile_options (${_app}-shared PRIVATE "-fPIC")
    target_link_libraries (${_app}-shared ${OPENAMP_LIB} ${_deps})
    install (TARGETS ${_app}-shared RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif (WITH_SHARED_LIB)

  if (WITH_STATIC_LIB)
    if (${PROJECT_SYSTEM} STREQUAL "linux")
      add_executable (${_app}-static ${_sources})
      target_compile_options (${_app}-static PRIVATE "-static")
      target_link_libraries (${_app}-static ${OPENAMP_LIB} ${_deps})
      install (TARGETS ${_app}-static RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    else (${PROJECT_SYSTEM})
      add_executable (${_app}.out ${_sources})
      set_source_files_properties(${_sources} PROPERTIES COMPILE_FLAGS "${_cflags}")

      target_link_libraries(${_app}.out -Wl,-Map=${_app}.map -Wl,--gc-sections ${_linker_opt} -Wl,--start-group -static ${OPENAMP_LIB} ${_deps} -Wl,--end-group)

      install (TARGETS ${_app}.out RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif (${PROJECT_SYSTEM} STREQUAL "linux" )
  endif (WITH_STATIC_LIB)
endforeach(_app)
