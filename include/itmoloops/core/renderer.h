#ifndef ITMOLOOPS_CORE_RENDERER_H_
#define ITMOLOOPS_CORE_RENDERER_H_

#include <cstdint>
#include <vector>

#include "itmoloops/core/note_event.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsp/registries.h"

namespace itmoloops::core {
  struct RenderOptions{
    double sample_rate = 44100.0;
    bool normalize = false;
  };

  class Renderer {
  public:
    Renderer(const dsp::InstrumentRegistry& inst_registry, const dsp::EffectRegistry& eff_registry);

    bool RenderMaster(const dsl::Module& module, const RenderPlan& plan,
                      const RenderOptions& opt, std::vector<float>* out_master,
                      dsl::Diagnostics* diags) const;

  private:
    int64_t ComputeSampleCount(double duration_sec, double sample_rate) const;

    void AssignEventsToBuckets(const std::vector<NoteEvent>& events, int instrument_count,
                               std::vector<std::vector<NoteEvent>>* buckets) const;

    bool RenderOneInstrument(const dsl::InstrumentSpec& spec, int instrument_id,
                             const std::vector<NoteEvent>& events, const RenderOptions& opt,
                             std::vector<float>* master, dsl::Diagnostics* diags) const;

    bool ApplyEffectChain(const dsl::InstrumentSpec& spec, const RenderOptions& opt,
                          std::vector<float>* buffer, dsl::Diagnostics* diags) const;

    void MixAdd(const std::vector<float>& src, std::vector<float>* dst) const;

    float PeakAbs(const std::vector<float>& buf) const;
    void NormalizeInPlace(std::vector<float>* buf) const;

    const dsp::InstrumentRegistry& inst_registry_;
    const dsp::EffectRegistry& eff_registry_;
  };

} // namespace itmoloops::core

#endif // ITMOLOOPS_CORE_RENDERER_H_