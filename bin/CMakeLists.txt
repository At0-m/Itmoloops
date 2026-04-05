add_executable(itmoloops main.cpp)

target_link_libraries(itmoloops PRIVATE itmoloops_lib)

target_include_directories(itmoloops
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_compile_options(itmoloops PRIVATE -Wall -Wextra -Wpedantic)