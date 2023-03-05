cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER clang++)

# set source directories
set(FSCORD_ALL_ROOT_DIR     "${CMAKE_CURRENT_LIST_DIR}/..")
set(FSCORD_ALL_SRC_DIR      "${FSCORD_ALL_ROOT_DIR}/src")
set(FSCORD_SERVER_SRC_DIR   "${FSCORD_ALL_SRC_DIR}/server")

# set binary directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

# set flags
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(
    ${FSCORD_SERVER_SRC_DIR}
    ${FSCORD_ALL_SRC_DIR}
    ${FSCORD_ALL_ROOT_DIR}/libs/include/libressl
)


project(fscord_server)

add_executable(fscord_server)
target_sources(fscord_server PUBLIC
    ${FSCORD_SERVER_SRC_DIR}/fscord_server.cpp
    ${FSCORD_SERVER_SRC_DIR}/MemoryArena.cpp
    ${FSCORD_SERVER_SRC_DIR}/AesGcm.cpp
    ${FSCORD_SERVER_SRC_DIR}/Rsa.cpp
    ${FSCORD_SERVER_SRC_DIR}/EpollManager.cpp
    ${FSCORD_SERVER_SRC_DIR}/ClientManager.cpp)

target_link_libraries(fscord_server ${FSCORD_ALL_ROOT_DIR}/libs/static/libressl/libcrypto.a)
