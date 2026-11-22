cmake_minimum_required(VERSION 3.23)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(YK_LZW20_IS_TOP_LEVEL ON)
else()
  set(YK_LZW20_IS_TOP_LEVEL OFF)
endif()

option(YK_LZW20_BUILD_TESTING "build tests" ${YK_LZW20_IS_TOP_LEVEL})

# Takes effect only when this is the first project(); a parent project, or a
# user passing their own CMAKE_PROJECT_TOP_LEVEL_INCLUDES, provides Catch2 itself
if(YK_LZW20_BUILD_TESTING AND NOT DEFINED CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
  set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_CURRENT_LIST_DIR}/test/cmake/dependencies.cmake")
endif()

project(yk-lzw20 LANGUAGES CXX)

add_library(lzw20 INTERFACE)
add_library(yk::lzw20 ALIAS lzw20)

target_compile_features(lzw20 INTERFACE cxx_std_20)

file(GLOB_RECURSE YK_LZW20_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)

target_sources(
    lzw20
    INTERFACE
        FILE_SET yk_lzw20_headers
            TYPE HEADERS
            BASE_DIRS include
            FILES ${YK_LZW20_HEADERS}
)

if(YK_LZW20_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
