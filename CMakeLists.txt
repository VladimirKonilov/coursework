cmake_minimum_required(VERSION 3.16)
project(netcourse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(netcourse_core
    src/protocol.cpp
    src/conversion.cpp
    src/user_store.cpp
    src/server_app.cpp
    src/client_lib.cpp
)

target_compile_options(netcourse_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(netcourse_core PRIVATE pthread)

add_executable(netcourse_server src/server_main.cpp)
target_link_libraries(netcourse_server PRIVATE netcourse_core pthread)

add_executable(netcourse_client src/client_main.cpp)
target_link_libraries(netcourse_client PRIVATE netcourse_core pthread)

add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE netcourse_core pthread)

add_executable(acceptance_test tests/acceptance_test.cpp)
target_link_libraries(acceptance_test PRIVATE netcourse_core pthread)

add_test(NAME smoke_test COMMAND smoke_test)
add_test(NAME acceptance_test COMMAND acceptance_test)

set_target_properties(
    netcourse_server
    netcourse_client
    smoke_test
    acceptance_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)
