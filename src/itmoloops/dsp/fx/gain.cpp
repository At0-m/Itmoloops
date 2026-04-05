#include "itmoloops/dsp/fx/gain.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <cstddef>
#include "itmoloops/util/parse_utils.h"
#include "itmoloops/dsl/ast.h"

namespace itmoloops::dsp{
namespace {
  double GetDoubleParam(const std::unordered_map<std::string, std::string>& params, const char* key, double def) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    double v = def;
    if (!util::ParseDoubleStrict(it->second, &v)) return def;
    return v;
  }
} // namespace 

  Gain::Gain(const dsl::EffectSpec& spec) {
    gain_ = GetDoubleParam(spec.params, "gain", 1.0);
  }

  void Gain::Process(std::vector<float>* mono_buffer, double sample_rate) {
    if (!mono_buffer) return;
    for (std::size_t i = 0; i < mono_buffer->size(); ++i) {
      (*mono_buffer)[i] = static_cast<float>((*mono_buffer)[i] * gain_);
    }
  }

} // namespace itmoloops::dsp