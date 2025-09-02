@echo off

conan install . --build=missing --settings=build_type=Debug

cmake --preset conan-default -DENABLE_PVD=OFF