#include "itmoloops/cli/argparser.h"

#include <charconv>
#include <cctype>
#include <string_view>
#include <system_error>
#include <utility>
#include <cstddef>
#include <string>
#include <vector>
#include <ostream>

namespace itmoloops::cli{
namespace{
  bool StartsWith(std::string_view s, std::string_view p){
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
  }
  auto SplitEq(std::string_view t){
    const std::size_t pos = t.find('=');
    return (pos == std::string_view::npos) ? std::make_pair(t, "") : 
                                             std::make_pair(t.substr(0, pos), t.substr(pos + 1));
  }

  template<class T>
  bool FromChars(std::string_view s, T* out){
    static_assert(std::is_arithmetic_v<T>);
    const char* first = s.data();
    const char* last  = s.data() + s.size();
    auto [ptr, ec]    = std::from_chars(first, last, *out);
    return ec == std::errc() && ptr == last; 
  }

  bool ConsumeValue(int argc, const char* const argv[], int* i,
                    std::string_view token, std::string_view short_name,
                    std::string_view long_name, std::string* out, 
                    std::vector<std::string>* errors){
    auto [name, after_eq] = SplitEq(token);
    if(name != short_name && name != long_name) return false;
    if(!after_eq.empty()) {
      *out = after_eq;
      return true;
    }
    if(name == short_name && token.size() > short_name.size()){
      *out = token.substr(short_name.size());
      return true;
    }
    if(*i + 1 >= argc){
      errors->push_back(std::string(name) + " requires a value");
      return true;
    }
    *out = argv[++(*i)];
    return true;
  }

  bool HandleFlag(std::string_view token, std::string_view short_name,
                  std::string_view long_name, bool* dst){
    if(token == short_name || token == long_name){
      *dst = true;
      return true;
    }
    return false;
  }

  bool EndsWithNoCase(std::string_view s, std::string_view suf){
    if(s.size() < suf.size()) return false;
    const std::size_t off = s.size() - suf.size();
    for(std::size_t i = 0; i < suf.size(); ++i){
      char a = std::tolower(s[off + i]);
      char b = std::tolower(suf[i]);
      if(a != b) return false;
    }
    return true;
  }

  void ParseArgs(int argc, const char* const argv[],
                 Parsed* parsed, ParseResult* res){
    bool stop_ops = false;
    std::vector<std::string>& errs = res->errors;
    std::vector<std::string> pos;

    for(int i = 1; i < argc; ++i){
      std::string_view tok(argv[i]);

      if(tok == "--"){
        stop_ops = true;
        continue;
      }

      if(!stop_ops && StartsWith(tok, "-")){
        if(tok == "-h" || tok == "--help"){
          res->show_help = true;
          continue;
        }

        if(HandleFlag(tok, "-n", "--normalize",  &parsed->normalize))  continue;
        if(HandleFlag(tok, "",   "--check-only", &parsed->check_only)) continue;
        if(HandleFlag(tok, "",   "--dump-ast",   &parsed->dump_ast))   continue;
        if(HandleFlag(tok, "-v", "--verbose",    &parsed->verbose))    continue;

        std::string value;
        if(ConsumeValue(argc, argv, &i, tok, "-s", "--sample-rate", &value, &errs)){
          if(!value.empty()){
            int sr;
            if(!FromChars(value, &sr) || sr <= 0) errs.push_back("Invalid --sample-rate");
            else parsed->sample_rate = sr;
          }
          continue;
        }
        if(ConsumeValue(argc, argv, &i, tok, "-b", "--bit-depth", &value, &errs)){
          if(!value.empty()){
            int bd;
            if(!FromChars(value, &bd) || bd <= 0) errs.push_back("Invalid --bit-depth");
            else parsed->bit_depth = bd;
          }
          continue;
        }
        if(ConsumeValue(argc, argv, &i, tok, "-c", "--channels", &value, &errs)){
          if(!value.empty()){
            int ch;
            if(!FromChars(value, &ch) || ch <= 0) errs.push_back("Invalid --channels");
            else parsed->chanells = ch;
          }
          continue;
        }

        errs.push_back(std::string("Unknown option: ") + std::string(tok));
        continue;
      }

      pos.emplace_back(tok);
    }

    if(parsed->check_only){
      if(pos.size() == 1){
        parsed->score_path = pos[0];
      } else if(pos.size() == 2){
        parsed->score_path = pos[0];
        parsed->output_wav = pos[1];
      } else if(pos.empty()){
        errs.emplace_back("Need Score file");
      } else {
        errs.emplace_back("Too many positional args");
      }
    } else {
      if(pos.size() != 2){
        errs.emplace_back("Usage: " + std::string(argv[0]) + " ScoreOut.wav");
      } else {
        parsed->score_path = pos[0];
        parsed->output_wav = pos[1];
      }
    }

    if(parsed->sample_rate != kSampleRate){
      errs.emplace_back("sample rate has to be equal 44100");
    }
    if(parsed->bit_depth != kBitDepth){
      errs.emplace_back("bit depth has to be equal 16");
    }
    if(parsed->chanells != kChannels){
      errs.emplace_back("channels has to be equal 1(mono)");
    }

    if(!parsed->output_wav.empty() && !EndsWithNoCase(parsed->output_wav, ".wav")){
      errs.emplace_back("Output file must have .wav extension");
    }
  }

  void PrintHelpBody(std::ostream& out, const std::string& prog){
    out <<R"(usage:)" << prog << R"( SCORE OUT.wav)" << prog << R"( --check-only SCORE
      Options:
        -h, --help               Show this help
        -v, --verbose            Verbose logging
        -n, --normalize          Normalize master to 0 dBFS
            --check-only         Parse & validate only (no rendering)
            --dump-ast           Print parsed AST (debug)

      WAV format (per lab spec, fixed):
        -s, --sample-rate  44100 (required)
        -b, --bit-depth    16    (required)
        -c, --channels     1     (required)
      )";
  }
} // namespace 

  Parser::Parser(std::string program) : program_(std::move(program)) {}

  ParseResult Parser::Parse(int argc, const char* const argv[]) const {
    ParseResult res;
    if(argc <= 0 || argv == nullptr){
      res.errors.emplace_back("Invalid argc/argv");
      return res;
    }

    ParseArgs(argc, argv, &res.parsed, &res);
    return res;
  }

  void Parser::PrintHelp(std::ostream& out) const {
    PrintHelpBody(out, program_);
  }
} // namespace itmoloops::cli
