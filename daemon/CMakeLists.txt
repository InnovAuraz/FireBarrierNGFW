cmake_minimum_required(VERSION 3.16)
project(ngfw_daemon)

set(CMAKE_CXX_STANDARD 17)

# Paths to external dependencies
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external")

# NPCAP include + library paths
set(NPCAP_INCLUDE "${EXTERNAL_DIR}/npcap/include")
set(NPCAP_LIB_DIR "${EXTERNAL_DIR}/npcap/lib/x64")

# NNG include + library paths
set(NNG_INCLUDE "${EXTERNAL_DIR}/nng/include")
set(NNG_LIB_DIR "${EXTERNAL_DIR}/nng/lib")

include_directories(
    ${NPCAP_INCLUDE}
    ${NNG_INCLUDE}
    ${CMAKE_SOURCE_DIR}/include
)

link_directories(
    ${NPCAP_LIB_DIR}
    ${NNG_LIB_DIR}
)

file(GLOB SRC_FILES
    src/*.cpp
)

add_executable(ngfw_daemon ${SRC_FILES})

target_link_libraries(ngfw_daemon
    wpcap
    Packet
    nng
)