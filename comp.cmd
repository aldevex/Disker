@echo off

rem Make directories
if not exist "build\" mkdir "build\"
if not exist "build\diskerdev\" mkdir "build\diskerdev\"
if not exist "prod\" mkdir "prod\"
if not exist "prod\data\" mkdir "prod\data\"
if not exist "prod\docs\" mkdir "prod\docs\"

rem Get default MBR from assembly
clang "precomp/embed.c" -std=c17 -D_CRT_SECURE_NO_WARNINGS -o "precomp/embed.exe"
nasm -f bin "res/default_mbr.nasm" -o "build/diskerdev/default_mbr.bin"
"precomp/embed"

rem Build
rem cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release || exit /b %errorlevel%
cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang || exit /b %errorlevel%
cmake --build "build"                                 || exit /b %errorlevel%
echo CMake build done

rem Production files and binaries
copy /y "res\config.ini" "prod\data\"
copy /y "build\Disker.exe" "prod\disker.exe"
