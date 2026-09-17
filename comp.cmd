@echo off

gcc -std=c17 -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
-Wall -Werror -g ^
src/mem/darray_imp.c src/core/ssimp_win.c src/entry/tui.c ^
-lole32
