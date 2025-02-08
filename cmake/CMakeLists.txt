cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "clang")
set(FSCORD_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(FSCORD_INC_DIR  "${FSCORD_ROOT_DIR}/src")
set(FSCORD_SRC_DIR  "${FSCORD_ROOT_DIR}/src")



project(fscord)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord)
add_executable(fscord)
set_property(TARGET fscord PROPERTY C_STANDARD 99)
target_sources(fscord PUBLIC
    ${FSCORD_SRC_DIR}/basic/mem_arena.c
    ${FSCORD_SRC_DIR}/basic/math.c
    ${FSCORD_SRC_DIR}/basic/string32.c
    ${FSCORD_SRC_DIR}/client/fscord.c
    ${FSCORD_SRC_DIR}/client/draw.c
    ${FSCORD_SRC_DIR}/client/asset_manager.c
    ${FSCORD_SRC_DIR}/client/sound.c
    ${FSCORD_SRC_DIR}/client/font.c
    ${FSCORD_SRC_DIR}/client/login.c
    ${FSCORD_SRC_DIR}/client/session.c
    ${FSCORD_SRC_DIR}/client/string32_handles.c
    ${FSCORD_SRC_DIR}/client/server_connection.c
    ${FSCORD_SRC_DIR}/crypto/aes_gcm.c
    ${FSCORD_SRC_DIR}/crypto/rsa.c
    ${FSCORD_SRC_DIR}/os/linux/linux_window.c
    ${FSCORD_SRC_DIR}/os/linux/linux_sound.c
    ${FSCORD_SRC_DIR}/os/linux/linux_library.c
    ${FSCORD_SRC_DIR}/os/linux/linux_work_queue.c
    ${FSCORD_SRC_DIR}/os/linux/linux_memory.c
    ${FSCORD_SRC_DIR}/os/linux/linux_net_secure_stream.c
)
target_include_directories(fscord PUBLIC ${FSCORD_INC_DIR})
target_link_libraries(fscord X11 GL asound ssl crypto)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord_server)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../data/fscord_server)
add_executable(fscord_server)
set_property(TARGET fscord_server PROPERTY C_STANDARD 99)
target_sources(fscord_server PUBLIC
    ${FSCORD_SRC_DIR}/basic/mem_arena.c
    ${FSCORD_SRC_DIR}/basic/string32.c
    ${FSCORD_SRC_DIR}/os/linux/linux_memory.c
    ${FSCORD_SRC_DIR}/os/linux/linux_net_secure_stream.c
    ${FSCORD_SRC_DIR}/crypto/aes_gcm.c
    ${FSCORD_SRC_DIR}/crypto/rsa.c
    ${FSCORD_SRC_DIR}/server/client_connections.c
    ${FSCORD_SRC_DIR}/server/c2s_handler.c
    ${FSCORD_SRC_DIR}/server/s2c_sender.c
    ${FSCORD_SRC_DIR}/server/fscord_server.c)
target_include_directories(fscord_server PUBLIC ${FSCORD_INC_DIR})
target_link_libraries(fscord_server ssl crypto)

