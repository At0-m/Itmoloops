#ifndef ITMOLOOPS_DSP_OSCILLATOR_UTIL_H_
#define ITMOLOOPS_DSP_OSCILLATOR_UTIL_H_

namespace itmoloops::dsp{
  constexpr double kTwoPi = 6.2831853071795864769;

  double Clamp(double x, double lo, double hi);

  double VelocityToGain(int velocity);

  double MidiToFreq(int midi);

  double EnvelopeAR(double t_sec, double dur_sec,
                    double attack_sec, double release_sec);

} // namespace itmoloops::dsp

#endif // ITMOLOOPS_DSP_OSCILLATOR_UTIL_H_