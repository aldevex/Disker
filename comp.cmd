@echo off
setlocal

rem Get default MBR from assembly
rem nasm -f bin "src/disk/scheme/mbr_default.nasm" -o "src/disk/scheme/mbr_default.bin"

rem Build
rem cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release || exit /b %errorlevel%
cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang || exit /b %errorlevel%
cmake --build "build"                                 || exit /b %errorlevel%
echo CMake build done

rem Production binary
if not exist "prod\" mkdir "prod\"
if not exist "prod\data\" mkdir "prod\data\"
if not exist "prod\docs\" mkdir "prod\docs\"
copy /y "build\Disker.exe" "prod\"

rem Dev quick test
copy /y "build\Disker.exe" ".\a.exe"

endlocal
