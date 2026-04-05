#ifndef ITMOLOOPS_DSL_SOURCE_H_
#define ITMOLOOPS_DSL_SOURCE_H_

#include <cstddef>
#include <string>

namespace itmoloops::dsl{
  struct Location
  {
    std::size_t line = 1;
    std::size_t col  = 1;
  };

  struct Range{
    Location begin;
    Location end;
  };

  struct FileSlice{
    const char* data = nullptr;
    std::size_t size = 0;
  };
  
} // namespace itmoloops::dsl

#endif // ITMOLOOPS_DSL_SOURCE_H_