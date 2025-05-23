# Required libraries for Demos for linking.
find_library(FIND_FREERTOS freertos)
if (FIND_FREERTOS)
  collect(PROJECT_LIB_DEPS freertos)
endif(FIND_FREERTOS)
