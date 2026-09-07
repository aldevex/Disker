@echo off

g++ src/entry/win.cpp src/utils/ssutils_win.cpp ^
-lshell32 ^
-finput-charset=UTF-8 -fexec-charset=UTF-8
