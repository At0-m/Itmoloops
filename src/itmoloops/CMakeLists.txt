file(GLOB_RECURSE ITMOLOOPS_LIB_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp"
)

add_library(itmoloops_lib STATIC ${ITMOLOOPS_LIB_SOURCES})

target_include_directories(itmoloops_lib
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(itmoloops_lib PRIVATE -Wall -Wextra -Wpedantic)