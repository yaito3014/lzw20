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

project(yk-lzw20 VERSION 0.2.0 LANGUAGES CXX)

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

option(YK_LZW20_INSTALL "generate install rules for lzw20" ${PROJECT_IS_TOP_LEVEL})

if(YK_LZW20_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(TARGETS lzw20 EXPORT lzw20_targets FILE_SET yk_lzw20_headers)
  install(
      EXPORT lzw20_targets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lzw20
      FILE lzw20-targets.cmake
      NAMESPACE yk::
  )

  configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lzw20-config.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/lzw20-config.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lzw20
  )
  # pre-1.0, a minor version may break the API
  write_basic_package_version_file(
      ${CMAKE_CURRENT_BINARY_DIR}/lzw20-config-version.cmake
      COMPATIBILITY SameMinorVersion
      ARCH_INDEPENDENT
  )
  install(
      FILES
          ${CMAKE_CURRENT_BINARY_DIR}/lzw20-config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/lzw20-config-version.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lzw20
  )
endif()
