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



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

# Use GNUInstallDirs to allow customisation
# but set our own default data and bin dir
if(NOT CMAKE_INSTALL_DATADIR)
    set(CMAKE_INSTALL_DATADIR "share/games")
endif()
if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "games")
endif()
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

include(Endian)
add_endian_definition()

include(CompileFlags)
compile_flags()



list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/rev.cpp")
if(WIN32)
    list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/ottdres.rc")
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


include(AddCustomXXXTimestamp)

if(OPTION_TOOLS_ONLY)
    if(HOST_BINARY_DIR)
        unset(HOST_BINARY_DIR CACHE)
    endif()
    add_subdirectory(${CMAKE_SOURCE_DIR}/src)
    return()
endif()


# Prefer -pthread over -lpthread, which is often the better option of the two.
set(CMAKE_THREAD_PREFER_PTHREAD YES)
# Make sure we have Threads available.
find_package(Threads REQUIRED)

find_package(ZLIB)
find_package(LibLZMA)
find_package(LZO)
find_package(PNG)

if(WIN32 OR EMSCRIPTEN)
    # Windows uses WinHttp for HTTP requests.
    # Emscripten uses Javascript for HTTP requests.
else()
    # All other targets use libcurl.
    find_package(CURL)
endif()

# Breakpad doesn't support emscripten.
if(NOT EMSCRIPTEN)
    find_package(unofficial-breakpad)
endif()

if(NOT OPTION_DEDICATED)
    find_package(OpusFile)
endif()

if(NOT EMSCRIPTEN AND NOT OPTION_DEDICATED)
    find_package(OpenGL COMPONENTS OpenGL)
endif()

if(MSVC)
    find_package(Editbin REQUIRED)
endif()

find_package(SSE)
find_package(Xaudio2)

find_package(Grfcodec)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_FOUND)

show_options()



if(OPTION_PACKAGE_DEPENDENCIES)
    if(NOT UNIX)
        message(FATAL_ERROR "Can only package dependencies on Linux")
    endif()
    if(OPTION_INSTALL_FHS)
        message(FATAL_ERROR "Cannot install in FHS folders when we are packaging dependencies")
    endif()
    if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
        message(FATAL_ERROR "OPTION_PACKAGE_DEPENDENCIES can only work with CMake 3.16+; you are using ${CMAKE_VERSION}")
    endif()

    # If we are packaging dependencies, we do two things:
    # 1) set the RPATH to include $ORIGIN/lib; $ORIGIN (that literal string)
    #    is a Linux indicator for "path where application is". In CMake, we
    #    have to do this before add_executable() is executed.
    # 2) copy the libraries that we compile against to the "lib" folder.
    #    This is done in InstallAndPackage.cmake.
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

include(CTest)
include(SourceList)

# Needed by rev.cpp
include_directories(${CMAKE_SOURCE_DIR}/src)
# Needed by everything that uses Squirrel
include_directories(${CMAKE_SOURCE_DIR}/src/3rdparty/squirrel/include)

include(MSVCFilters)

add_library(openttd_lib OBJECT ${GENERATED_SOURCE_FILES})
add_library(openttd_lib SHARED ${GENERATED_SOURCE_FILES})

add_executable(openttd WIN32)
add_executable(openttd_test)

add_library(openttd WIN32 SHARED ${GENERATED_SOURCE_FILES})

set_target_properties(openttd PROPERTIES OUTPUT_NAME "${BINARY_NAME}")



# All other files are added via target_sources()

if(MSVC)
    # Add DPI manifest to project; other WIN32 targets get this via ottdres.rc
    target_sources(openttd PRIVATE "${CMAKE_SOURCE_DIR}/os/windows/openttd.manifest")

    # If target -static is used, switch our project to static (/MT) too.
    # If the target ends on -static-md, it will remain dynamic (/MD).
    if(VCPKG_TARGET_TRIPLET MATCHES "-static" AND NOT VCPKG_TARGET_TRIPLET MATCHES "-md")
        set_property(TARGET openttd_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
     endif()
endif()

target_precompile_headers(openttd_lib
    PRIVATE
    src/stdafx.h
    src/core/format.hpp
)

add_subdirectory(${CMAKE_SOURCE_DIR}/bin)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/media)

add_dependencies(openttd
    find_version)

target_link_libraries(openttd_lib
    openttd::languages
    openttd::settings
    openttd::script_api
    Threads::Threads
)

target_link_libraries(openttd
    openttd_lib
    openttd::media
    openttd::basesets
)

target_link_libraries(openttd_test PRIVATE openttd_lib)
if(ANDROID)
    target_link_libraries(openttd_test PRIVATE log)
endif()

include(Catch)
catch_discover_tests(openttd_test)

if(HAIKU)
    target_link_libraries(openttd_lib "be" "network" "midi")
endif()

if(IPO_FOUND)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE True)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL True)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO True)
endif()
set_target_properties(openttd PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
process_compile_flags()

include(LinkPackage)
link_package(PNG TARGET PNG::PNG ENCOURAGED)
link_package(ZLIB TARGET ZLIB::ZLIB ENCOURAGED)
link_package(LIBLZMA TARGET LibLZMA::LibLZMA ENCOURAGED)
link_package(LZO)




if(NOT OPTION_DEDICATED)
    link_package(Fluidsynth)
    link_package(SDL)
    link_package(SDL2 TARGET SDL2::SDL2)
    link_package(Allegro)
    link_package(FREETYPE TARGET Freetype::Freetype)
    link_package(Fontconfig TARGET Fontconfig::Fontconfig)
    link_package(Harfbuzz TARGET harfbuzz::harfbuzz)
    link_package(ICU_i18n)
    link_package(ICU_uc)
    link_package(OpusFile TARGET OpusFile::opusfile)

    if(SDL2_FOUND AND OPENGL_FOUND AND UNIX)
        # SDL2 dynamically loads OpenGL if needed, so do not link to OpenGL when
        # on Linux. For Windows, we need to link to OpenGL as we also have a win32
        # driver using it.
        add_definitions(-DWITH_OPENGL)
        message(STATUS "OpenGL found -- -DWITH_OPENGL -- (via SDL2)")
    else()
        link_package(OpenGL TARGET OpenGL::GL)
    endif()
endif()

include(3rdparty/llvm/CheckAtomic)




if(NOT PERSONAL_DIR STREQUAL "(not set)")
    add_definitions(
        -DWITH_PERSONAL_DIR
        -DPERSONAL_DIR="${PERSONAL_DIR}"
    )
endif()

if(NOT SHARED_DIR STREQUAL "(not set)")
    add_definitions(
        -DWITH_SHARED_DIR
        -DSHARED_DIR="${SHARED_DIR}"
    )
endif()

if(NOT GLOBAL_DIR STREQUAL "(not set)")
    add_definitions(
        -DGLOBAL_DATA_DIR="${GLOBAL_DIR}"
    )
endif()

link_package(SSE)

add_definitions_based_on_options()

if(WIN32)
    add_definitions(
        -DUNICODE
        -D_UNICODE
        -DWITH_UNISCRIBE
        -DPSAPI_VERSION=1
    )

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

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DPOINTER_IS_64BIT)
endif()

enable_testing()

add_subdirectory(regression)

if(APPLE OR WIN32)
    find_package(Pandoc)
endif()

include(InstallAndPackage)
