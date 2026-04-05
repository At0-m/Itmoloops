#include "itmoloops/util/parse_utils.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstdint>
#include <cctype>
#include <string_view>
#include <iostream>

namespace itmoloops::util {

  bool ParseDoubleStrict(const std::string& s, double* out) {
    if (!out || s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const char* begin = s.c_str();
    double v = std::strtod(begin, &end);
    if (end == begin || *end != '\0' || errno == ERANGE) return false;
    *out = v;
    return true;
  }

  bool ParseIntStrict(const std::string& s, int* out) {
    if (!out || s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str() || *end != '\0' || errno == ERANGE) return false;
    if (v < INT_MIN || v > INT_MAX) return false;
    *out = static_cast<int>(v);
    return true;
  }

  bool ParseI64Strict(const std::string& s, int64_t* out) {
    if (!out || s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (end == s.c_str() || *end != '\0' || errno == ERANGE) return false;
    *out = static_cast<int64_t>(v);
    return true;
  }

  bool ParseI64Pair(const std::string& s, int64_t* a, int64_t* b) {
    std::size_t pos = s.find(',');
    if (pos == std::string::npos) return false;
    return ParseI64Strict(s.substr(0, pos), a) &&
           ParseI64Strict(s.substr(pos + 1), b);
  }


  bool IsAbsolutePath(const std::string& p) {
    if (p.empty()) return false;
    if (p[0] == '/' || p[0] == '\\') return true;
    return (p.size() > 1 && std::isalpha(static_cast<unsigned char>(p[0])) && p[1] == ':');
  }

  std::string JoinPath(std::string_view dir, const std::string& rel) {
    if (dir.empty()) return rel;
    if (rel.empty()) return std::string(dir);
    std::string res(dir);
    char last = res.back();
    if (last != '/' && last != '\\') res.push_back('/');
    res += rel;
    return res;
  }

  std::string ResolveSamplePath(std::string_view score_dir, const std::string& rel) {
    if (IsAbsolutePath(rel)) return rel;
    std::string p1 = JoinPath(score_dir, rel);

    if (FileExists(p1)) return p1;

    if (FileExists(rel)) return rel;

    std::string sd(score_dir);
    std::size_t pos = sd.find_last_of("/\\");
    std::string parent = (pos == std::string::npos) ? "." : sd.substr(0, pos);
    std::string p2 = JoinPath(parent, rel);
    if (FileExists(p2)) return p2;

    return p1;
  }

  bool FileExists(const std::string& path) {
    std::ifstream f(path.c_str(), std::ios::binary);
    return static_cast<bool>(f);
  }

}  // namespace itmoloops::util