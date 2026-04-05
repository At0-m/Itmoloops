#ifndef ITMOLOOPS_DSP_INSTR_SINE_H_
#define ITMOLOOPS_DSP_INSTR_SINE_H_

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/instrument.h"

namespace itmoloops::dsp{

  class Sine : public IInstrument {
  public:
    explicit Sine(const dsl::InstrumentSpec& spec);

    void Render(const std::vector<core::NoteEvent>& notes,
                double sample_rate,
                std::vector<float>* out) override;

  private:
    void RenderOneNote(const core::NoteEvent& e, double sample_rate,
                       std::vector<float>* out) const;

    double attack_sec_ = 0.0;
    double release_sec_ = 0.0;
  };

} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_INSTR_SINE_H_