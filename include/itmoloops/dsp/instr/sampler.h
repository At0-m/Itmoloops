#ifndef ITMOLOOPS_DSP_INSTR_SAMPLER_H_
#define ITMOLOOPS_DSP_INSTR_SAMPLER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/instrument.h"

namespace itmoloops::dsp {

  struct WavInfo {
    uint16_t format_tag = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
  };

  class Sampler : public IInstrument {
  public:
    explicit Sampler(const dsl::InstrumentSpec& spec);

    void Render(const std::vector<core::NoteEvent>& notes, double out_sample_rate,
                std::vector<float>* out) override;
  private:
    bool LoadSample(const std::string& path);
    void ParseParams(const dsl::InstrumentSpec& spec);
    void SanitizeLoop();

    float SampleAt(double pos) const;
    double WrapLoop(double pos) const;

    void RenderOneNote(const core::NoteEvent& e, double out_sample_rate,
                      std::vector<float>* out) const;

    static bool ReadWavFile(const std::string& path, WavInfo* info, std::vector<char>* data);
    static bool ConvertToMonoFloat(const WavInfo& info, const std::vector<char>& data,
                                   std::vector<float>* out);

    static double Clamp01(double x);

    std::vector<float> sample_;
    int sample_rate_ = 44100;

    int root_midi_ = 60;
    double attack_sec_ = 0.0;
    double release_sec_ = 0.0;

    int64_t loop_start_ = -1;
    int64_t loop_end_ = -1;
  };

}  // namespace itmoloops::dsp

#endif  // ITMOLOOPS_DSP_INSTR_SAMPLER_H_