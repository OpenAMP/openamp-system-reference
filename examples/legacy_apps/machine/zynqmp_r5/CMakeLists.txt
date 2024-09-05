collect (APP_COMMON_SOURCES platform_info.c)
collect (APP_COMMON_SOURCES rsc_table.c)
collect (APP_COMMON_SOURCES zynqmp_r5_a53_rproc.c)
collect (APP_COMMON_SOURCES helper.c)

# For now this is unconditionally included.
# Once surrounding tooling accommodates CMake-configure steps for apps
# this will move to be included conditionally based on CMake flags.
collect (APP_COMMON_SOURCES platform_info_versal_net.c)
collect (APP_COMMON_SOURCES platform_info_versal.c)
collect (APP_COMMON_SOURCES platform_info_zynqmp.c)
collect (APP_COMMON_SOURCES debug_info.c)

collect (APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

find_path(XIL_INCLUDE_DIR NAMES xparameters.h PATHS ${CMAKE_FIND_ROOT_PATH})
collect (PROJECT_INC_DIRS "${XIL_INCLUDE_DIR}")

find_library(LIBXIL_LIB NAMES xil PATHS ${CMAKE_FIND_ROOT_PATH})
get_filename_component(LIBXIL_LIB_DIR ${LIBXIL_LIB} DIRECTORY)
collect(PROJECT_LIB_DIRS ${LIBXIL_LIB_DIR})

# libxil provides AMD-Xilinx BSP APIs to access Xilinx hardware.
collect(PROJECT_LIB_DEPS xil)

# C standard library
collect(PROJECT_LIB_DEPS c)

set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_files/linker_remote.ld")
set (_linker_large_text_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_files/linker_large_text.ld")

get_property (HAS_SYSTEM_DT GLOBAL PROPERTY HAS_SYSTEM_DT)
get_property (LINKER_METADATA_FILE GLOBAL PROPERTY LINKER_METADATA_FILE)
get_property (SOC GLOBAL PROPERTY SOC)

set (SOCS ZYNQMP VERSAL VERSAL_NET)
set (STATIC_LINKERS linker_sdt.ld linker_sdt.ld linker_sdt_versal_net.ld)
set (LOPPER_CFG_LINKERS lscript_r5.ld.in lscript_versal_r5.ld.in lscript_versal_net_r5.ld.in)
set (PLAT_SRCS platform_info_zynqmp.c platform_info_versal.c platform_info_versal_net.c)

list(FIND SOCS "${SOC}" soc_index)

list(GET PLAT_SRCS ${soc_index} plat_c)
collect (APP_COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${plat_c}")

if (HAS_SYSTEM_DT)
  # Xilinx-AMD SDT workflow has split up libxil. Below are required libs
  # That were previously in libxil.
  collect(PROJECT_LIB_DEPS xiltimer)
  collect(PROJECT_LIB_DEPS xilstandalone)

  if (EXISTS ${LINKER_METADATA_FILE})
    include(${LINKER_METADATA_FILE})
    list (GET LOPPER_CFG_LINKERS ${soc_index} linker_in)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linker_files/${linker_in} "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
    list(APPEND LINKER_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${linker_in})
    list(APPEND LINKER_FILE ${linker_in})
    set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
    message("Using LINKER_METADATA_FILE: ${LINKER_METADATA_FILE}")
  else()
    list (GET STATIC_LINKERS ${soc_index} static_linker)
    set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_files/${static_linker}")
  endif(EXISTS ${LINKER_METADATA_FILE})

  set (_linker_large_text_script "${_linker_script}")
endif(HAS_SYSTEM_DT)

# If not provided, add default value for resource table
if(NOT DEFINED RSC_TABLE)
  set(RSC_TABLE 0x3ed20000 CACHE STRING "")
endif(NOT DEFINED RSC_TABLE)

set_property (GLOBAL PROPERTY APP_LINKER_OPT "-Wl,--defsym,_rsc_table=${RSC_TABLE} -T\"${_linker_script}\"")
set_property (GLOBAL PROPERTY APP_LINKER_LARGE_TEXT_OPT "-Wl,--defsym,_rsc_table=${RSC_TABLE} -T\"${_linker_large_text_script}\"")
