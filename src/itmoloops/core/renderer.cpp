#include "itmoloops/core/renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <cstdint>
#include <memory>
#include <cstddef>

#include "itmoloops/dsp/registries.h"
#include "itmoloops/core/note_event.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsp/instrument.h"
#include "itmoloops/dsp/effect.h"

namespace itmoloops::core {

  Renderer::Renderer(const dsp::InstrumentRegistry& inst_registry, const dsp::EffectRegistry& eff_registry)
      : inst_registry_(inst_registry), eff_registry_(eff_registry) {}

  int64_t Renderer::ComputeSampleCount(double duration_sec, double sample_rate) const {
    if (duration_sec <= 0.0 || sample_rate <= 0.0) return 0;
    double x = duration_sec * sample_rate;
    if (x < 0.0) return 0;
    int64_t n = static_cast<int64_t>(x + 0.5);
    if (n < 0) n = 0;
    if (n == 0 && duration_sec > 0.0) n = 1;
    return n;
  }

  void Renderer::AssignEventsToBuckets(const std::vector<NoteEvent>& events, int instrument_count,
                                       std::vector<std::vector<NoteEvent>>* buckets) const {
    if (!buckets) return;
    buckets->clear();
    if (instrument_count <= 0) return;
    buckets->resize(instrument_count);
    for (std::size_t i = 0; i < events.size(); ++i) {
      const int id = events[i].instrument_id;
      if (id < 0 || id >= instrument_count) continue;
      (*buckets)[id].push_back(events[i]);
    }
  }

  void Renderer::MixAdd(const std::vector<float>& src, std::vector<float>* dst) const {
    if (!dst) return;
    const std::size_t n = std::min(src.size(), dst->size());
    for (std::size_t i = 0; i < n; ++i) {
      (*dst)[i] += src[i];
    }
  }

  bool Renderer::ApplyEffectChain(const dsl::InstrumentSpec& spec, const RenderOptions& opt,
                                  std::vector<float>* buffer, dsl::Diagnostics* diags) const {
    if (!buffer) return false;
    bool ok = true;
    for (std::size_t i = 0; i < spec.effects.size(); ++i) {
      const dsl::EffectSpec& fx = spec.effects[i];
      std::unique_ptr<dsp::IEffect> effect = eff_registry_.Create(fx.type, fx);
      if (!effect) {
        if (diags) diags->Error(fx.where.begin, "unknown effect type: " + fx.type);
        ok = false;
        continue;
      }
      effect->Process(buffer, opt.sample_rate);
    }
    return ok;
  }

  bool Renderer::RenderOneInstrument(const dsl::InstrumentSpec& spec, int instrument_id,
                                     const std::vector<NoteEvent>& events, const RenderOptions& opt,
                                     std::vector<float>* master, dsl::Diagnostics* diags) const {
    if (!master) return false;

    std::unique_ptr<dsp::IInstrument> inst = inst_registry_.Create(spec.type, spec);
    if (!inst) {
      if (diags) diags->Error(spec.where.begin, "unknown instrument type: " + spec.type);
      return false;
    }

    std::vector<float> track(master->size(), 0.0f);
    inst->Render(events, opt.sample_rate, &track);

    bool fx_ok = ApplyEffectChain(spec, opt, &track, diags);
    MixAdd(track, master);

    (void)instrument_id;
    return fx_ok;
  }

  float Renderer::PeakAbs(const std::vector<float>& buf) const {
    float peak = 0.0f;
    for (std::size_t i = 0; i < buf.size(); ++i) {
      float a = std::fabs(buf[i]);
      if (a > peak) peak = a;
    }
    return peak;
  }

  void Renderer::NormalizeInPlace(std::vector<float>* buf) const {
    if (!buf || buf->empty()) return;
    float peak = PeakAbs(*buf);
    if (peak <= 1.0f || peak <= 0.0f) return;
    float k = 1.0f / peak;
    for (std::size_t i = 0; i < buf->size(); ++i) (*buf)[i] *= k;
  }

  bool Renderer::RenderMaster(const dsl::Module& module, const RenderPlan& plan,
                              const RenderOptions& opt, std::vector<float>* out_master,
                              dsl::Diagnostics* diags) const {
    if (!out_master) return false;

    const std::size_t sample_count = ComputeSampleCount(plan.duration_sec, opt.sample_rate);
    out_master->assign(sample_count, 0.0f);

    std::vector<std::vector<NoteEvent>> buckets;
    AssignEventsToBuckets(plan.events, static_cast<int>(module.instruments.size()), &buckets);

    bool ok = true;
    for (std::size_t i = 0; i < module.instruments.size(); ++i) {
      const std::size_t idx = i;
      bool inst_ok = RenderOneInstrument(module.instruments[idx], i, buckets[idx], opt, out_master, diags);
      if (!inst_ok) ok = false;
    }

    if (opt.normalize) NormalizeInPlace(out_master);
    if (diags) return ok && diags->Ok();
    return ok;
  }

}  // namespace itmoloops::core