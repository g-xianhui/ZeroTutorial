@echo off

.\bin\protoc.exe --cpp_out=../server/src/proto *.proto
.\bin\protogen.exe --proto_path=. --csharp_out=../client/Assets/Scripts/network/proto *.proto
