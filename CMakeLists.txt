cmake_minimum_required(VERSION 3.16)

if(NOT BINARY_NAME)
    set(BINARY_NAME openttd)
endif()

project(${BINARY_NAME}
    VERSION 15.0
    LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please run \"cmake ..\" from the build directory. You may need to delete \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" first.")
endif()

# Debug mode by default.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Windows-specific flags and settings
if(WIN32)
    add_definitions(
        -DUNICODE
        -D_UNICODE
        -DWITH_UNISCRIBE
        -DPSAPI_VERSION=1
    )

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES)  # Export all symbols automatically
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(GNUInstallDirs)

include(Options)
set_options()
set_directory_options()

include(Static)
set_static_if_needed()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# An empty target for the tools
add_custom_target(tools)

# Include necessary libraries
find_package(ZLIB REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(LZO REQUIRED)
find_package(PNG REQUIRED)

# Optional Windows packages
find_package(CURL REQUIRED)


list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/rev.cpp")
if(WIN32)
    list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/ottdres.rc")
endif()



add_library(openttd_lib OBJECT ${GENERATED_SOURCE_FILES})


if(WIN32)
    # Add necessary Windows libraries
    target_link_libraries(openttd_lib
        ws2_32
        winmm
        imm32
        usp10
        psapi
        winhttp
        bcrypt
    )
endif()

# Generate a target to determine version, which is execute every 'make' run
add_custom_target(find_version
        ${CMAKE_COMMAND}
                -DFIND_VERSION_BINARY_DIR=${CMAKE_BINARY_DIR}/generated
                -DCPACK_BINARY_DIR=${CMAKE_BINARY_DIR}
                -DREV_MAJOR=${PROJECT_VERSION_MAJOR}
                -DREV_MINOR=${PROJECT_VERSION_MINOR}
                -DWINDOWS=${WIN32}
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/FindVersion.cmake"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        BYPRODUCTS ${GENERATED_SOURCE_FILES}
)

include(LinkPackage)

# Include and link external libraries like ZLIB, PNG, etc.
link_package(ZLIB TARGET ZLIB::ZLIB ENCOURAGED)
link_package(LIBLZMA TARGET LibLZMA::LibLZMA ENCOURAGED)
link_package(LZO)

# Create a DLL from OpenTTD
add_library(openttd SHARED ${GENERATED_SOURCE_FILES} src/*.cpp)

# Ensure functions are exported properly for rundll32
target_compile_definitions(openttd PUBLIC EXPORT_OPENTTD)

# Make sure we export the functions needed for rundll32
target_sources(openttd
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/main.cpp  # Your entry point (if it exists)
)

# Set up the necessary precompiled headers for optimization
target_precompile_headers(openttd
    PRIVATE
    src/stdafx.h
    src/core/format.hpp
)

# Link the libraries required by OpenTTD (such as ZLIB, PNG, etc.)
target_link_libraries(openttd
    ZLIB::ZLIB
    LibLZMA::LibLZMA
    LZO
    PNG::PNG
    CURL::CURL
)

# Set the output name for the DLL (optional)
set_target_properties(openttd PROPERTIES OUTPUT_NAME "openttd_dll")

# Ensure tests and dependencies are linked properly
enable_testing()

# If you want to add tests or other components, you can add them here:
# add_subdirectory(tests)

# Now install the DLL and its dependencies as needed
include(InstallAndPackage)

# Finalize the build process
include(CTest)
include(SourceList)

