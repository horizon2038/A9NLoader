cmake_minimum_required(VERSION 3.16)
project(EDK2Build)
include(ExternalProject)

set(ARCH "x86_64" CACHE STRING "Target architecture for EDK2 build")

# set(LLVMDIR "/usr/local/opt/llvm/bin" CACHE PATH "Directory containing LLVM binaries")
set(CHAINDIR "${CMAKE_SOURCE_DIR}/chain" CACHE PATH "Chain directory for EDK2")
set(A9NLOADERPKG_DIR "${CMAKE_SOURCE_DIR}/a9nloaderPkg" CACHE PATH "Directory containing a9nloaderPkg")
set(TARGET_TXT "${CMAKE_SOURCE_DIR}/target.txt" CACHE FILEPATH "Path to target.txt")
set(CLANG_BIN "/lib/llvm-19/bin/")

set(EDK2_REPO "git@github.com:tianocore/edk2.git")
set(EDK2_DIR "${CHAINDIR}/${ARCH}/edk2")
set(EDK2_CONF_DIR "${EDK2_DIR}/Conf")
set(BASETOOLS_DIR "${EDK2_DIR}/BaseTools")
file(MAKE_DIRECTORY "${CHAINDIR}/${ARCH}")

set(EDK2_BINARY "${EDK2_DIR}/Build/A9NLoader/X64/A9NLoader.efi" CACHE FILEPATH "Path to the built EDK2 binary")

ExternalProject_Add(edk2_project
  GIT_REPOSITORY "git@github.com:tianocore/edk2.git"
  GIT_TAG "edk2-stable202411"
  GIT_SHALLOW TRUE
  SOURCE_DIR "${CHAINDIR}/${ARCH}/edk2"
  BUILD_IN_SOURCE 1
  CONFIGURE_WORKING_DIRECTORY "${EDK2_DIR}"
  BUILD_WORKING_DIRECTORY "${EDK2_DIR}"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND bash -c "set -ex && cd ${EDK2_DIR} && git submodule update --init && export EDK_TOOLS_PATH=$(pwd)/BaseTools && export CLANG_BIN=/lib/llvm-19/bin/ && source ./edksetup.sh BaseTools && cp -r ${A9NLOADERPKG_DIR} a9nloaderPkg && cp -f ${TARGET_TXT} Conf/target.txt && build -n 4"
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
  LOG_UPDATE ON
  LOG_CONFIGURE ON
  LOG_BUILD ON
)
