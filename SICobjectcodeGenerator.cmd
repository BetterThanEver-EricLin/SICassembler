@echo off
echo "SICtest.txt -> SIC.exe -> objcode.txt"
g++ -O3 -g -o SIC.exe SIC.cpp
SIC.exe SICtest.txt objcode.txt