cmake_minimum_required(VERSION 3.20)
project(TechVideoEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

# Find required packages
find_package(DirectX REQUIRED QUIET)

# Dear ImGui sources (will be populated by git submodule or manual copy)
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
    external/imgui/backends/imgui_impl_win32.cpp
)

# Miniaudio (single header library)
set(MINIAUDIO_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/miniaudio)

# Source files
set(SOURCES
    src/main.cpp
    src/core/Engine.cpp
    src/core/FrameProcessor.cpp
    src/core/Timeline.cpp
    src/capture/ScreenCapture.cpp
    src/capture/KeyHook.cpp
    src/audio/AudioMixer.cpp
    src/audio/WASAPICapture.cpp
    src/ui/UIManager.cpp
    src/utils/Logger.cpp
)

set(HEADERS
    include/Engine.h
    include/FrameProcessor.h
    include/Timeline.h
    include/ScreenCapture.h
    include/KeyHook.h
    include/AudioMixer.h
    include/WASAPICapture.h
    include/UIManager.h
    include/Logger.h
)

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
    ${MINIAUDIO_INCLUDE_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        dxgi
        mfreadwrite
        mfplat
        mfuuid
        ole32
        user32
        shell32
        comctl32
        imm32
        setupapi
        ws2_32
    )
    
    # Enable hardware transforms for Media Foundation
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS
        _ENABLE_EXTENDED_ALIGNED_STORAGE
    )
endif()

# Compiler optimizations
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -Wall -Wextra)
endif()

# Copy miniaudio header if not present
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/miniaudio/miniaudio.h")
    message(STATUS "Downloading miniaudio.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
        "${CMAKE_SOURCE_DIR}/external/miniaudio/miniaudio.h"
        SHOW_PROGRESS
    )
endif()

# ImGui setup notice
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp")
    message(WARNING "Dear ImGui not found. Please clone it into external/imgui/")
    message(STATUS "Run: git clone --depth 1 https://github.com/ocornut/imgui.git external/imgui")
endif()
