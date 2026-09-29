@echo off
setlocal

cmake -B "build" -G "Ninja" -DCMAKE_C_COMPILER=clang
cmake --build "build"
del "Disker.exe"
move "build\Disker.exe" "."
rename "Disker.exe" "a.exe"

endlocal
