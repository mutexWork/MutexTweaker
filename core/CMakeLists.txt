cmake_minimum_required(VERSION 3.16)
project(MutexTweaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Статический CRT (/MT): готовая сборка не требует Visual C++ Redistributable
# и не зависит от MSVCP140.dll / VCRUNTIME140.dll.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)

if (MSVC)
    add_compile_options(/utf-8 /MP)
endif()

include_directories(include)

file(GLOB_RECURSE SOURCES src/*.cpp src/Tweaks/*.cpp)

add_library(MutexTweaker SHARED ${SOURCES})

target_link_libraries(MutexTweaker wbemuuid ole32 oleaut32 powrprof advapi32 user32 iphlpapi dnsapi shell32)

set_target_properties(MutexTweaker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
