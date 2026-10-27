cmake_minimum_required(VERSION 3.16)
project(MutexTweakerGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)

if (MSVC)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_SOURCE_DIR}\"")
endif()

add_executable(MutexTweakerGUI WIN32
    src/main.cpp
    app.rc
    app.manifest
)

target_include_directories(MutexTweakerGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MutexTweakerCore/include
)

target_link_directories(MutexTweakerGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MutexTweakerCore/lib
)

target_link_libraries(MutexTweakerGUI PRIVATE
    MutexTweaker
    comctl32
    user32
    gdi32
    gdiplus
    shell32
    advapi32
)

if (MSVC)
    target_compile_options(MutexTweakerGUI PRIVATE /utf-8 /MT)
    target_link_options(MutexTweakerGUI PRIVATE
        "/MANIFEST:EMBED"
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/app.manifest"
        "/MANIFESTUAC:NO"
    )
endif()

set_target_properties(MutexTweakerGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET MutexTweakerGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/MutexTweakerCore/bin/MutexTweaker.dll
        $<TARGET_FILE_DIR:MutexTweakerGUI>/MutexTweaker.dll
)
