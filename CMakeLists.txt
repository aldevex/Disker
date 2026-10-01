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
    # Memory
    "src/mem/core_imp.c"
    # Utils
    "src/utils/checksum.c"
    "src/utils/tui.c"
    "src/utils/string.c"
    "src/utils/file.c"
    "src/utils/guid.c"
    # Disk: Info
    "src/disk/info.c"
    # Disk: Geometry
    "src/disk/geo/geo.c"
    # No C files yet
    # Core: Commands
    "src/core/cmds/write.c"
)
    # System-specific implementation
if(PLATFORM STREQUAL "windows")
    target_sources(Disker PRIVATE "src/ssimp/windows.c" "src/res/windows.rc")
else()
    target_sources(Disker PRIVATE "src/ssimp/linux.c")
endif()

    # Core: UI-specific
if(UI_TYPE STREQUAL "tui")
    target_sources(Disker PRIVATE
        "src/core/tui/entry.c"
        "src/core/tui/parse.c"
        "src/core/tui/state.c"
        "src/core/tui/error.c"
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

if(PLATFORM STREQUAL "windows")
    target_link_options(Disker PRIVATE -Wl,/ignore:longsections)
    target_link_libraries(Disker PRIVATE shell32 ole32)
endif()
