@echo off

conan install . --build=missing --settings=build_type=Debug

cd build

cmake --preset conan-default