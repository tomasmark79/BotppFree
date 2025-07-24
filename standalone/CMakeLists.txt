cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# MIT License Copyright (c) 2024-2025 Tomáš Mark

# +-+-+-+-+-+-+-+-+-+-+-+-+
# STANDALONE
# +-+-+-+-+-+-+-+-+-+-+-+-+

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0091 NEW)

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
include(../cmake/tmplt-runtime.cmake)
include(../cmake/tmplt-sanitizer.cmake)
include(../cmake/tmplt-hardening.cmake)
include(../cmake/tmplt-ipo.cmake)
include(../cmake/tmplt-debug.cmake)

# === linting C/C++ code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Project attributes
# ==============================================================================
set(STANDALONE_NAME botpp)
string(TOLOWER "${STANDALONE_NAME}" STANDALONE_NAME_LOWER)
set(STANDALONE_NAMESPACE dotname)
project(
    ${STANDALONE_NAME}
    LANGUAGES C CXX ASM
    DESCRIPTION "template Copyright (c) 2024 TomasMark [at] digitalspace.name"
    HOMEPAGE_URL "https://github.com/tomasmark79")

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        WARNING
            "In-source builds. Please make a new directory (called a Build directory) and run CMake from there."
    )
endif()

# ==============================================================================
# System / Conan dependencies
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# ==============================================================================
# CPM.cmake dependencies - take care conflicts
# ==============================================================================
include(../cmake/CPM.cmake)
CPMAddPackage(NAME IBot SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
cpm_licenses_create_disclaimer_target(
    write-licenses-${STANDALONE_NAME}
    "${CMAKE_CURRENT_BINARY_DIR}/${STANDALONE_NAME}_third_party.txt" "${CPM_PACKAGES}")
CPMAddPackage(
    GITHUB_REPOSITORY jarro2783/cxxopts
    VERSION 3.2.1
    OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL NO")
# ==============================================================================
# src/ for source files and `internal` header files that are not intended for public use
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

# add stable Main.cpp
list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp)

# ==============================================================================
# Create target
# ==============================================================================
add_executable(${STANDALONE_NAME} ${sources})

apply_hardening(${STANDALONE_NAME})
apply_sanitizers(${STANDALONE_NAME})
apply_static_runtime(${STANDALONE_NAME})
apply_debug_info_control(${STANDALONE_NAME})

# ==============================================================================
# Set linking
# ==============================================================================
target_link_libraries(${STANDALONE_NAME} PRIVATE dotname::IBot cxxopts)

# ==============================================================================
# Add missing installation exports
# ==============================================================================
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
install(TARGETS ${STANDALONE_NAME} RUNTIME DESTINATION bin)

# ==============================================================================
# Asset files processing must be before Emscripten filesystem mounting
# ==============================================================================
include(../cmake/tmplt-assets.cmake)
apply_assets_processing_standalone()

# ==============================================================================
# Set target properties
# ==============================================================================

# emscripten handler
include(../cmake/tmplt-emscripten.cmake)
emscripten(${STANDALONE_NAME} 1 1 "")

# ==============================================================================
# GTests processing via interface
# ==============================================================================
if(ENABLE_GTESTS)
    message(STATUS "GTESTS enabled")
    add_library(standalone_common INTERFACE)
    target_link_libraries(standalone_common INTERFACE dotname::IBot cxxopts)
    add_library(dotname::standalone_common ALIAS standalone_common)
    add_subdirectory(tests)
endif()
