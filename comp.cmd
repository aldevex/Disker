@echo off

g++ src/main/main_win.cpp src/main/main_main.cpp ^
-lshell32 ^
-finput-charset=UTF-8 -fexec-charset=UTF-8
