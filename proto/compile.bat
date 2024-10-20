@echo off

protoc.exe --cpp_out=../server/src/proto --csharp_out=../client/Assets/Scripts/network/proto *.proto
