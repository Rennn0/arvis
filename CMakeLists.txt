cmake_minimum_required(VERSION 3.24)
project(maskj
    VERSION 1.0.0 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use a prebuilt libcurl (install via vcpkg or system package manager).
# This keeps the generated solution clean — only the maskj project appears,
# with no third-party curl subprojects.
find_package(CURL REQUIRED)

add_executable(maskj
    src/main.cpp
    src/network_manager.cpp
)

target_include_directories(maskj PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(maskj PRIVATE CURL::libcurl)

# Make Visual Studio open 'maskj' as the startup project instead of ALL_BUILD
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT maskj)

if(MSVC)
    target_compile_options(maskj PRIVATE /W4)
else()
    target_compile_options(maskj PRIVATE -Wall -Wextra -Wpedantic)
endif()
