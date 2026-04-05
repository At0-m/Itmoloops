#ifndef ITMOLOOPS_DSP_EFFECT_H_
#define ITMOLOOPS_DSP_EFFECT_H_

#include <vector>

namespace itmoloops::dsp{
  class IEffect{
  public:
    virtual ~IEffect() = default;

    virtual void Process(std::vector<float>* mono_bufer, double sample_rate) = 0;
  };
} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_EFFECT_H_