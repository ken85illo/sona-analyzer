@echo off
cd %~dp0
cd ..
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cd ..
cmake --build build
copy build\bin\sona_lexical_analyzer.exe bin\Release