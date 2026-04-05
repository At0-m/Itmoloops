#ifndef ITMOLOOPS_DSP_FX_ECHO_H_
#define ITMOLOOPS_DSP_FX_ECHO_H_

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/effect.h"

namespace itmoloops::dsp{
  class Echo : public IEffect {
  public:
    explicit Echo(const dsl::EffectSpec& spec);

    void Process(std::vector<float>* mono_buffer, double sample_rate) override;

  private:
    double delay_sec_ = 0.25;
    double decay_ = 0.4;
  };

} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_FX_ECHO_H_