#ifndef ITMOLOOPS_DSL_PITCH_H_
#define ITMOLOOPS_DSL_PITCH_H_

#include <optional>
#include <string>

namespace itmoloops::dsl{
  std::optional<int> ParsePitchToMidi(const std::string& s);

} // namespace itmoloops::dsl

#endif // ITMOLOOPS_DSL_PITCH_H