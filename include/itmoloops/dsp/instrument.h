#ifndef ITMOLOOPS_DSP_INSTRUMENT_H_
#define ITMOLOOPS_DSP_INSTRUMENT_H_

#include <memory>
#include <vector>

#include "itmoloops/core/note_event.h"

namespace itmoloops::dsp{

  class IInstrument{
  public:
    virtual ~IInstrument() = default;
    
    virtual void Render(const std::vector<core::NoteEvent>& notes, 
                        double sample_rate, std::vector<float>* out) = 0;
  };

} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_INSTRUMENT_H_