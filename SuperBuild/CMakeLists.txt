cmake_minimum_required(VERSION 2.8.3)


project(OTB-SuperBuild)

# Path to additional CMake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/CMake
    ${CMAKE_MODULE_PATH})

include(ExternalProject)
include(CTest)
include(SuperBuild_Macro)

# General options
option(ENABLE_QT4 " Use QT4 in the SuperBuild" ON)
option(ENABLE_OTB_LARGE_INPUTS " Use the OTB-LargeInputs for testing" OFF)
option(BUILD_ICE "Clone and build ICE" OFF)
option(BUILD_MONTEVERDI2 "Clone and build Monteverdi2" OFF)
option(BUILD_MONTEVERDI "Clone and build Monteverdi" OFF)

# check if superbuild is in OTB source or in stand-alone mode
set(SUPERBUILD_IN_OTB_TREE OFF)
if(EXISTS "${CMAKE_SOURCE_DIR}/../CMakeLists.txt")
  file(STRINGS "${CMAKE_SOURCE_DIR}/../CMakeLists.txt" _CONTENT_CMAKELIST
    REGEX "project\(OTB\)")
  if("${_CONTENT_CMAKELIST}")
    set(SUPERBUILD_IN_OTB_TREE ON)
  endif()
  
endif()


set(SB_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

set(SB_BUILD_SHARED_LIBS ON)

if(WIN32)
  if(MSVC)
    set(SB_CMAKE_COMMAND ${CMAKE_COMMAND})
     set(SB_BUILD_SHARED_LIBS OFF)
  endif()      
else(UNIX)
  if(APPLE)
    set(LDLIBVAR DYLD_LIBRARY_PATH)
    set(SB_ENV_CONFIGURE_CMD)
    set(SB_CMAKE_COMMAND ${CMAKE_COMMAND})
  else()
    set(LDLIBVAR LD_LIBRARY_PATH)
    set(SB_ENV_CONFIGURE_CMD env ${LDLIBVAR}=${SB_INSTALL_PREFIX}/lib)
    set(SB_CMAKE_COMMAND env ${LDLIBVAR}=${CMAKE_INSTALL_PREFIX}/lib  ${CMAKE_COMMAND})
  endif()
endif()
# Call external projects

#TODO: enforce check from ITK on directory length
# Setup third parties
SETUP_SYSTEM_LIBRARY(PROJECT ZLIB DEFAULT ON)
SETUP_SYSTEM_LIBRARY(PROJECT JPEG DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT EXPAT DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT PNG DEFAULT ON)
SETUP_SYSTEM_LIBRARY(PROJECT TIFF DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT PROJ DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT GEOTIFF DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT SQLITE DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT GEOS DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT OPENJPEG DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT BOOST DEFAULT ON)
SETUP_SYSTEM_LIBRARY(PROJECT LIBKML DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT CURL DEFAULT ON)
SETUP_SYSTEM_LIBRARY(PROJECT GDAL DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT FFTW DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT ITK DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT OPENTHREADS DEFAULT OFF)
if(WIN32)
  SETUP_SYSTEM_LIBRARY(PROJECT OSSIM DEFAULT ON)
else()
  SETUP_SYSTEM_LIBRARY(PROJECT OSSIM DEFAULT OFF)
endif()
SETUP_SYSTEM_LIBRARY(PROJECT MUPARSER DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT MUPARSERX DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT TINYXML DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT OPENCV DEFAULT OFF)
SETUP_SYSTEM_LIBRARY(PROJECT PCRE DEFAULT ON)
SETUP_SYSTEM_LIBRARY(PROJECT SWIG DEFAULT ON)

SETUP_SYSTEM_LIBRARY(PROJECT QT4 DEFAULT ON)


# Call OTB
option(OTB_USE_6S "Enable module 6S in OTB" ON)
option(OTB_USE_CURL "Enable module Curl in OTB" ON)
option(OTB_USE_EDISON "Enable module Edison in OTB" ON)
option(OTB_USE_LIBKML "Enable module libKML in OTB" ON)
option(OTB_USE_LIBSVM "Enable module libSVM in OTB" ON)
option(OTB_USE_MAPNIK "Enable module Mapnik in OTB" OFF)
option(OTB_USE_MUPARSER "Enable module muparser in OTB" ON)
option(OTB_USE_MUPARSERX "Enable module muparserX in OTB" ON)
option(OTB_USE_OPENCV "Enable module OpenCV in OTB" ON)
option(OTB_USE_OPENJPEG "Enable module openjpeg in OTB" ON)
option(OTB_USE_QT4 "Enable module QT4 in OTB" ${ENABLE_QT4})
option(OTB_USE_SIFTFAST "Enable module Siftfast in OTB" ON)

# set OTB_DATA_ROOT to run test
find_path(OTB_DATA_ROOT README-OTB-Data PATHS $ENV{OTB_DATA_ROOT} ${OTB-SuperBuild_SOURCE_DIR}/../OTB-Data)

if(ENABLE_OTB_LARGE_INPUTS)
  find_path(OTB_DATA_LARGEINPUT_ROOT OTBData.readme $ENV{OTB_DATA_LARGEINPUT_ROOT})
endif()

# SWIG Wrapping
option(OTB_WRAP_PYTHON "Enable python wrappers for OTB applications (requires SWIG)" OFF)
option(OTB_WRAP_JAVA "Enable java wrappers for OTB applications (requires SWIG)" OFF)

include(External_otb)

if(BUILD_ICE)
  option(BUILD_ICE_APPLICATION "Build the Ice application (requires glfw and glut)" OFF)
  include(External_ice)
endif()

if(BUILD_MONTEVERDI2)
  include(External_monteverdi2)
endif()

if(BUILD_MONTEVERDI)
  include(External_monteverdi)
endif()