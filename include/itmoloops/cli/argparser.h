#ifndef ITMOLOOPS_ARGPARSER_H
#define ITMOLOOPS_ARGPARSER_H

#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace itmoloops::cli{

  constexpr int kSampleRate = 44100;
  constexpr int kBitDepth   = 16;
  constexpr int kChannels   = 1;  

  struct Parsed {
    std::string score_path;
    std::string output_wav;

    int sample_rate = kSampleRate;
    int bit_depth   = kBitDepth;
    int chanells    = kChannels;

    bool normalize  = false;
    bool check_only = false;
    bool dump_ast   = false; 
    bool verbose    = false;
  };

  struct ParseResult{
    Parsed parsed;
    bool show_help = false;
    std::vector<std::string> errors;

    [[nodiscard]] bool Ok() const noexcept {
       return errors.empty();
    }
  };

  class Parser{
  public:
    explicit Parser(std::string program = "itmoloops");

    ParseResult Parse(int argc, const char* const argv[]) const;
    void PrintHelp(std::ostream& out) const;
  private:
    std::string program_;
  };
} // namespace itmoloops::cli

#endif // ITMOLOOPS_ARGPARSER_H