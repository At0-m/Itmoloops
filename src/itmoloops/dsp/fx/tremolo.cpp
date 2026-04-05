#include "itmoloops/dsp/fx/tremolo.h"

#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstddef>

#include "itmoloops/dsp/oscillator_util.h"
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

  double Clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
  }
} // namespace

  Tremolo::Tremolo(const dsl::EffectSpec& spec) {
    freq_hz_ = GetDoubleParam(spec.params, "freq", 5.0);
    if (freq_hz_ <= 0.0) freq_hz_ = 5.0;
    depth_ = Clamp01(GetDoubleParam(spec.params, "depth", 0.5));
  }

  void Tremolo::Process(std::vector<float>* mono_buffer, double sample_rate) {
    if (!mono_buffer || sample_rate <= 0.0) return;

    double phase = 0.0;
    const double step = freq_hz_ / sample_rate;

    for (std::size_t i = 0; i < mono_buffer->size(); ++i) {
      double lfo = std::sin(kTwoPi * phase);
      double mod = (1.0 - depth_) + depth_ * (0.5 * (1.0 + lfo));
      (*mono_buffer)[i] = static_cast<float>((*mono_buffer)[i] * mod);

      phase += step;
      if (phase >= 1.0) phase -= 1.0;
    }
  }
} // namespace itmoloops::dsp