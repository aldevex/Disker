cmake_minimum_required(VERSION 3.16)
project(Disker C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Get platform
if(WIN32)
    set(PLATFORM "windows")
else()
    set(PLATFORM "linux")
endif()

# Get and cache UI type and register property for CMake GUI (TUI default)
set(UI_TYPE "tui" CACHE STRING "UI type (gui/tui)")
set_property(CACHE UI_TYPE PROPERTY STRINGS "gui" "tui")

add_executable(Disker)



target_include_directories(Disker PUBLIC "src/")

target_sources(Disker PRIVATE
    # Memory management
    "src/mem/core_impl.c"
    # Core: Utils
    "src/core/utils/checksum.c"
    "src/core/utils/tui.c"
    "src/core/utils/string.c"
    "src/core/utils/file.c"
    "src/core/utils/guid.c"
    # Layout: Generic function interface
    "src/core/layout/generic.c"
    # Layout: Disk types
    "src/core/layout/disk/raw_img.c"
)
    # Platform-specific code
if(PLATFORM STREQUAL "windows")
    target_sources(Disker PRIVATE
        "src/base/impl_windows.c" # Windows & C standard base
        "res/windows.rc"          # Windows resource code (CMake compiles it)
    )
else()
    target_sources(Disker PRIVATE
        "src/base/impl_linux.c"
    )
endif()

    # Core: Main (UI-specific)
if(UI_TYPE STREQUAL "tui")
    target_sources(Disker PRIVATE
        "src/core/tui/entry.c"
        "src/core/tui/core.c"
        "src/core/tui/input.c"
        "src/core/tui/parse.c"
    )
elseif(UI_TYPE STREQUAL "gui")
    target_sources(Disker PRIVATE
        "src/core/gui/entry.c"
        "src/core/gui/parse.c"
        "src/core/gui/state.c"
    )
else()
    message(FATAL_ERROR "Invalid UI_TYPE: ${UI_TYPE}. expected 'gui' or 'tui'.")
endif()

if(PLATFORM STREQUAL "windows")
    target_link_options(Disker PRIVATE
        -Wl,/ignore:longsections
    )
    target_link_libraries(Disker PRIVATE
        shell32 ole32
    )
endif()

target_compile_options(Disker PRIVATE
    -finput-charset=UTF-8
    -fexec-charset=UTF-8
    -gdwarf
    -Wall
    -Wno-missing-braces
    -Wno-microsoft-goto
    -Wno-pragma-pack
    -Wno-misleading-indentation
    -D_CRT_SECURE_NO_WARNINGS
    -Werror
)
