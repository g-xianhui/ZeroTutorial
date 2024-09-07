cmake_minimum_required(VERSION 3.8)
project(ZeroServer)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 20)

set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/${CMAKE_SYSTEM_NAME})
include_directories(${THIRDPARTY_DIR}/include/libevent)
link_directories(${THIRDPARTY_DIR}/lib)

set(THIRDPARTY_LIBS event_core event_extra)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND SYSTEM_LIBS
        ws2_32
    )
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
source_group(network ${CMAKE_CURRENT_SOURCE_DIR}/src/network)

file(GLOB GAME_SERVER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/game_server.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.cpp)
file(GLOB GAME_SERVER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*.h)

add_executable(game_server ${GAME_SERVER_SRCS} ${GAME_SERVER_HEADERS})
target_link_libraries(game_server ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})

IF (MSVC)
    set(MY_PATH "PATH=%PATH%" ${THIRDPARTY_DIR}/lib)
    set_target_properties(game_server PROPERTIES VS_DEBUGGER_ENVIRONMENT "${MY_PATH}")
ENDIF()

