@echo off
setlocal

rem Build
cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang  || exit /b %errorlevel%
cmake --build "build"                                 || exit /b %errorlevel%
echo CMake build done

rem Production binary
copy /y "build\Disker.exe" "bin\"

rem Dev quick test
copy /y "build\Disker.exe" ".\a.exe"

endlocal
