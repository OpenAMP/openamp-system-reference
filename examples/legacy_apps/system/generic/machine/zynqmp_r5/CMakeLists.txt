include(CheckSymbolExists)

collect (APP_COMMON_SOURCES helper.c)

set (_linker_large_text_script "${CMAKE_CURRENT_SOURCE_DIR}/linker_large_text.ld")

get_property (HAS_SYSTEM_DT GLOBAL PROPERTY HAS_SYSTEM_DT)
get_property (LINKER_METADATA_FILE GLOBAL PROPERTY LINKER_METADATA_FILE)
get_property (SOC GLOBAL PROPERTY SOC)

set (SOCS ZYNQMP VERSAL VERSAL_NET)
set (STATIC_LINKERS linker_sdt.ld linker_sdt.ld linker_sdt_versal_net.ld)
set (LOPPER_CFG_LINKERS lscript_r5.ld.in lscript_versal_r5.ld.in lscript_versal_net_r5.ld.in)

list(FIND SOCS ${SOC} soc_index)
if (HAS_SYSTEM_DT)
  if (EXISTS ${LINKER_METADATA_FILE})
    include(${LINKER_METADATA_FILE})
    list (GET LOPPER_CFG_LINKERS ${soc_index} linker_in)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linker_files/${linker_in} "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
    list(APPEND LINKER_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${linker_in})
    list(APPEND LINKER_FILE ${linker_in})
    set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
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

find_path(XIL_INCLUDE_DIR NAMES xparameters.h PATHS ${CMAKE_FIND_ROOT_PATH})
collect (PROJECT_INC_DIRS "${XIL_INCLUDE_DIR}")

find_library(LIBXIL_LIB NAMES xil PATHS ${CMAKE_FIND_ROOT_PATH})
get_filename_component(LIBXIL_LIB_DIR ${LIBXIL_LIB} DIRECTORY)
collect(PROJECT_LIB_DIRS ${LIBXIL_LIB_DIR})

# check PM API for certain headers and set client version based
CHECK_SYMBOL_EXISTS(XPAR_XILPM_ENABLED "xparameters.h" PM_FOUND)
if(PM_FOUND)
  find_library(HAS_PM_LIB NAMES xilpm PATHS ${CMAKE_FIND_ROOT_PATH})
  collect (PROJECT_LIB_DEPS xilpm)
  CHECK_SYMBOL_EXISTS(versal "xparameters.h" VERSION_2_PM_CLIENT)
  if (VERSION_2_PM_CLIENT)
    collect (APP_EXTRA_C_FLAGS "  -DVERSION_2_PM_CLIENT ")
  else()
    collect (APP_EXTRA_C_FLAGS "  -DVERSION_1_PM_CLIENT ")
  endif(VERSION_2_PM_CLIENT)
endif(PM_FOUND)

collect(PROJECT_LIB_DEPS xil)
collect(PROJECT_LIB_DEPS c)
collect(PROJECT_LIB_DEPS m)
find_library(XILMEM_LIB NAMES xilmem PATHS ${CMAKE_FIND_ROOT_PATH})
if(XILMEM_LIB)
  collect (PROJECT_LIB_DEPS xilmem)
endif(XILMEM_LIB)
