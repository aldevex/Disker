@echo off

g++ src/entry/entry_win.cpp src/algos/ssutils_win.cpp ^
-lshell32 ^
-finput-charset=UTF-8 -fexec-charset=UTF-8
