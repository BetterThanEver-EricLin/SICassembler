@echo off
echo "SICXEtest.txt -> SICXE.exe -> objcode.txt"
g++ -O3 -o SICXE.exe SICXE.cpp
SICXE.exe SICXEtest.txt objcode.txt