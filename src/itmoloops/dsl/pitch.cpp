#include "itmoloops/dsl/pitch.h"

#include <cctype>
#include <optional>
#include <string>
#include <cstddef>

namespace itmoloops::dsl{
namespace {
  int NoteBase(char c){
    switch (std::toupper(c))
    {
      case 'C': return 0;
      case 'D': return 2;
      case 'E': return 4;
      case 'F': return 5;
      case 'G': return 7;
      case 'A': return 9;
      case 'B': return 11;
      default: return -1;
    }
  }
} // namespace 

  std::optional<int> ParsePitchToMidi(const std::string& s){
    if(s.size() < 2) return std::nullopt;
    int base = NoteBase(s[0]);
    if(base < 0) return std::nullopt;

    int acc = 0;
    std::size_t i = 1;
    if(i < s.size() && (s[i] == '#' || s[i] == 'b' || s[i] == 'B')){
      acc = (s[i] == '#') ? 1 : -1;
      ++i;
    }
    if(i >= s.size()) return std::nullopt;

    bool neg = false;
    if(s[i] == '.') {
      neg = true;
      ++i;
    }
    if(i >= s.size()) return std::nullopt;

    int oct = 0;
    for(; i < s.size(); ++i){
      if(!std::isdigit(s[i])) return std::nullopt;
      oct = oct*10 + (s[i] - '0');
    }
    if(neg) oct = -oct;

    int midi = (oct + 1) * 12 + (base + acc);
    if(midi < 0 || midi > 127) return std::nullopt;
    return midi;
  }
} // namespace itmoloops::dsl