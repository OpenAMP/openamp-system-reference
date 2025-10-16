cmake_minimum_required (VERSION 3.16)

set (APPS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (APPS_SHARE_DIR "${CMAKE_CURRENT_BINARY_DIR}/share")

list (APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms")

include (syscheck)
project (osr_legacy_apps C)

include (CheckIncludeFiles)
include (CheckCSourceCompiles)
include (collect)
include (options)
include (depends)
enable_testing ()

collector_create (APP_COMMON_SOURCES "")
collector_create (APP_LIB_DIRS "")
collector_create (APP_INC_DIRS "")
collector_create (APP_LIB_DEPS "")

collector_create (APP_EXTRA_C_FLAGS "")
collect (APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory (system)
add_subdirectory (machine)

if (WITH_TESTS)
  add_subdirectory (tests)
endif(WITH_TESTS)
if (WITH_EXAMPLES)
add_subdirectory (examples)
endif (WITH_EXAMPLES)
