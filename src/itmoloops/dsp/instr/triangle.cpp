#include "itmoloops/dsp/instr/triangle.h"

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstddef>

#include "itmoloops/dsp/oscillator_util.h"
#include "itmoloops/util/parse_utils.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/core/note_event.h"

namespace itmoloops::dsp {
namespace {

  double GetDoubleParam(const std::unordered_map<std::string, std::string>& params,
                        const char* key, double def) {
    auto it = params.find(key);
    if (it == params.end()) return def;
    double v = def;
    if (!util::ParseDoubleStrict(it->second, &v)) return def;
    return v;
  }

  double NonNegative(double x) { return (x < 0.0) ? 0.0 : x; }

  double TriangleSample(double phase) {
    const double v = 4.0 * std::fabs(phase - 0.5) - 1.0;
    return v;
  }

}  // namespace

  Triangle::Triangle(const dsl::InstrumentSpec& spec) {
    attack_sec_  = NonNegative(GetDoubleParam(spec.params, "attack", 0.0));
    release_sec_ = NonNegative(GetDoubleParam(spec.params, "release", 0.0));
  }

  void Triangle::Render(const std::vector<core::NoteEvent>& notes, double sample_rate,
                        std::vector<float>* out) {
    if (!out || sample_rate <= 0.0) return;
    for (std::size_t i = 0; i < notes.size(); ++i){
       RenderOneNote(notes[i], sample_rate, out);
    }
  }

  void Triangle::RenderOneNote(const core::NoteEvent& e, double sample_rate,
                               std::vector<float>* out) const {
    if (!out || out->empty()) return;

    const int64_t n = out->size();
    int64_t start = static_cast<int64_t>(e.start_sec * sample_rate);
    if (start >= n) return;
    if (start < 0) start = 0;

    const double total_sec = e.dur_sec + release_sec_;
    if (total_sec <= 0.0) return;

    int64_t count = static_cast<int64_t>(total_sec * sample_rate + 0.5);
    if (count <= 0) return;

    int64_t end = start + count;
    if (end > n) end = n;
    count = end - start;
    if (count <= 0) return;

    const double gain = VelocityToGain(e.velocity);
    const double freq = MidiToFreq(e.midi);
    const double step = freq / sample_rate;

    double phase = 0.0;
    for (int64_t i = 0; i < count; ++i) {
      const double t = static_cast<double>(i) / sample_rate;
      const double env = EnvelopeAR(t, e.dur_sec, attack_sec_, release_sec_);
      const double s = TriangleSample(phase);
      (*out)[static_cast<std::size_t>(start + i)] += static_cast<float>(s * env * gain);

      phase += step;
      if (phase >= 1.0) phase -= 1.0;
    }
  }

}  // namespace itmoloops::dsp