find_package(Catch2 3 CONFIG REQUIRED)

function(yk_lzw20_add_test test_name)
  set(target_name yk_lzw20_${test_name}_test)
  add_executable(${target_name} ${test_name}.cpp)
  target_compile_features(${target_name} PRIVATE cxx_std_20)
  target_link_libraries(${target_name} PRIVATE yk::lzw20 Catch2::Catch2WithMain)
  add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

yk_lzw20_add_test(codec)
yk_lzw20_add_test(compile_time)

# Every header compiles on its own: one translation unit per header, including it twice
file(
    GLOB_RECURSE YK_LZW20_PARTS
    RELATIVE ${PROJECT_SOURCE_DIR}/include
    CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/include/*.hpp
)
set(YK_LZW20_PART_SOURCES)
foreach(part IN LISTS YK_LZW20_PARTS)
  string(MAKE_C_IDENTIFIER ${part} part_name)
  set(source ${CMAKE_CURRENT_BINARY_DIR}/self_contained/${part_name}.cpp)
  file(CONFIGURE OUTPUT ${source} CONTENT "#include <${part}>\n#include <${part}>\n")
  list(APPEND YK_LZW20_PART_SOURCES ${source})
endforeach()
add_library(yk_lzw20_self_contained OBJECT ${YK_LZW20_PART_SOURCES})
target_link_libraries(yk_lzw20_self_contained PRIVATE yk::lzw20)
