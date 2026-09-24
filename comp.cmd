@echo off

gcc -std=c17 -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
-Wall -Werror -g ^
src/mem/darray_imp.c src/entry/tui.c ^
src/core/ssimp_win.c src/core/main_tui.c src/core/commands.c ^
-lole32
