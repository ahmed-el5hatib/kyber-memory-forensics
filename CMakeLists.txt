cmake_minimum_required(VERSION 3.12)
project(KyberMemoryForensics C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to force manual paths in case find_package fails or needs to be bypassed
option(USE_MANUAL_OQS_PATHS "Set to ON to manually specify paths for liboqs headers and library" OFF)

if(NOT USE_MANUAL_OQS_PATHS)
    # First, try to find liboqs configuration package (normally installed via CMake)
    find_package(liboqs CONFIG QUIET)
    if(liboqs_FOUND)
        message(STATUS "Found liboqs via find_package CONFIG. Target: liboqs")
    else()
        find_package(liboqs QUIET)
    endif()
endif()

# Fallback mechanism: Manually configure liboqs paths if find_package failed or was bypassed
if(NOT liboqs_FOUND)
    message(STATUS "liboqs package not automatically found. Allowing manual paths.")
    
    # Expose variables in the CMake cache so the user can easily point to their liboqs installation
    set(OQS_INCLUDE_DIR "" CACHE PATH "Path to the directory containing oqs/oqs.h")
    set(OQS_LIBRARY "" CACHE FILEPATH "Path to the liboqs import library (e.g., oqs.lib, liboqs.dll.a, or liboqs.lib)")

    if(OQS_INCLUDE_DIR AND OQS_LIBRARY)
        message(STATUS "Using user-specified manual liboqs paths:")
        message(STATUS "  Headers: ${OQS_INCLUDE_DIR}")
        message(STATUS "  Library: ${OQS_LIBRARY}")
        
        add_library(liboqs_manual INTERFACE)
        target_include_directories(liboqs_manual INTERFACE ${OQS_INCLUDE_DIR})
        target_link_libraries(liboqs_manual INTERFACE ${OQS_LIBRARY})
        set(liboqs_TARGET liboqs_manual)
    else()
        message(WARNING "liboqs package was not found and OQS_INCLUDE_DIR / OQS_LIBRARY are empty.")
        message(STATUS "Falling back to default compiler search paths with '-loqs'.")
        
        add_library(liboqs_default INTERFACE)
        target_link_libraries(liboqs_default INTERFACE oqs)
        set(liboqs_TARGET liboqs_default)
    endif()
else()
    # Handle targets exported by the official liboqs package
    if(TARGET OQS::oqs)
        set(liboqs_TARGET OQS::oqs)
    elseif(TARGET liboqs)
        set(liboqs_TARGET liboqs)
    else()
        add_library(liboqs_imported INTERFACE)
        target_include_directories(liboqs_imported INTERFACE ${liboqs_INCLUDE_DIRS})
        target_link_libraries(liboqs_imported INTERFACE ${liboqs_LIBRARIES})
        set(liboqs_TARGET liboqs_imported)
    endif()
endif()

# Main executable
add_executable(kyber_poc main.c)

# Link against liboqs (either found package, manual interface, or default fallback)
target_link_libraries(kyber_poc PRIVATE ${liboqs_TARGET})

# Windows specific requirements (liboqs uses sockets and helper APIs from Winsock)
if(WIN32)
    target_link_libraries(kyber_poc PRIVATE ws2_32)
endif()
