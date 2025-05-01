cmake_minimum_required(VERSION 3.8)
project(ZeroServer)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 20)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND SYSTEM_LIBS
        ws2_32
    )
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
source_group(network ${CMAKE_CURRENT_SOURCE_DIR}/src/network)
source_group(combat ${CMAKE_CURRENT_SOURCE_DIR}/src/combat)
source_group(skill ${CMAKE_CURRENT_SOURCE_DIR}/src/combat/skill)
source_group(aoi ${CMAKE_CURRENT_SOURCE_DIR}/src/aoi)

file(GLOB GAME_SERVER_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/combat/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/combat/skill/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aoi/*.cpp
)
file(GLOB GAME_SERVER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/combat/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/combat/skill/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/aoi/*.h
)

add_executable(game_server ${GAME_SERVER_SRCS} ${GAME_SERVER_HEADERS})
target_link_libraries(game_server ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})

find_package(protobuf)
find_package(absl REQUIRED)
find_package(Libevent REQUIRED)
find_package(spdlog REQUIRED)

target_include_directories(game_server PUBLIC ${protobuf_INCLUDE_DIR} ${absl_INCLUDE_DIR} ${Libevent_INCLUDE_DIR})
target_link_libraries(game_server protobuf::protobuf)
target_link_libraries(game_server abseil::abseil)
target_link_libraries(game_server libevent::libevent)
target_link_libraries(game_server spdlog::spdlog)

IF (MSVC)
    # set_target_properties(game_server PROPERTIES VS_DEBUGGER_ENVIRONMENT "${MY_PATH}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
ENDIF()
