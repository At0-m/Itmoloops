#ifndef ITMOLOOPS_UTIL_PARSE_UTILS_H_
#define ITMOLOOPS_UTIL_PARSE_UTILS_H_

#include <cstdint>
#include <string>
#include <string_view>

namespace itmoloops::util {

  bool ParseDoubleStrict(const std::string& s, double* out);

  bool ParseIntStrict(const std::string& s, int* out);

  bool ParseI64Strict(const std::string& s, int64_t* out);

  bool ParseI64Pair(const std::string& s, int64_t* a, int64_t* b);


  bool IsAbsolutePath(const std::string& p);

  std::string JoinPath(std::string_view dir, const std::string& rel);

  std::string ResolveSamplePath(std::string_view score_dir, const std::string& rel);

  bool FileExists(const std::string& path);

}  // namespace itmoloops::util

#endif  // ITMOLOOPS_UTIL_PARSE_UTILS_H_