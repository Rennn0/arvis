cmake_minimum_required(VERSION 3.24)
project(maskj
    VERSION 1.0.0 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(STATUS "libcurl not found, fetching from github...")
    message(STATUS "libcurl not found on system — fetching and building statically")                            
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_8_0 
    )
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(curl)
endif()



add_executable(maskj
    src/main.cpp
    src/network_manager.cpp
)

target_include_directories(maskj PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(maskj PRIVATE CURL::libcurl)

if(MSVC)
    target_compile_options(maskj PRIVATE /W4)
else()
    target_compile_options(maskj PRIVATE -Wall -Wextra -Wpedantic)
endif()
