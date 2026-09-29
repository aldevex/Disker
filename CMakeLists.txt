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



set(SOURCES
    # Memory
    "src/mem/core_imp.c"
    # Utils
    "src/utils/checksum.c"
    "src/utils/tui.c"
    "src/utils/string.c"
    "src/utils/file.c"
    "src/utils/path.c"
    "src/utils/guid.c"
    # Disk
    # No C files yet
    # Core: Commands
    "src/core/cmds/write.c"
)

    # System-specific implementation
if(PLATFORM STREQUAL "windows")
    list(APPEND SOURCES "src/ssimp/windows.c")
else()
    list(APPEND SOURCES "src/ssimp/linux.c")
endif()

    # Core: UI-specific
if(UI_TYPE STREQUAL "tui")
    list(APPEND SOURCES
        "src/core/tui/entry.c"
        "src/core/tui/parse.c"
        "src/core/tui/state.c"
    )
elseif(UI_TYPE STREQUAL "gui")
    list(APPEND SOURCES
        "src/core/gui/entry.c"
        "src/core/gui/parse.c"
        "src/core/gui/state.c"
    )
else()
    message(FATAL_ERROR "Invalid UI_TYPE: ${UI_TYPE}. expected 'gui' or 'tui'.")
endif()



add_executable(Disker ${SOURCES})

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
