#include "itmoloops/dsp/fx/echo.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

#include "itmoloops/util/parse_utils.h"
#include "itmoloops/dsl/ast.h"

namespace itmoloops::dsp{
namespace{
  double GetDoubleParam(const std::unordered_map<std::string, std::string>& params, const char* key, double def) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    double v = def;
    if (!util::ParseDoubleStrict(it->second, &v)) return def;
    return v;
  }
} // namespace 

  Echo::Echo(const dsl::EffectSpec& spec) {
    delay_sec_ = GetDoubleParam(spec.params, "delay", 0.25);
    if (delay_sec_ < 0.0) delay_sec_ = 0.0;
    decay_ = GetDoubleParam(spec.params, "decay", 0.4);
  }

  void Echo::Process(std::vector<float>* mono_buffer, double sample_rate) {
    if (!mono_buffer || sample_rate <= 0.0) return;
    if (delay_sec_ <= 0.0) return;
    if (decay_ == 0.0) return;

    const int64_t n = mono_buffer->size();
    int64_t d = static_cast<int64_t>(delay_sec_ * sample_rate + 0.5);
    if (d <= 0 || d >= n) return;

    for (int64_t i = d; i < n; ++i) {
      (*mono_buffer)[i] += static_cast<float>(decay_ * (*mono_buffer)[i - d]);
    }
  }
} // namespace itmoloops::dsp