@echo off
setlocal

rem Make directories
if not exist "build\" mkdir "build\"
if not exist "build\diskerdev\" mkdir "build\diskerdev\"
if not exist "prod\" mkdir "prod\"
if not exist "prod\data\" mkdir "prod\data\"
if not exist "prod\docs\" mkdir "prod\docs\"

rem Get default MBR from assembly
rem clang "./embed.c" -std=c17 -D_CRT_SECURE_NO_WARNINGS -o "./embed.exe"
rem nasm -f bin "src/disk/scheme/mbr_default.nasm" -o "build/diskerdev/mbr_default.bin"
rem "./embed"

rem Build
rem cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release || exit /b %errorlevel%
cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang || exit /b %errorlevel%
cmake --build "build"                                 || exit /b %errorlevel%
echo CMake build done

rem Production binary
copy /y "build\Disker.exe" "prod\"

rem Dev quick test
copy /y "build\Disker.exe" ".\a.exe"

endlocal
