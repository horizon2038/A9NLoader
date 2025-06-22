cmake_minimum_required(VERSION 3.16)
project(EDK2Build)
include(ExternalProject)

set(ARCH "x86_64" CACHE STRING "Target architecture for EDK2 build")

set(CHAINDIR "${CMAKE_SOURCE_DIR}/chain" CACHE PATH "Chain directory for EDK2")
set(A9NLOADERPKG_DIR "${CMAKE_SOURCE_DIR}/a9nloaderPkg" CACHE PATH "Directory containing a9nloaderPkg")
set(TARGET_TXT "${CMAKE_SOURCE_DIR}/target.txt" CACHE FILEPATH "Path to target.txt")

find_program(LLVM_CONFIG_EXECUTABLE llvm-config)
if(LLVM_CONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir
        OUTPUT_VARIABLE LLVM_BINDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CLANG_BIN "${LLVM_BINDIR}/")
else()
    message(FATAL_ERROR "llvm-config is not found. Please install it or create an alias as llvm-config if it has a version-specific name.")
endif()

set(EDK2_DIR "${CHAINDIR}/${ARCH}/edk2")
set(EDK2_CONF_DIR "${EDK2_DIR}/Conf")
set(BASETOOLS_DIR "${EDK2_DIR}/BaseTools")
file(MAKE_DIRECTORY "${CHAINDIR}/${ARCH}")

set(A9NLOADER_EXECUTABLE_DIR "${EDK2_DIR}/build/a9nloaderPkg/X64/RELEASE_CLANGPDB/X64")
set(A9NLOADER_EXECUTABLE "a9nloader.efi")

ExternalProject_Add(edk2_project
  GIT_REPOSITORY "git@github.com:tianocore/edk2.git"
  GIT_TAG "edk2-stable202411"
  GIT_SHALLOW TRUE
  SOURCE_DIR "${CHAINDIR}/${ARCH}/edk2"
  BUILD_IN_SOURCE 1
  CONFIGURE_WORKING_DIRECTORY "${EDK2_DIR}"
  BUILD_WORKING_DIRECTORY "${EDK2_DIR}"
  CONFIGURE_COMMAND bash -c "set -ex && cd ${EDK2_DIR} && git submodule update --init && source ./edksetup.sh && make -C BaseTools && ${CMAKE_COMMAND} -E create_symlink ${A9NLOADERPKG_DIR} a9nloaderPkg && ${CMAKE_COMMAND} -E create_symlink ${TARGET_TXT} Conf/target.txt"
  BUILD_COMMAND bash -c "set -ex && cd ${EDK2_DIR} && export EDK_TOOLS_PATH=${EDK2_DIR}/BaseTools && export CLANG_BIN=${CLANG_BIN} && source ./edksetup.sh BaseTools && build -n 4"
  BUILD_BYPRODUCTS "${A9NLOADER_EXECUTABLE_DIR}/${A9NLOADER_EXECUTABLE}"
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${A9NLOADER_EXECUTABLE_DIR}/${A9NLOADER_EXECUTABLE}" "${PROJECT_BINARY_DIR}/"
  # LOG_DOWNLOAD ON
  # LOG_UPDATE ON
  # LOG_CONFIGURE ON
  # LOG_BUILD ON
)

# cp -r ${A9NLOADERPKG_DIR} a9nloaderPkg && cp -f ${TARGET_TXT} Conf/target.txt
