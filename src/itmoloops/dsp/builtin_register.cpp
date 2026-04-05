#include "itmoloops/dsp/builtin_register.h"

#include "itmoloops/dsp/instr/sine.h"
#include "itmoloops/dsp/instr/square.h"
#include "itmoloops/dsp/instr/triangle.h"
#include "itmoloops/dsp/instr/sampler.h"

#include "itmoloops/dsp/fx/echo.h"
#include "itmoloops/dsp/fx/gain.h"
#include "itmoloops/dsp/fx/tremolo.h"
#include "itmoloops/dsp/registries.h"

namespace itmoloops::dsp {

  void RegisterBuiltinInstruments(InstrumentRegistry* inst_reg) {
    RegisterInstrument<Sampler>(inst_reg, "sampler");
    RegisterInstrument<Sine>(inst_reg, "sine");
    RegisterInstrument<Square>(inst_reg, "square");
    RegisterInstrument<Triangle>(inst_reg, "triangle");
  }

  void RegisterBuiltinEffects(EffectRegistry* eff_reg) {
    RegisterEffect<Gain>(eff_reg, "gain");
    RegisterEffect<Echo>(eff_reg, "echo");
    RegisterEffect<Tremolo>(eff_reg, "tremolo");
  }

  void RegisterBuiltinDsp(InstrumentRegistry* inst_reg, EffectRegistry* eff_reg) {
    RegisterBuiltinInstruments(inst_reg);
    RegisterBuiltinEffects(eff_reg);
  }

}  // namespace itmoloops::dsp