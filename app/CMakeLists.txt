cmake_minimum_required(VERSION 3.16)
project(FireBarrier LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
)

set(MAIN_DIR ${CMAKE_SOURCE_DIR}/../app)
file(GLOB_RECURSE SOURCES
    "${MAIN_DIR}/src/*.cpp"
    "${MAIN_DIR}/include/*.h"
    "${MAIN_DIR}/resources.qrc"
)

# Paths to external dependencies
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external")

# NNG include + library paths
set(NNG_INCLUDE "${EXTERNAL_DIR}/nng/include")
set(NNG_LIB_DIR "${EXTERNAL_DIR}/nng/lib/x64")

include_directories(
    ${NNG_INCLUDE}
    ${MAIN_DIR}/include
)

link_directories(
    ${NNG_LIB_DIR}
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(FireBarrier PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_definitions(FireBarrier PRIVATE 
    NNG_STATIC_LIB
)

target_link_libraries(${PROJECT_NAME}
    nng
    ws2_32
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
)