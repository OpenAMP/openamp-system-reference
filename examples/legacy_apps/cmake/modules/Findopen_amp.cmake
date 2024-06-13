# Findopen_amp
# --------
#
# Find open_amp 
#
# Find the native open_amp includes and library this module defines
#
# ::
#
#   OPENAMP_INCLUDE_DIR, where to find metal/sysfs.h, etc.

# FIX ME, CMAKE_FIND_ROOT_PATH doesn't work
# even use the following
# set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
# set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
# set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
find_path(OPENAMP_INCLUDE_DIR NAMES openamp/ PATHS ${CMAKE_FIND_ROOT_PATH})
find_library(OPENAMP_LIB NAMES open_amp PATHS ${CMAKE_FIND_ROOT_PATH})
get_filename_component(OPENAMP_LIB_DIR ${OPENAMP_LIB} DIRECTORY)

# handle the QUIETLY and REQUIRED arguments and set HUGETLBFS_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (open_amp DEFAULT_MSG OPENAMP_LIB OPENAMP_INCLUDE_DIR)

if (OPENAMP_FOUND)
  set (OPENAMP_LIBS ${OPENAMP_LIB})
endif (OPENAMP_FOUND)

mark_as_advanced (OPENAMP_LIB OPENAMP_INCLUDE_DIR OPENAMP_LIB_DIR)
