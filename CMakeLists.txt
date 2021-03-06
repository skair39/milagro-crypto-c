cmake_minimum_required (VERSION 2.8)
project (AMCL)

# enable testing
enable_testing ()

# Extract version from the VERSION file
file(STRINGS VERSION VERSION_FILE_CONTENT)
string(REPLACE "." ";" VERSION_FILE_PARTS ${VERSION_FILE_CONTENT})
list(GET VERSION_FILE_PARTS 0 AMCL_VERSION_MAJOR)
list(GET VERSION_FILE_PARTS 1 AMCL_VERSION_MINOR)
list(GET VERSION_FILE_PARTS 2 AMCL_VERSION_PATCH)
set(PROJECT_VERSION "${AMCL_VERSION_MAJOR}.${AMCL_VERSION_MINOR}.${AMCL_VERSION_PATCH}")
message(STATUS "PROJECT_VERSION='${PROJECT_VERSION}'")
message(STATUS "Generator - ${CMAKE_GENERATOR}")

# Add options for build
option (BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option (BUILD_PYTHON "Build Python" OFF)
option (BUILD_GO "Build Golang" OFF)
option (BUILD_MPIN "Build MPIN" ON)
option (BUILD_WCC "Build WCC" ON)
option (BUILD_DOXYGEN "Build Doxygen" ON)
option (USE_PATENTS "Use patents for G1 and G2 multiplication" OFF)
option (USE_ANONYMOUS "Anonymous authentication for M-Pin Full" OFF)

# Allow the developer to select if Dynamic or Static libraries are built
# Set the default LIB_TYPE variable to STATIC
SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
ENDIF (BUILD_SHARED_LIBS)

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
    CACHE STRING "Choose the type of build: Debug Release Coverage ASan Check CheckFull"
    FORCE)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_COMPILER_IS_GNUCC)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Wno-strict-prototypes -Wunused-value -Wcast-align -Wunused-variable -Wundef -Wformat-security")

    if (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
        set(CMAKE_C_FLAGS_ASAN    "-O0 -g3 -fsanitize=address")
    else (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
        message(STATUS "GCC 4.8 required to run address sanitizer - please upgrade your installation")
    endif(GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)

    set(CMAKE_C_FLAGS_RELEASE     "-O2")
    set(CMAKE_C_FLAGS_DEBUG       "-O0 -g3")
    set(CMAKE_C_FLAGS_COVERAGE    "-O0 -g3 --coverage")
    set(CMAKE_C_FLAGS_CHECK       "-O2 -Werror")
    set(CMAKE_C_FLAGS_CHECKFULL   "${CMAKE_C_FLAGS_CHECK} -Wcast-qual")
endif(CMAKE_COMPILER_IS_GNUCC)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "--coverage")
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   set(OS "Mac OS X")
   # MACOSX_RPATH
   cmake_policy(SET CMP0042 NEW)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   # Linux specific code
   # Default install path on Linux
   set(OS "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   # Windows specific code
   set(OS "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Select word length
set(AMCL_CHUNK "64" CACHE STRING "Word length. See ./include/arch.h.in for explanation")
set(Word_Length_Values "16;32;64")
set_property(CACHE AMCL_CHUNK PROPERTY STRINGS ${Word_Length_Values})
message(STATUS "AMCL_CHUNK='${AMCL_CHUNK}'")

# Select Field
set(AMCL_CHOICE "BN254_CX" CACHE STRING "Choice of Field. See ./include/amcl.h.in for explanation")
set(Field_Values "NIST256;C25519;BRAINPOOL;ANSSI;MF254;MS255;MF256;MS256;HIFIVE;GOLDILOCKS;NIST384;C41417;NIST521;BN454;BN646;BN254;BN254_CX;BN254_T;BN254_T2;BLS455")
set_property(CACHE AMCL_CHOICE PROPERTY STRINGS ${Field_Values})
message(STATUS "AMCL_CHOICE='${AMCL_CHOICE}'")

# Select Curve type
set(AMCL_CURVETYPE "WEIERSTRASS" CACHE STRING "Choice of Curve. See ./include/amcl.h.in for explanation")
set(Curve_Values "WEIERSTRASS;EDWARDS;MONTGOMERY")
set_property(CACHE AMCL_CURVETYPE PROPERTY STRINGS ${Curve_Values})
message(STATUS "AMCL_CURVETYPE='${AMCL_CURVETYPE}'")

# Select Finite field support
set(AMCL_FFLEN "8" CACHE STRING "Finite field support. See ./include/amcl.h.in for explanation")
set(ff_len_values "4;8;16")
set_property(CACHE AMCL_FFLEN PROPERTY STRINGS ${ff_Len_values})
message(STATUS "AMCL_FFLEN='${AMCL_FFLEN}'")

# configure arch header file to pass CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/arch.h.in"
  "${PROJECT_BINARY_DIR}/include/arch.h"
  )

# configure amcl header file to pass CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/amcl.h.in"
  "${PROJECT_BINARY_DIR}/include/amcl.h"
  )

# Build libx509
if( ((AMCL_CHOICE STREQUAL "NIST256") AND (AMCL_FFLEN STREQUAL "8")) OR 
    ((AMCL_CHOICE STREQUAL "NIST256") AND (AMCL_FFLEN STREQUAL "16")) OR
    ((AMCL_CHOICE STREQUAL "C25519") AND (AMCL_FFLEN STREQUAL "8") AND (AMCL_CURVETYPE STREQUAL "EDWARDS")) OR
    ((AMCL_CHOICE STREQUAL "NIST384") AND (AMCL_FFLEN STREQUAL "8")) OR
    ((AMCL_CHOICE STREQUAL "NIST521") AND (AMCL_FFLEN STREQUAL "8")) )
  message(STATUS "Build x.509")
  set(BUILD_X509 ON)
endif( ((AMCL_CHOICE STREQUAL "NIST256") AND (AMCL_FFLEN STREQUAL "8")) OR 
    ((AMCL_CHOICE STREQUAL "NIST256") AND (AMCL_FFLEN STREQUAL "16")) OR
    ((AMCL_CHOICE STREQUAL "C25519") AND (AMCL_FFLEN STREQUAL "8") AND (AMCL_CURVETYPE STREQUAL "EDWARDS")) OR
    ((AMCL_CHOICE STREQUAL "NIST384") AND (AMCL_FFLEN STREQUAL "8")) OR
    ((AMCL_CHOICE STREQUAL "NIST521") AND (AMCL_FFLEN STREQUAL "8")) )

if(NOT(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS"))
  message(STATUS "Curve choice prevents MPIN being built")
  set(BUILD_MPIN OFF)
endif(NOT(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS"))

if(NOT(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS"))
  message(STATUS "Curve choice prevents WCC being built")
  set(BUILD_WCC OFF)
endif(NOT(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS"))

# test configuration
if(BUILD_MPIN OR BUILD_WCC)
  set(MPIN_TIME_PERMIT_TESTS 10 CACHE STRING "Number of days in the future to test M-PIN time permits")
  set(MPIN_RANDOM_TESTS 10 CACHE STRING "Number of random M-PIN tests")
  message(STATUS "MPIN_TIME_PERMIT_TESTS=${MPIN_TIME_PERMIT_TESTS}")
  message(STATUS "MPIN_RANDOM_TESTS=${MPIN_RANDOM_TESTS}")
  # Configure test variables
  configure_file (
    "${PROJECT_SOURCE_DIR}/include/config.h.in"
    "${PROJECT_BINARY_DIR}/include/config.h"
  )
endif(BUILD_MPIN OR BUILD_WCC)

# Add subdirectories
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)

# Only build Python for shared libs
if (NOT BUILD_SHARED_LIBS)
  set(BUILD_PYTHON OFF)
endif (NOT BUILD_SHARED_LIBS)

if (BUILD_PYTHON)
  message(STATUS "Build Python CFFI")
  execute_process (COMMAND python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process (COMMAND python -c "from distutils.sysconfig import get_python_lib; from os.path import dirname; print dirname(get_python_lib())" OUTPUT_VARIABLE PYTHON_SITE_LIB OUTPUT_STRIP_TRAILING_WHITESPACE)

  if((AMCL_CHOICE STREQUAL "BN254_CX") AND BUILD_MPIN)
    file(COPY "${PROJECT_SOURCE_DIR}/testVectors/mpin/BN254_CX.json" DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python")
    file(RENAME "${PROJECT_BINARY_DIR}/wrappers/python/BN254_CX.json" "${PROJECT_BINARY_DIR}/wrappers/python/MPINTestVectors.json")
  endif((AMCL_CHOICE STREQUAL "BN254_CX") AND BUILD_MPIN)
  add_subdirectory (wrappers/python)
endif (BUILD_PYTHON)

if (BUILD_GO)
  set(GO_PATH "$ENV{GOPATH}" CACHE STRING "GOPATH")
  message(STATUS "Build Golang: GOPATH='${GO_PATH}'")
  add_subdirectory (wrappers/go)
endif (BUILD_GO)

# Build Documentation
if (BUILD_DOXYGEN)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    add_subdirectory (doc)
  else(DOXYGEN_FOUND)
    message(STATUS "Doxygen not found. Documentation will not be built.")
  endif(DOXYGEN_FOUND)
endif (BUILD_DOXYGEN)

message(STATUS "Install ${CMAKE_INSTALL_PREFIX}")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# Build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME "AMCL")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "support@miracl.com")
set(CPACK_PACKAGE_VENDOR "MIRACL")
if (BUILD_PYTHON)
  set(CPACK_RPM_PACKAGE_REQUIRES "python >= 2.7.0")
endif (BUILD_PYTHON)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # Linux specific code
  set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  # Prevents CPack from generating file conflicts
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}/bin")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}/include")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX}/lib")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${PYTHON_SITE_LIB}")
  list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${PYTHON_SITE_PACKAGES}")
  set(CPACK_GENERATOR "RPM")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # Windows specific code
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "AMCL")
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icon\\\\icon.bmp")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icon\\\\icon.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icon\\\\icon.ico")
  set(CPACK_NSIS_HELP_LINK "http://www.miracl.com")
  set(CPACK_NSIS_URL_INFO_ABOUT "http://www.miracl.com")
  set(CPACK_NSIS_CONTACT "support@miracl.com")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Mac OS X specific code
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

include (CPack)

