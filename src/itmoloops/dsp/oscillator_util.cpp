#include "itmoloops/dsp/oscillator_util.h"

#include <cmath>

namespace itmoloops::dsp {

  double Clamp(double x, double lo, double hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
  }

  double VelocityToGain(int velocity) {
    int v = velocity;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return static_cast<double>(v) / 100.0;
  }

  double MidiToFreq(int midi) {
    const double a4 = 440.0;
    const double semis = static_cast<double>(midi - 69) / 12.0;
    return a4 * std::pow(2.0, semis);
  }

  double EnvelopeAR(double t_sec, double dur_sec,
                    double attack_sec, double release_sec) {
    if (t_sec < 0.0) return 0.0;
    if (dur_sec <= 0.0) return 0.0;

    double a = attack_sec;
    double r = release_sec;
    if (a < 0.0) a = 0.0;
    if (r < 0.0) r = 0.0;

    double end_amp = 1.0;
    if (a > 0.0) end_amp = Clamp(dur_sec / a, 0.0, 1.0);

    if (t_sec < dur_sec) {
      if (a <= 0.0) return 1.0;
      return Clamp(t_sec / a, 0.0, 1.0);
    }
    if (r <= 0.0) return 0.0;
    if (t_sec >= dur_sec + r) return 0.0;

    const double x = (t_sec - dur_sec) / r;
    return end_amp * Clamp(1.0 - x, 0.0, 1.0);
  }

}  // namespace itmoloops::dsp