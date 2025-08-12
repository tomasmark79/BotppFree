cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# MIT License Copyright (c) 2024-2025 Tomáš Mark

# +-+-+-+-+-+-+-+-+-+-+
# LIBRARY
# +-+-+-+-+-+-+-+-+-+-+

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0091 NEW)
add_compile_options(-fdiagnostics-color=always)

option(ENABLE_CCACHE "Use ccache compiler cache" ON)
option(BUILD_SHARED_LIBS "Build shared (.so) libraries" OFF)
option(USE_STATIC_RUNTIME "Link C++ runtime statically" OFF)
option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)
option(SANITIZE_UNDEFINED "Enable undefined behavior sanitizer" OFF)
option(SANITIZE_THREAD "Enable thread sanitizer" OFF)
option(SANITIZE_MEMORY "Enable memory sanitizer" OFF)
option(ENABLE_HARDENING "Enable security hardening" OFF)
option(ENABLE_IPO "Enable link-time optimization" OFF)

option(ENABLE_GTESTS "Build and run unit tests" ON)

if(ENABLE_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()
include(cmake/tmplt-runtime.cmake)
include(cmake/tmplt-sanitizer.cmake)
include(cmake/tmplt-hardening.cmake)
include(cmake/tmplt-ipo.cmake)
include(cmake/tmplt-debug.cmake)

# Linting C/C++ code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Library attributes - case size will reflect resource files case size
# ==============================================================================
set(LIBRARY_NAME IBot)
set(LIBRARY_NAMESPACE dotname)
project(
    ${LIBRARY_NAME}
    VERSION 0.0.13
    LANGUAGES C CXX
    DESCRIPTION "template Copyright (c) 2024 TomasMark [at] digitalspace.name"
    HOMEPAGE_URL "https://github.com/tomasmark79")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        WARNING
            "In-source builds. Please make a new directory (called a Build directory) and run CMake from there."
    )
endif()

# ==============================================================================
# Library installation attributes
# ==============================================================================
set(INSTALL_INCLUDE_DIR include/${LIBRARY_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}/
        DESTINATION ${INSTALL_INCLUDE_DIR})

# ==============================================================================
# System / Conan dependencies
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Opus REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# ==============================================================================
# CPM.cmake dependencies - take care conflicts
# ==============================================================================
# export CPM_SOURCE_CACHE=$HOME/.cache/CPM # for localy caching CPM packages
# ==============================================================================
include(cmake/CPM.cmake)
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.12.0")
CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.7")
cpm_licenses_create_disclaimer_target(
    write-licenses-${LIBRARY_NAME} "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY_NAME}_third_party.txt"
    "${CPM_PACKAGES}")

# ccache help lots with dpp
# brainboxdotcc/DPP https://github.com/brainboxdotcc/DPP/blob/master/CMakeLists.txt important
# "CONAN_EXPORTED ON" has to be used together with Conan
CPMAddPackage(
    NAME dpp
    GITHUB_REPOSITORY brainboxdotcc/DPP
    GIT_TAG v10.1.3
    OPTIONS "BUILD_SHARED_LIBS ON" "DPP_BUILD_TEST OFF" "DPP_INSTALL OFF" "DPP_NO_CORO OFF"
            "CONAN_EXPORTED ON")
#equivalent to: "DPP_INSTALL ON" in CPM for DPP Target - kept for example
install(TARGETS dpp EXPORT ${LIBRARY_NAME}Targets)

# ==============================================================================
# public header files
# ==============================================================================
file(
    GLOB_RECURSE
    headers
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx)

# ==============================================================================
# source files and `internal` header files that are not intended for public use
# ==============================================================================
file(
    GLOB_RECURSE
    sources
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

# ==============================================================================
# Create target
# ==============================================================================
add_library(${LIBRARY_NAME})
target_sources(${LIBRARY_NAME} PRIVATE ${headers} ${sources})

apply_ipo(${LIBRARY_NAME})
apply_hardening(${LIBRARY_NAME})
apply_sanitizers(${LIBRARY_NAME})
apply_static_runtime(${LIBRARY_NAME})
apply_debug_info_control(${LIBRARY_NAME})

# ==============================================================================
# Set target properties
# ==============================================================================
# emscripten handler
include(cmake/tmplt-emscripten.cmake)
emscripten(${LIBRARY_NAME} 0 1 "")

# ==============================================================================
# Set headers
# ==============================================================================
# header-only libraries change all PUBLIC flags to INTERFACE and create an interface target:
# <add_library(${LIBRARY_NAME} INTERFACE)>
target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    PUBLIC $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>)

# ==============================================================================
# Set compile options
# ==============================================================================
# note: generator expressions are evaluated during generation of the buildsystem, and not during
# processing of CMakeLists.txt files
target_compile_options(
    ${LIBRARY_NAME}
    PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive-;/W4>"
    PUBLIC
        "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CXX,MSVC>>,$<NOT:$<PLATFORM_ID:Darwin>>,$<NOT:$<CXX_COMPILER_ID:Clang>>>:-Wall;-Wextra;-Wpedantic;-MMD;-MP>"
    PUBLIC
        "$<$<AND:$<NOT:$<COMPILE_LANG_AND_ID:CXX,MSVC>>,$<PLATFORM_ID:Darwin>>:-Wall;-Wextra;-Wpedantic>"
)

# ==============================================================================
# Set compile features C++ version from Conan Profile has priority over this setting
# ==============================================================================
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Set linking
# ==============================================================================
target_link_libraries(
    ${LIBRARY_NAME}
    PUBLIC fmt::fmt
    PUBLIC ZLIB::ZLIB
    PUBLIC tinyxml2::tinyxml2
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE Opus::opus
    PUBLIC CURL::libcurl
    PRIVATE OpenSSL::SSL
    PRIVATE OpenSSL::Crypto
    PUBLIC dpp)

# ==============================================================================
# set packageProject arttributes for library
# ==============================================================================
packageProject(
    NAME ${LIBRARY_NAME}
    VERSION ${PROJECT_VERSION}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR "/include"
    INCLUDE_DESTINATION "include"
    INCLUDE_HEADER_PATTERN "*.h;*.hpp;*.hh;*.hxx"
    DEPENDENCIES "cxxopts#3.2.1;fmt#11.2.0;dpp#10.1.3;CPMLicenses.cmake@0.0.7"
    VERSION_HEADER "${LIBRARY_NAME}/version.h"
    EXPORT_HEADER "${LIBRARY_NAME}/export.h"
    NAMESPACE ${LIBRARY_NAMESPACE}
    # `AnyNewerVersion|SameMajorVersion|SameMinorVersion'
    COMPATIBILITY AnyNewerVersion
    DISABLE_VERSION_SUFFIX YES
    # YES for header only (i.e. INTERFACE) libraries
    ARCH_INDEPENDENT NO
    CPACK YES
    RUNTIME_DESTINATION /)
