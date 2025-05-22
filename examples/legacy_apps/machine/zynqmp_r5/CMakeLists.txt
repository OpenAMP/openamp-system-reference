collect (APP_COMMON_SOURCES platform_info.c)
collect (APP_COMMON_SOURCES rsc_table.c)
collect (APP_COMMON_SOURCES zynqmp_r5_a53_rproc.c)
collect (APP_COMMON_SOURCES helper.c)
collect (APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM}/gic_init.c")
  collect (APP_COMMON_SOURCES ${PROJECT_SYSTEM}/gic_init.c)
endif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM}/gic_init.c")

# If not provided, add default value for resource table
if(NOT DEFINED ${RSC_TABLE})
  set(RSC_TABLE 0x3ed20000 CACHE STRING "")
endif(NOT DEFINED ${RSC_TABLE})

set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_remote.ld")
set_property (GLOBAL PROPERTY APP_LINKER_OPT "-Wl,--defsym,_rsc_table=${RSC_TABLE} -T\"${_linker_script}\"")
set (_linker_large_text_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_large_text.ld")
set_property (GLOBAL PROPERTY APP_LINKER_LARGE_TEXT_OPT "-Wl,--defsym,_rsc_table=${RSC_TABLE} -T\"${_linker_large_text_script}\"")
