#ifndef ITMOLOOPS_DSP_BUILTIN_REGISTER_H_
#define ITMOLOOPS_DSP_BUILTIN_REGISTER_H_

#include "itmoloops/dsp/registries.h"

namespace itmoloops::dsp{
  
  void RegisterBuiltinInstruments(InstrumentRegistry* inst_reg);

  void RegisterBuiltinEffects(EffectRegistry* eff_reg);

  void RegisterBuiltinDsp(InstrumentRegistry* inst_reg, EffectRegistry* eff_reg);
} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_BUILTIN_REGISTER_H_