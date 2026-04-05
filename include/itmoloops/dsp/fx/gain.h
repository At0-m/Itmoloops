#ifndef ITMOLOOPS_DSP_FX_GAIN_H_
#define ITMOLOOPS_DSP_FX_GAIN_H_

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/effect.h"

namespace itmoloops::dsp{

  class Gain : public IEffect {
  public:
    explicit Gain(const dsl::EffectSpec& spec);

    void Process(std::vector<float>* mono_buffer, double sample_rate) override;

  private:
    double gain_ = 1.0;
  };
} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_FX_GAIN_H_