#ifndef ITMOLOOPS_DSP_FX_TREMOLO_H_
#define ITMOLOOPS_DSP_FX_TREMOLO_H_

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/effect.h"

namespace itmoloops::dsp{

  class Tremolo : public IEffect {
  public:
    explicit Tremolo(const dsl::EffectSpec& spec);
    void Process(std::vector<float>* mono_buffer, double sample_rate) override;

  private:
    double freq_hz_ = 5.0;
    double depth_   = 0.5;
  };

} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_FX_TREMOLO_H_